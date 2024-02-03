#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <mysql/mysql.h>

void addExam(int);
void addBooking(int);
MYSQL *connection();

int main() {
    int listenfd, connfd = -1;
    int request;
    struct sockaddr_in servaddr;

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
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(1025);

    /**
     * La system call bind permette di assegnare l'indirizzo memorizzato nel campo s_addr della struct sin_addr, che è
     * a sua volta un campo della struct sockaddr_in (servaddr nel nostro caso), al descrittore listenfd.
     */
    if ((bind(listenfd, (struct sockaddr *)&servaddr, sizeof (servaddr))) < 0) {
        perror("Errore nell'operazione di bind!");
        exit(1);
    }

    /**
     * Mettiamo il server in ascolto, specificando quante connessioni possono essere in attesa di venire accettate
     * tramite il secondo argomento della chiamata.
     */
    if ((listen(listenfd, 1024)) < 0) {
        perror("Errore nell'operazione di listen!");
        exit(1);
    }

    /**
     * Ci serviamo di un "insieme" di descrittori per strutturare la successiva select.
     * In particolare abbiamo read_set che mantiene l'insieme dei descrittori in lettura.
     * La variabile max_fd serve a specificare quante posizioni dell'array di descrittori devono essere controllate
     * all'interno della funzione select.
     */
    fd_set read_set;
    int max_fd;
    max_fd = listenfd;

    /**
     * Imposto un ciclo while "infinito" in modo che il server possa servire una nuova connessione,
     * quindi un nuovo client, dopo averne terminata una (di connessione client).
     */
    while(1) {
        /**
         * Ad ogni iterazione reinizializzo a 0 il read_set e vi aggiungo la socket che permette l'ascolto di nuove
         * connessioni da parte della segreteria.
         */
        FD_ZERO(&read_set);
        FD_SET(listenfd, &read_set);

        /**
         * Se il descrittore della socket relativa alla connessione con la segreteria è maggiore di -1 significa che la
         * segreteria è connessa e quindi si aggiunge anche il suo descrittore al read_set.
         */
        if (connfd > -1) {
            FD_SET(connfd, &read_set);
        }

        /**
         * La funzione select restituisce il numero di descrittori pronti.
         */
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            perror("Errore nell'operazione di select!");
        }

        /**
         * Si controlla se sono in attesa di essere accettate nuove connessioni.
         */
        if (FD_ISSET(listenfd, &read_set)) {
            /**
             * La system call accept permette di accettare una nuova connessione (lato server) in entrata da un client.
             */
            if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
                perror("Errore nell'operazione di accept!");
            }

            /**
             * Si ricalcola il numero di posizioni da controllare nella select
             */
            if (connfd > max_fd) {
                max_fd = connfd;
            }
        }

        /**
         * Si controlla se la segreteria vuole inviare una nuova richiesta al server universitario.
         */
        if (FD_ISSET(connfd, &read_set)) {
            /**
             * In caso affermativo si effettua la read, sempre se è possibile effettuarla, ossia se non viene
             * chiusa la segreteria.
             */
            if (read(connfd, &request, sizeof(request)) > 0) {
                /**
                 * Se la richiesta della segreteria è 1 si richiama la funzione di aggiunta di un appello, mentre se
                 * è 2 si richiama la funzione di aggiunta di una prenotazione di uno studente per un determinato
                 * appello.
                 */
                if (request == 1) {
                    addExam(connfd);
                } else if (request == 2) {
                    addBooking(connfd);
                }
            }
        }
    }
}

/**
 * Procedura per l'aggiunta di un appello all'interno del database a partire dal nome dell'esame e dalla data passati
 * dalla segreteria.
 * @param connfd socket di connessione con la segreteria (client)
 */
void addExam(int connfd) {
    MYSQL *conn = connection();

    char name[255] = {0};
    char date[12] = {0};

    /**
     * Leggiamo dalla socket di connessione con la segreteria il nome dell'esame di cui si vuole aggiungere un appello.
     */
    read(connfd, name, sizeof(name));

    /**
     * Leggiamo dalla socket di connessione con la segreteria la date del nuovo appello.
     */
    read(connfd, date, sizeof(date));

    /**
     * Componiamo una stringa che fungerà da query di inserimento nella tabella appello.
     */
    char query[500];

    snprintf(query, sizeof(query), "INSERT INTO appello (nome, data_appello) VALUES ('%s', (STR_TO_DATE('%s', '%%Y-%%m-%%d')))",
             name, date);

    /**
     * Eseguo una query di inserimento a partire dall'oggetto di connessione precedentemente inizializzato
     * e dalla stringa appena definita. La query può fallire solo se fallisce il vincolo di integrità referenziale
     * legato alla foreign key per quanto riguarda il nome dell'esame di cui si vuole aggiungere l'appello,
     * di conseguenza gestiamo questa possibilità.
     */
    if (mysql_query(conn, query) != 0) {
        if (strstr(mysql_error(conn), "foreign key constraint fails")) {
            const char *err = "non esiste un esame con questo nome!";
            write(connfd, err, strlen(err));
        }
        else {
            const char *err = mysql_error(conn);
            write(connfd, err, strlen(err));
        }
    }
    else {
        const char *ins = "inserimento del nuovo appello completato con successo!";
        /**
         * Inviamo l'esito dell'operazione di aggiunta di un appello alla segreteria.
         */
        write(connfd, ins, strlen(ins));
    }

    mysql_close(conn);
}

