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
    int c;
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

    /**
     * Fase di login con inserimento delle credenziali richieste, che saranno successivamente passate alla segreteria
     * che si occuperà del controllo dell'esistenza delle credenziali inserite all'interno della tabella apposita
     * nel database.
     */
    printf("LOGIN\n");
    printf("Inserire matricola: ");
    scanf("%d", &mat);

    /**
     * Pulisco il buffer di input.
     */
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

    if(strcmp(state, "credenziali non corrette, accesso negato!") == 0) {
        exit(1);
    }

    /**
     * Itero le operazioni che lo studente può decidere di effettuare. In base alla scelta effettuata verranno svolte
     * operazioni diverse.
     */
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

        /**
         * Invio la scelta effettuata alla segreteria.
         */
        write(sockfd, &request, sizeof(request));

        /**
         * Se la scelta è 1 significa che lo studente vuole visualizzare gli appelli disponibili, potendo scegliere tra
         * il visualizzarli tutti oppure visualizzare solo gli appelli per un determinato esame.
         */
        if (request == 1) {
            int id;
            char name[255] = {0};
            char date[12] = {0};
            int num_rows;

            printf("Vuoi visualizzare tutti gli appelli o solo quelli relativi ad un determinato esame?");
            printf("\n1 - Tutti gli appelli");
            printf("\n2 - Esame specifico");
            printf("\nScelta: ");
            scanf("%d", &req);

            /**
             * Inviamo la nuova scelta alla segreteria.
             */
            write(sockfd, &req, sizeof(req));

            /**
             * Pulisco il buffer di input.
             */
            while ((c = getchar()) != '\n' && c != EOF);

            /**
             * Nel caso in cui la scelta sia 1 non c'è bisogno di inviare ulteriori informazioni, mentre se la scelta
             * ricade su un esame specifico bisogna inviare alla segreteria anche il nome dell'esame.
             */
            if (req == 2) {
                char exam[255] = {0};
                printf("\nInserisci nome esame: ");
                fgets(exam, sizeof(exam), stdin);
                exam[strlen(exam) - 1] = 0;

                write(sockfd, exam, strlen(exam));
            }

            /**
             * Lo studente riceve dalla segreteria il numero di appelli disponibili, in modo che se non ne esistono
             * viene visualizzato un messaggio di errore, mentre se ce n'è almeno 1 vengono letti i vari campi relativi
             * al risultato della query.
             */
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
        /**
         * Se invece lo studente vuole prenotare un determinato appello deve inviare il codice dell'appello al quale
         * vuole prenotarsi e la sua matricola, che in realtà viene inviata automaticamente a partire da quella inserita
         * durante la fase di login.
         */
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

            /**
             * Lo studente riceve dalla segreteria l'esito dell'operazione e se la prenotazione è stata inserita con
             * successo lo studente vedrà anche il numero della sua prenotazione.
             */
            read(sockfd, res, sizeof(res));

            printf("\nEsito operazione: %s\n", res);

            if (strcmp(res, "inserimento della nuova prenotazione completato con successo!") == 0) {
                int count;
                read(sockfd, &count, sizeof(count));

                printf("Numero prenotazione: %d", count);
                printf("\n");
            }
        }
        /**
         * Se la scelta è 3 significa che lo studente vuole effettuare un'operazione di logout, di conseguenza si procede
         * alla disconnessione.
         */
        else if (request == 3) {
            printf("Logout effettuato correttamente!\n");
            close(sockfd);
            exit(1);
        }
    }
}
