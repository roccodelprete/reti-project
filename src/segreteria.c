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

    /**
     * Inizializzo il database mediante l'oggetto di connessione su cui poi verrà effettuata la connessione vera e propria
     * successivamente.
     */
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

    /**
     * Ci serviamo di 3 "insiemi" di descrittori per strutturare la successiva select.
     * In particolare abbiamo read_set e write_set che mantengono l'insieme dei descrittori, rispettivamente in lettura
     * e scrittura, e master_set, che permette di reinizializzare read_set e write_set ad ogni iterazione.
     */
    fd_set read_set, write_set, master_set;
    int max_fd;

    /**
     * Inizializzo a 0 tutti i bit relativi ai descrittori del master_set, dopodichè aggiungo a questo array
     * di descrittori sia la socket che permette la connessione al server universitario che quella che permette l'ascolto
     * delle richieste di connessione provenienti dagli studenti.
     * La variabile max_fd serve a specificare quante posizioni degli array di descrittori devono essere controllate
     * all'interno della funzione select.
     */
    FD_ZERO(&master_set);

    FD_SET(sockfd, &master_set);
    max_fd = sockfd;

    FD_SET(listenfd, &master_set);
    max_fd = max(max_fd, listenfd);

    /**
     * Ciclo infinito per effettuare operazioni nel minor tempo possibile.
     */
    while(1) {
        read_set = master_set;
        write_set = master_set;

        /**
         * Come prima cosa ciclo while per soddisfare tutte le richieste di connessione degli studenti in attesa.
         */
        while (1) {
            /**
             * La funzione select restituisce il numero di descrittori pronti, a partire dagli "insiemi" di descrittori
             * passati.
             */
            if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0) {
                perror("Errore nell'operazione di select!");
            }

            /**
             * Si controlla se il descrittore listenfd, ossia quello che monitora le nuove richieste di connessioni,
             * sia pronto, il che è vero quando appunto ci sono nuove connessioni in attesa.
             */
            if (FD_ISSET(listenfd, &read_set)) {
                /**
                 * La system call accept permette di accettare una nuova connessione (lato server) in entrata da un client.
                 */
                if ((client_sockets[dim].connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
                    perror("Errore nell'operazione di accept!");
                }
                else {
                    /**
                     * Si aggiunge il descrittore legato alla nuova connessione da uno studente all'interno dell'array di
                     * descrittori master_set e si ricalcola il numero di posizioni da controllare nella select.
                     */
                    FD_SET(client_sockets[dim].connfd, &master_set);
                    max_fd = max(max_fd, client_sockets[dim].connfd);

                    /**
                     * La segreteria riceve le credenziali di accesso inserite dallo studente e procede ad un controllo
                     * delle stesse nella tabella apposita del database.
                     */
                    read(client_sockets[dim].connfd, &client_sockets[dim].matricola, sizeof(client_sockets[dim].matricola));
                    read(client_sockets[dim].connfd, password, sizeof(password));

                    /**
                     * Componiamo una stringa che fungerà da query.
                     */
                    char q[500];

                    snprintf(q, sizeof(q), "SELECT * FROM STUDENTE WHERE matricola = %d AND password = SHA2('%s',256)",
                             client_sockets[dim].matricola, password);

                    if (mysql_query(conn, q) != 0) {
                        fprintf(stderr, "mysql_query() fallita\n");
                    }

                    MYSQL_RES *res1 = mysql_store_result(conn);
                    if (res1 == NULL) {
                        fprintf(stderr, "mysql_store_result() fallita\n");
                    }

                    unsigned int num_rows = mysql_num_rows(res1);

                    mysql_free_result(res1);

                    /**
                     * Se vi è una riga risultante dalla query significa che le credenziali sono corrette, altrimenti
                     * ovviamente sono errate, ma in entrambi i casi viene inviato l'esito dell'operazione allo studente.
                     */
                    if (num_rows != 1) {
                        char ret[255] = "credenziali non corrette, accesso negato!";
                        write(client_sockets[dim].connfd, ret, strlen(ret));
                    } else {
                        char ret[255] = "credenziali corrette, accesso consentito!";
                        write(client_sockets[dim].connfd, ret, strlen(ret));
                        /**
                         * Incremento il numero di connessioni accettate all'interno di questo ciclo, in modo da effettuare
                         * successivamente le richieste provenienti dai "dim" studenti che si sono connessi.
                         */
                        dim++;
                    }
                }
            }
            else {
                break;
            }
        }

        /**
         * Itero in base al valore di dim in modo da soddisfare le richieste di tutti gli studenti connessi in modo
         * concorrente.
         */
        for (int i=0; i < dim; i++) {
            /**
             * Controllo se i descrittori delle socket di connessione dei client sono pronte in lettura, poichè
             * significherebbe che lo studente specifico è pronto in scrittura.
             */
            if (FD_ISSET(client_sockets[i].connfd, &read_set)) {
                /**
                 * La segreteria riceve la scelta di operazione da effettuare da parte dello studente.
                 */
                read(client_sockets[i].connfd, &behaviour, sizeof(behaviour));

                /**
                 * Se la scelta è 1 significa che lo studente vuole visualizzare gli appelli disponibili.
                 */
                if (behaviour == 1) {
                    /**
                     * La segreteria riceve inoltre la scelta da parte dello studente, in base a se vuole visualizzare
                     * tutti gli appelli disponibili del suo corso, oppure solo quelli relativi ad un determinato esame.
                     */
                    int req;
                    read(client_sockets[i].connfd, &req, sizeof(req));

                    /**
                     * Componiamo una stringa che fungerà da query per l'operazione appena precedentemente descritta.
                     */
                    char query[500];

                    /**
                     * Se la scelta è 1 significa che vuole visualizzare tutti gli appelli disponibili per il suo corso
                     * di studi.
                     */
                    if (req == 1) {
                        snprintf(query, sizeof(query), "SELECT a.id, a.nome, DATE_FORMAT(a.data_appello, '%%Y-%%m-%%d')"
                                                       " FROM appello a join esame e ON a.nome = e.nome"
                                                       " WHERE e.cds = (SELECT piano_studi FROM studente WHERE matricola = %d)", client_sockets[i].matricola);

                        if (mysql_query(conn, query) != 0) {
                            fprintf(stderr, "mysql_query() fallita\n");

                        }

                    }
                    /**
                     * Se la richiesta è 2 significa che lo studente vuole visualizzare gli appelli disponibili solo
                     * relativi ad un esame specifico.
                     */
                    else if (req == 2) {
                        char exam[255] = {0};
                        /**
                         * La segreteria riceve il nome dell'esame di cui lo studente vuole visualizzare gli appelli
                         * disponibili.
                         */
                        read(client_sockets[i].connfd, exam, sizeof(exam));

                        snprintf(query, sizeof(query), "SELECT a.id, a.nome, DATE_FORMAT(a.data_appello, '%%Y-%%m-%%d')"
                                                       " FROM appello a join esame e ON a.nome = e.nome"
                                                       " WHERE e.cds = (SELECT piano_studi FROM studente WHERE matricola = %d)"
                                                       " AND e.nome = '%s'", client_sockets[i].matricola, exam);

                        if (mysql_query(conn, query) != 0) {
                            fprintf(stderr, "mysql_query() fallita\n");
                        }
                    }

                    MYSQL_RES *res2 = mysql_store_result(conn);
                    if (res2 == NULL) {
                        fprintf(stderr, "mysql_store_result() fallita\n");
                    }
                    else {
                        /**
                         * Inviamo allo studente il numero di righe risultanti dalla query, in modo che lo studente
                         * possa visualizzare correttamente tutti i campi degli appelli disponibili.
                         */
                        unsigned int num_rows = mysql_num_rows(res2);
                        write(client_sockets[i].connfd, &num_rows, sizeof(num_rows));

                        int id;
                        char exam_name[255];
                        char exam_date[12];

                        /**
                         * Inviamo allo studente tutti i campi di tutte le righe risultanti dalla query.
                         */
                        MYSQL_ROW row;
                        while ((row = mysql_fetch_row(res2))) {
                            sscanf(row[0], "%d", &id);
                            sscanf(row[1], "%[^\n]", exam_name);
                            sscanf(row[2], "%s", exam_date);
                            write(client_sockets[i].connfd, &id, sizeof(id));
                            write(client_sockets[i].connfd, exam_name, sizeof(exam_name));
                            write(client_sockets[i].connfd, exam_date, sizeof(exam_date));
                        }
                    }

                    mysql_free_result(res2);
                }
                /**
                 * Se la scelta è 2 significa che lo studente vuole prenotare un appello di un esame.
                 */
                else if (behaviour == 2) {
                    int cod, mat;
                    char res[255] = {0};

                    /**
                     * La prenotazione deve avvenire tramite il server universitario, quindi inoltriamo la richiesta
                     * dello studente al server universitario.
                     */
                    write(sockfd, &behaviour, sizeof(behaviour));

                    /**
                     * La segreteria riceve dallo studente l'id (cod) dell'appello che vuole prenotare e la sua matricola.
                     */
                    read(client_sockets[i].connfd, &cod, sizeof(cod));
                    read(client_sockets[i].connfd, &mat, sizeof(mat));

                    /**
                     * Inoltra i dati appena ricevuti dallo studente al server universitario.
                     */
                    write(sockfd, &cod, sizeof(cod));
                    write(sockfd, &mat, sizeof(mat));

                    /**
                     * Riceve dal server universitario l'esito dell'operazione.
                     */
                    read(sockfd, res, sizeof(res));

                    /**
                     * L'esito viene inoltrato allo studente.
                     */
                    write(client_sockets[i].connfd, res, sizeof(res));

                    /**
                     * Nel caso in cui la prenotazione sia andata a buon fine, viene inviato allo studente anche il
                     * numero di prenotazione, dopo averlo letto dal server universitario.
                     */
                    if (strcmp(res, "inserimento della nuova prenotazione completato con successo!") == 0) {
                        int count;
                        read(sockfd, &count, sizeof(count));
                        write(client_sockets[i].connfd, &count, sizeof(count));
                    }
                }
                /**
                 * Se la scelta dello studente era relativa ad un operazione di logout, viene chiusa la socket riguardante
                 * le richieste di quello studente, viene azzerata la matricola e viene eliminato il descrittore
                 * di questa socket dall' "insieme" dei descrittori master_set.
                 */
                else if (behaviour == 3) {
                    close(client_sockets[i].connfd);
                    client_sockets[i].matricola = 0;
                    FD_CLR(client_sockets[i].connfd, &master_set);
                    mysql_close(conn);
                }
            }
        }

        /**
         * Controllo se la segreteria è pronta in scrittura.
         */
        if (FD_ISSET(sockfd, &write_set)) {
            /**
             * All'interno richiedo alla segreteria se vuole continuare a gestire le richieste degli studenti oppure
             * vuole procedere all'inserimento di un nuovo appello, prima di soddisfare le richieste degli studenti
             * in attesa.
             */
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

            /**
             * Nel caso in cui si voglia aggiungere un nuovo appello, si procede alla comunicazione con il server
             * universitario.
             */
            if (logical == 2) {
                int request = 1;
                /**
                 * Si invia la richiesta al server universitario.
                 */
                write(sockfd, &request, sizeof(request));

                printf("Inserire il nome dell'esame di cui si vuole aggiungere un appello:\n");
                fgets(name, sizeof(name), stdin);
                name[strlen(name) - 1] = 0;

                printf("\n");

                /**
                 * Si invia il nome dell'esame di cui si vuole aggiungere un appello.
                 */
                write(sockfd, name, strlen(name));

                printf("Inserire la data del nuovo appello (in formato YYYY-MM-DD):\n");
                fgets(date, sizeof(date), stdin);
                date[strlen(date) - 1] = 0;

                /**
                 * Si invia la data del nuovo appello.
                 */
                write(sockfd, date, strlen(date));

                /**
                 * Dal server universitario si riceve l'esito dell'operazione.
                 */
                read(sockfd, result, sizeof(result));

                printf("\nEsito: %s\n\n", result);
            }
        }
    }
}
