#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <mysql/mysql.h>

void addExam(int);
void addBooking(int);

int main() {
    int listenfd, connfd;
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
     * Mettiamo il server in ascolto, specificando quante connessioni possono essere in attesa venire accettate
     * tramite il secondo argomento della chiamata.
     */
    if ((listen(listenfd, 1024)) < 0) {
        perror("Errore nell'operazione di listen!");
        exit(1);
    }

    /**
     * La system call accept permette di accettare una nuova connessione (lato server) in entrata da un client.
     */
    if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
        perror("Errore nell'operazione di accept!");
        exit(1);
    }

    close(listenfd);

    /**
     * Imposto un ciclo while "infinito" in modo che il server possa servire una nuova connessione,
     * quindi un nuovo client, dopo averne terminata una (di connessione client).
     */
    while(1) {
        /**
         * Recupero il tipo di richiesta inviata dalla segreteria, in modo da attivare un'operazione piuttosto
         * che un'altra.
         */
        read(connfd, &request, sizeof(request));

        if (request == 1) {
            addExam(connfd);
        } else if (request == 2) {
            addBooking(connfd);
        }
    }
}

/**
 * Procedura per l'aggiunta di un appello all'interno del database a partire dal nome dell'esame e dalla data passati
 * dalla segreteria.
 * @param connfd socket di connessione con la segreteria (client)
 */
void addExam(int connfd) {
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

    /**
     * Componiamo una stringa che fungerà da query di inserimento nella tabella appello.
     */
    char query[500];

    snprintf(query, sizeof(query), "INSERT INTO appello (nome, data_appello) VALUES ('%s', (STR_TO_DATE('%s', '%%Y-%%m-%%d')))",
             name, date);

    /**
     * Eseguo una query di inserimento a partire dall'oggetto di connessione precedentemente inizializzato
     * e dalla stringa appena definita.
     */
    if (mysql_query(conn, query) != 0) {
        fprintf(stderr, "mysql_query() fallita: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    mysql_close(conn);

    const char *ins = "inserimento del nuovo appello completato con successo!";

    write(connfd, ins, strlen(ins));

}

/**
 * Procedura per l'aggiunta di una prenotazione ad un appello da parte di uno studente all'interno del database,
 * a partire dall'id dell'appello, dalla matricola dello studente che si sta prenotando e dalla data nella quale avviene
 * la prenotazione.
 * @param connfd socket di connessione con la segreteria (client)
 */
void addBooking(int connfd) {
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
        fprintf(stderr, "mysql_query() fallita: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    /**
     * Componiamo una stringa che fungerà da query di inserimento nella tabella prenotazione.
     */
    char q[500];

    snprintf(q, sizeof(q), "SELECT * FROM prenotazione WHERE idAppello = %d AND matricola = %d",
             id, mat);

    /**
     * Eseguo una query di inserimento a partire dall'oggetto di connessione precedentemente inizializzato
     * e dalla stringa appena definita.
     */
    if (mysql_query(conn, q) != 0) {
        fprintf(stderr, "mysql_query() fallita: %s\n", mysql_error(conn));
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

    if (num_rows != 1) {
        char ins[255] = "inserimento della nuova prenotazione non completato!";
        write(connfd, ins, strlen(ins));
    }
    else {
        char ins[255] = "inserimento della nuova prenotazione completato con successo!";
        write(connfd, ins, strlen(ins));
    }

    mysql_close(conn);

}