/**
 * Procedura per l'aggiunta di una prenotazione ad un appello da parte di uno studente all'interno del database,
 * a partire dall'id dell'appello, dalla matricola dello studente che si sta prenotando e dalla data nella quale avviene
 * la prenotazione.
 * @param connfd socket di connessione con la segreteria (client)
 */
void addBooking(int connfd) {
    MYSQL *conn = connection();

    int id, mat;

    /**
     * Leggiamo dalla socket di connessione con la segreteria l'id dell'appello al quale lo studente si vuole prenotare.
     */
    read(connfd, &id, sizeof(id));

    /**
     * Leggiamo dalla socket di connessione con la segreteria la matricola dello studente che si vuole prenotare
     * all'appello.
     */
    read(connfd, &mat, sizeof(mat));

    char cds[500];
    char pds[500];

    snprintf(cds, sizeof(cds), "SELECT e.cds FROM appello a join esame e on a.nome = e.nome WHERE id = %d", id);

    if (mysql_query(conn, cds) != 0) {
        fprintf(stderr, "mysql_query(cds) fallita: %s", mysql_error(conn));
    }

    MYSQL_RES *res_cds = mysql_store_result(conn);
    if (res_cds == NULL) {
        fprintf(stderr, "mysql_store_result(cds) fallita: %s", mysql_error(conn));
    }

    unsigned int rows = mysql_num_rows(res_cds);
    /**
     * Se non ci sono righe risultanti dalla query soprastante significa che non esiste un appello con questo id
     * e quindi non si procede all'inserimento della nuova prenotazione, altrimenti si continua con le operazioni
     * successive.
     */
    if (!rows) {
        const char *err = "non esiste un appello con questo id!";
        write(connfd, err, strlen(err));
    }
    else {
        MYSQL_ROW row_cds = mysql_fetch_row(res_cds);
        char cds_name[255];
        sscanf(row_cds[0],"%[^\n]", cds_name);

        mysql_free_result(res_cds);

        snprintf(pds, sizeof(pds), "SELECT piano_studi FROM studente WHERE matricola = %d", mat);

        if (mysql_query(conn, pds) != 0) {
            fprintf(stderr, "mysql_query(pds) fallita: %s", mysql_error(conn));
        }

        MYSQL_RES *res_pds = mysql_store_result(conn);
        if (res_pds == NULL) {
            fprintf(stderr, "mysql_store_result(pds) fallita: %s", mysql_error(conn));
        }

        MYSQL_ROW row_pds = mysql_fetch_row(res_pds);
        char pds_name[255];
        sscanf(row_pds[0],"%[^\n]", pds_name);

        mysql_free_result(res_pds);
        /**
         * Controlliamo che l'appello al quale lo studente vuole iscriversi appartenga ad un esame che fa parte del corso
         * di studi a cui lo stesso studente è iscritto.
         */
        if (strcmp(cds_name, pds_name) != 0) {
            const char *err = "non esiste un esame con questo nome nel tuo corso di studi!";
            write(connfd, err, strlen(err));
        }
        else {
            /**
            * Componiamo una stringa che fungerà da query di inserimento nella tabella prenotazione.
            */
            char query[500];

            snprintf(query, sizeof(query), "INSERT INTO prenotazione (idAppello, matricola, data_prenotazione) VALUES ('%d', '%d', NOW())",
                     id, mat);

            /**
             * Eseguo una query di inserimento a partire dall'oggetto di connessione precedentemente inizializzato
             * e dalla stringa appena definita.
             */
            if (mysql_query(conn, query) != 0) {
                if (strstr(mysql_error(conn), "Duplicate entry")) {
                    const char *err = "sei gia' prenotato per questo appello!";
                    write(connfd, err, strlen(err));
                }
                else {
                    const char *err = mysql_error(conn);
                    write(connfd, err, strlen(err));
                }
            }
            else {
                const char *ins = "inserimento della nuova prenotazione completato con successo!";
                write(connfd, ins, strlen(ins));

                char q3[255];

                snprintf(q3, sizeof(q3), "SELECT COUNT(*) FROM prenotazione where idAppello = %d", id);

                /**
                 * Eseguo una query a partire dall'oggetto di connessione precedentemente inizializzato
                 * e dalla stringa appena definita.
                 */
                if (mysql_query(conn, q3) != 0) {
                    fprintf(stderr, "mysql_query() fallita: %s\n", mysql_error(conn));
                }

                MYSQL_RES *res2 = mysql_store_result(conn);
                if (res2 == NULL) {
                    fprintf(stderr, "mysql_store_result() fallita\n");
                }

                /**
                 * Viene inviato il numero di prenotazione alla segreteria, che procederà ad inoltrarlo allo studente,
                 * nel caso in cui venga completata con successo l'operazione di prenotazione.
                 */
                int count;
                MYSQL_ROW row = mysql_fetch_row(res2);
                sscanf(row[0], "%d", &count);

                write(connfd, &count, sizeof(count));

                mysql_free_result(res2);

            }
        }
    }

    fflush(stdin);

    mysql_close(conn);

}

MYSQL *connection() {
    /**
     * Inizializzazione della connessione MySQL, con la funzione init che restituisce un puntatore a questa struttura.
     */
    MYSQL *conn = mysql_init(NULL);

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

    return conn;
}