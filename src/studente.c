#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>

int main (int argc, char **argv) {
    int sockfd;
    int request, req;
    struct sockaddr_in servaddr;
    int mat;
    char pass[255] = {0};

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
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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
    servaddr.sin_port = htons(1026);

    /**
         * La system call connect permette di connettere la socket al server specificato nella struct "servaddr" tramite
         * l'indirizzo IP e la porta memorizzate nella struttura.
         */
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("Errore nell'operazione di connect!");
        exit(1);
    }

    printf("LOGIN\n");
    printf("Inserire matricola: ");
    scanf("%d", &mat);

    /**
     * Pulisco il buffer di input.
     */
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Inserire password: ");
    fgets(pass, sizeof(pass), stdin);
    pass[strlen(pass) - 1] = 0;

    write(sockfd, &mat, sizeof(mat));
    write(sockfd, pass, sizeof(pass));

    char state[255] = {0};
    read(sockfd, state, sizeof(state));

    printf("\nEsito login: %s", state);
    printf("\n");

    if (strcmp(state, "credenziali non corrette, accesso negato!") == 0) {
        exit(1);
    }

    while (1) {
        printf("\nInserire il numero relativo all'operazione che si vuole effettuare:\n");
        printf("1 - Visualizza appelli disponibili\n");
        printf("2 - Prenota un appello\n");
        printf("3 - Logout\n");
        printf("Scelta: ");
        scanf("%d", &request);
        printf("\n");

        /**
         * Pulisco il buffer di input.
         */
        while ((c = getchar()) != '\n' && c != EOF);

        write(sockfd, &request, sizeof(request));

        if (request == 1) {
            int id;
            char name[255] = {0};
            char date[12] = {0};
            int num_rows;

            printf("\nVuoi visualizzare tutti gli appelli o solo quelli relativi ad un determinato esame?");
            printf("\n1 - Tutti gli appelli");
            printf("\n2 - Esame specifico");
            printf("\nScelta: ");
            scanf("%d", &req);

            write(sockfd, &req, sizeof(req));

            /**
             * Pulisco il buffer di input.
             */
            while ((c = getchar()) != '\n' && c != EOF);

            if (req == 2) {
                char exam[255] = {0};
                printf("\nInserisci nome esame: ");
                fgets(exam, sizeof(exam), stdin);
                exam[strlen(exam) - 1] = 0;

                write(sockfd, exam, strlen(exam));
            }

            read(sockfd, &num_rows, sizeof(num_rows));

            if (num_rows == 0) {
                printf("\nNon esistono appelli disponibili!\n");
            }
            else {
                printf("\nAppelli disponibili:\n");
                for (int i = 0; i < num_rows; i++) {
                    read(sockfd, &id, sizeof(id));
                    read(sockfd, name, sizeof(name));
                    read(sockfd, date, sizeof(date));

                    printf("%d\t%s\t%s\n", id, name, date);
                }
            }
        }
        else if (request == 2) {
            int cod;

            printf("Digita il codice dell'appello al quale vuoi prenotarti: ");
            scanf("%d", &cod);

            /**
             * Pulisco il buffer di input.
             */
            while ((c = getchar()) != '\n' && c != EOF);

            write(sockfd, &cod, sizeof(cod));
            write(sockfd, &mat, sizeof(mat));

            char res[255] = {0};

            read(sockfd, res, sizeof(res));

            printf("\nEsito operazione: %s", res);
            printf("\n");
        }
        else if (request == 3) {
            printf("Logout effettuato correttamente!\n");
            exit(1);
        }
    }
}
