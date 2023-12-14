# Traccia - Università
Scrivere un'applicazione client/server parallelo per gestire gli esami universitari

## Gruppo 1 studente

### Segreteria:

- Inserisce gli esami sul server dell'università (salvare in un file o conservare in memoria il dato)

- Inoltra la richiesta di prenotazione degli studenti al server universitario

- Fornisce allo studente le date degli esami disponibili per l'esame scelto dallo studente

### Studente:

- Chiede alla segreteria se ci siano esami disponibili per un corso

- Invia una richiesta di prenotazione di un esame alla segreteria

### Server universitario:

- Riceve l'aggiunta di nuovi esami

- Riceve la prenotazione di un esame

## Gruppo 2 studenti

Il server universitario ad ogni richiesta di prenotazione invia alla segreteria il numero di prenotazione progressivo assegnato allo studente e la segreteria a sua volta lo inoltra allo studente 

## Gruppo 3 studenti

Se la segreteria non risponde alla richiesta dello studente questo deve ritentare la connessione per 3 volte. Se le richieste continuano a  fallire allora aspetta un tempo random e ritenta.  Simulare un timeout della segreteria in modo da arrivare a testare l'attesa random

# Note di sviluppo

La prova d’esame richiede la progettazione e lo sviluppo della traccia proposta. 

Il progetto deve essere sviluppato secondo le seguenti linee:

- utilizzare un linguaggio di programmazione a scelta (C, Java, Python, etc...)

- utilizzare una piattaforma Unix-like;

- utilizzare le socket;

- inserire sufficienti commenti;

# Consegna progetto

## Documentazione

Lo studente deve presentare la documentazione relativa al progetto. La documentazione deve contenere:

- Descrizione del progetto;

- Descrizione e schema dell'architettura;

- Dettagli implementativi dei client/server;

- Parti rilevanti del codice sviluppato;

- Manuale utente con le istruzioni su compilazione ed esecuzione;

E' possibile redigere la documentazione usando latex o markdown

Per chi usa latex. Si consiglia di utilizzare la piattaforma Overleaf: https://www.overleaf.com/

Per i markdown:

- https://mystmd.org/

- Pagine descrittive usando Jekyll (https://jekyllrb.com/) o Hugo (https://gohugo.io/). Consigliato usare le github pages (https://pages.github.com/)

# Formato consegna

Ogni gruppo deve consegnare tutti i file e la documentazione tramite un servizio git remoto (github, gitlab, ...):

- Creare un repository pubblico!

- Ogni partecipante del gruppo deve essere aggiunto come collaboratore

- Dare nomi significativi ai commit 
 

# Consegna 

Il progetto va consegnato tramite email al seguente indirizzo bd1726bd.studenti.uniparthenope.it@emea.teams.ms

- Obbligatorio inviare l'email dall'account studente (altrimenti non verrà consegnata)

- Inserire Nome, Cognome e Marticola di tutti i membri del gruppo

- Inserire il link al repository github

- Entro una settimana dall'esame
 
# Modalità di esame

L'esame consisterà nella discussione del progetto con possibili domande sulla parte pratica e progettuale e domande di teoria.

I progetti di gruppo devono essere discussi OBBLIGATORIAMENTE da tutti i membri lo stesso giorno.
