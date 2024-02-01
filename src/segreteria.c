#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <mysql/mysql.h>

#define max(x, y) ({typeof (x) x_ = (x); typeof (y) y_ = (y); \
x_ > y_ ? x_ : y_;})

typedef struct {
    int connfd;
    int matricola;
} Client_stud;

int main(int argc, char **argv) {
    int sockfd, listenfd;
    int behaviour;
    int logical;
    int dim = 0;
    struct sockaddr_in servaddr, secaddr;
    char name[255] = {0};
    char date[12] = {0};
    char result[255] = {0};
    char password[255] = {0};
    MYSQL *conn;
    Client_stud client_sockets[4096];

    // SEGRETERIA CLIENT

    /**
     * Controllo che venga inserito da riga di comando l'indirizzo IP relativo al server al quale ci si vuole connettere.
     */
    if (argc != 2) {
        fprintf(stderr, "Utilizzo: %s <indirizzoIP>\n", argv[0]);
        exit(1);
    }

    /**
     * Utilizzo la system call socket, che prende in input tre parametri di tipo intero, per creare una nuova socket
     * da associare al descrittore "sockfd". I tre parametri in input riguardano, in ordine, il dominio
     * degli indirizzi IP (IPv4 in questo caso), il protocollo di trasmissione (in questo caso TCP), mentre l'ultimo
     * parametro, se messo a 0, specifica che si tratta del protocollo standard.
     */
    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }

    /**
     * Specifico la struttura dell'indirizzo del server al quale ci si vuole connettere tramite i campi di una struct
     * di tipo sockaddr_in. Vengono utilizzati indirizzi IPv4, ci si connette all'indirizzo IP sul quale si trova
     * il server, il localhost in questo caso, e la porta su cui sta attendendo il server.
     */
    servaddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "Errore inet_pton per %s\n", argv[1]);
        exit(1);
    }
    servaddr.sin_port = htons(1025);

    /**
     * La system call connect permette di connettere la socket al server specificato nella struct "servaddr" tramite
     * l'indirizzo IP e la porta memorizzate nella struttura.
     */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Errore nell'operazione di connect!");
        exit(1);
    }


    // SEGRETERIA SERVER

    /**
     * Utilizzo la system call socket, che prende in input tre parametri di tipo intero, per creare una nuova socket
     * da associare al descrittore "listenfd". I tre parametri in input riguardano, in ordine, il dominio
     * degli indirizzi IP (IPv4 in questo caso), il protocollo di trasmissione (in questo caso TCP), mentre l'ultimo
     * parametro, se messo a 0, specifica che si tratta del protocollo standard.
     */
    if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }

    /**
     * Specifico la struttura dell'indirizzo del server tramite i campi di una struct di tipo sockaddr_in.
     * Vengono utilizzati indirizzi IPv4, vengono accettate connessioni da qualsiasi indirizzo e la porta su cui
     * il server risponderà ai client sarà la 1025.
     */
    secaddr.sin_family = AF_INET;
    secaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    secaddr.sin_port = htons(1026);

    /**
     * La system call bind permette di assegnare l'indirizzo memorizzato nel campo s_addr della struct sin_addr, che è
     * a sua volta un campo della struct sockaddr_in (secaddr nel nostro caso), al descrittore listenfd.
     */
    if ((bind(listenfd, (struct sockaddr *)&secaddr, sizeof (secaddr))) < 0) {
        perror("Errore nell'operazione di bind!");
        exit(1);
    }

    /**
     * Mettiamo il server in ascolto, specificando quante connessioni possono essere in attesa venire accettate
     * tramite il secondo argomento della chiamata.
     */
    if ((listen(listenfd, 1024)) < 0) {
        perror("Errore nell'operazione di listen!");
        exit(1);
    }

    conn = mysql_init(NULL);

    if (conn == NULL) {
        fprintf(stderr, "mysql_init() fallita\n");
        exit(1);
    }

    /**
     * Connessione vera e propria al database MySQL specificato da host, user, password e nome dello schema.
     */
    if (mysql_real_connect(conn, "192.168.1.84", "root", "luca2001", "universita", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() fallita: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    fd_set read_set, write_set, master_set;
    int max_fd;

    FD_ZERO(&master_set);

    FD_SET(sockfd, &master_set);
    max_fd = sockfd;

    FD_SET(listenfd, &master_set);
    max_fd = max(max_fd, listenfd);

    while(1) {
        read_set = master_set;
        write_set = master_set;

        while (1) {
            if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0) {
                perror("Errore nell'operazione di select!");
                exit(1);
            }

            if (FD_ISSET(listenfd, &read_set)) {
                /**
                 * La system call accept permette di accettare una nuova connessione (lato server) in entrata da un client.
                 */
                if ((client_sockets[dim].connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
                    perror("Errore nell'operazione di accept!");
                    exit(1);
                }

                FD_SET(client_sockets[dim].connfd, &master_set);
                max_fd = max(max_fd, client_sockets[dim].connfd);

                read(client_sockets[dim].connfd, &client_sockets[dim].matricola, sizeof(client_sockets[dim].matricola));
                read(client_sockets[dim].connfd, password, sizeof(password));

                /**
                 * Componiamo una stringa che fungerà da query di inserimento nella tabella appello.
                 */
                char q[500];

                snprintf(q, sizeof(q), "SELECT * FROM STUDENTE WHERE matricola = %d AND password = SHA2('%s',256)",
                         client_sockets[dim].matricola, password);

                if (mysql_query(conn, q) != 0) {
                    fprintf(stderr, "mysql_query() fallita\n");
                    mysql_close(conn);
                    exit(1);
                }

                MYSQL_RES *res1 = mysql_store_result(conn);
                if (res1 == NULL) {
                    fprintf(stderr, "mysql_store_result() fallita\n");
                    mysql_close(conn);
                    exit(1);
                }

                unsigned int num_rows = mysql_num_rows(res1);

                mysql_free_result(res1);

                if (num_rows != 1) {
                    mysql_close(conn);
                    char ret[255] = "credenziali non corrette, accesso negato!";
                    write(client_sockets[dim].connfd, ret, strlen(ret));
                } else {
                    char ret[255] = "credenziali corrette, accesso consentito!";
                    write(client_sockets[dim].connfd, ret, strlen(ret));
                }

                dim++;
            }
            else {
                break;
            }
        }

        for (int i=0; i < dim; i++) {
            if (FD_ISSET(client_sockets[i].connfd, &read_set)) {
                read(client_sockets[i].connfd, &behaviour, sizeof(behaviour));

                if (behaviour == 1) {
                    int req;
                    read(client_sockets[i].connfd, &req, sizeof(req));

                    if (req == 1) {
                        /**
                         * Componiamo una stringa che fungerà da query di inserimento nella tabella appello.
                         */
                        char query[500];

                        snprintf(query, sizeof(query), "SELECT a.id, a.nome, DATE_FORMAT(a.data_appello, '%%Y-%%m-%%d')"
                                                       " FROM appello a join esame e ON a.nome = e.nome"
                                                       " WHERE e.cds = (SELECT piano_studi FROM studente WHERE matricola = %d)", client_sockets[i].matricola);

                        if (mysql_query(conn, query) != 0) {
                            fprintf(stderr, "mysql_query() fallita\n");
                            mysql_close(conn);
                            exit(1);
                        }

                        MYSQL_RES *res2 = mysql_store_result(conn);
                        if (res2 == NULL) {
                            fprintf(stderr, "mysql_store_result() fallita\n");
                            mysql_close(conn);
                            exit(1);
                        }

                        unsigned int num_rows = mysql_num_rows(res2);
                        write(client_sockets[i].connfd, &num_rows, sizeof(num_rows));

                        int id;
                        char name[255];
                        char date[12];

                        MYSQL_ROW row;
                        while ((row = mysql_fetch_row(res2))) {
                            sscanf(row[0], "%d", &id);
                            sscanf(row[1], "%[^\n]", name);
                            sscanf(row[2], "%s", date);
                            write(client_sockets[i].connfd, &id, sizeof(id));
                            write(client_sockets[i].connfd, name, sizeof(name));
                            write(client_sockets[i].connfd, date, sizeof(date));
                        }
                        mysql_free_result(res2);
                    }
                    else if (req == 2) {
                        char exam[255] = {0};
                        read(client_sockets[i].connfd, exam, sizeof(exam));

                        /**
                         * Componiamo una stringa che fungerà da query di inserimento nella tabella appello.
                         */
                        char query[500];

                        snprintf(query, sizeof(query), "SELECT a.id, a.nome, DATE_FORMAT(a.data_appello, '%%Y-%%m-%%d')"
                                                       " FROM appello a join esame e ON a.nome = e.nome"
                                                       " WHERE e.cds = (SELECT piano_studi FROM studente WHERE matricola = %d)"
                                                       " AND e.nome = '%s'", client_sockets[i].matricola, exam);

                        if (mysql_query(conn, query) != 0) {
                            fprintf(stderr, "mysql_query() fallita\n");
                            mysql_close(conn);
                            exit(1);
                        }

                        MYSQL_RES *res2 = mysql_store_result(conn);
                        if (res2 == NULL) {
                            fprintf(stderr, "mysql_store_result() fallita\n");
                            mysql_close(conn);
                            exit(1);
                        }

                        unsigned int num_rows = mysql_num_rows(res2);
                        write(client_sockets[i].connfd, &num_rows, sizeof(num_rows));

                        int id;
                        char name[255];
                        char date[12];

                        MYSQL_ROW row;
                        while ((row = mysql_fetch_row(res2))) {
                            sscanf(row[0], "%d", &id);
                            sscanf(row[1], "%[^\n]", name);
                            sscanf(row[2], "%s", date);
                            write(client_sockets[i].connfd, &id, sizeof(id));
                            write(client_sockets[i].connfd, name, sizeof(name));
                            write(client_sockets[i].connfd, date, sizeof(date));
                        }
                        mysql_free_result(res2);
                    }
                }
                else if (behaviour == 2) {
                    int cod, mat;
                    char res[255] = {0};

                    write(sockfd, &behaviour, sizeof(behaviour));

                    read(client_sockets[i].connfd, &cod, sizeof(cod));
                    read(client_sockets[i].connfd, &mat, sizeof(mat));

                    write(sockfd, &cod, sizeof(cod));
                    write(sockfd, &mat, sizeof(mat));

                    read(sockfd, res, sizeof(res));

                    write(client_sockets[i].connfd, res, sizeof(res));
                }
                else if (behaviour == 3) {
                    close(client_sockets[i].connfd);
                    client_sockets[i].matricola = 0;
                    FD_CLR(client_sockets[i].connfd, &master_set);
                    mysql_close(conn);
                }
            }
        }

        if (FD_ISSET(sockfd, &write_set)) {
            printf("Vuoi gestire le richieste degli studenti o inserire un nuovo appello?\n");
            printf("1 - Gestire le richieste degli studenti\n");
            printf("2 - Inserire un nuovo appello\n");
            printf("Scelta: ");

            scanf("%d", &logical);
            printf("\n");

            /**
             * Pulisco il buffer di input.
             */
            int c;
            while ((c = getchar()) != '\n' && c != EOF);

            if (logical == 2) {
                int request = 1;
                write(sockfd, &request, sizeof(request));

                printf("Inserire il nome dell'esame di cui si vuole aggiungere un appello:\n");
                fgets(name, sizeof(name), stdin);
                name[strlen(name)-1] = 0;

                printf("\n");

                write(sockfd, name, strlen(name));

                printf("Inserire la data del nuovo appello (in formato YYYY-MM-DD):\n");
                fgets(date, sizeof(date), stdin);
                date[strlen(date)-1] = 0;

                write(sockfd, date, strlen(date));

                read(sockfd, result, sizeof(result));

                printf("\nEsito: %s\n\n", result);
            }
            else {
                continue;
            }
        }
    }

    return 0;
}
