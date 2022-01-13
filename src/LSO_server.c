/*
 ============================================================================
 Name        : LSO_server.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#define BUFFER_STRLEN 150
#define LISTENER_QUEUE_STRLEN 50
#define INCOMING_MSG_STRLEN 70
#define OUTCOMING_MSG_STRLEN 82
#define GRAPHICS_CHAT_WIDTH 82
#define DISTANCE 10

#define pi 3.14159265358979323846

#include <math.h>

#include <stdio.h>

#include <stdlib.h>

#include <sys/socket.h>

#include <sys/types.h>

#include <netinet/in.h>

#include <errno.h>

#include <arpa/inet.h>

#include <pthread.h>

#include <stdbool.h>

#include <string.h>

#include <signal.h>

#include <unistd.h>

#include <time.h>

#include <fcntl.h>

#include <ctype.h>

#include <stdbool.h>

#include "client.h"

//typedef enum { false = 0, true = 1 }bool;

pthread_mutex_t mutexClientInfo = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexDatabase = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexLogs = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCursor = PTHREAD_MUTEX_INITIALIZER;

LpClientInfo listClientInfo = NULL;
int totalConnections = 0;
int connectedClients = 0;
int totalInfections = 0;
int listenerSocket;
int listenerPort;
int logs;
int database;

int createSocket();
void * listenerClient(void * arg);
void * connectionRequestsManagement(void * arg);
bool signIn(LpClientInfo clientInfo);
bool login(LpClientInfo clientInfo);
int initFile(void);
void sendMsg(LpClientInfo clientInfo, char * outcomingMsg);
void sendMsgToAll(LpClientInfo, char * );
void signalHandler(int);
double deg2rad(double);
double rad2deg(double);
double distance(double, double, double, double);
void checkInfections();
bool requestNear(LpClientInfo);

bool updateGpsInfo(LpClientInfo);

int main() {

  pthread_t tidConnectionRequestsManagement;
  srand(time(NULL));

  signal(SIGPIPE, SIG_IGN);
  signal(SIGALRM, SIG_IGN);

  if (initFile()) {

    printf("\n <!> Errore durante l'inizializzazione del Server 1.\n\n");

    return 1;
  }

  if (createSocket()) {
    return 1;
  }

  signal(SIGSTOP, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGALRM, checkInfections);
  printf("In ascolto ... \n\n\n");

  //handler sigstop e sigint
  pthread_create( & tidConnectionRequestsManagement,
    NULL, connectionRequestsManagement, NULL);
  printf("In ascolto ... w\n\n\n");

  alarm(20); //20 secondi chiama la funzione checkInfections()
  while (true) {

  }

  return 0;

}

int createSocket() {

  int bytesReaded;
  char buffer[BUFFER_STRLEN];
  int port;
  bool error;
  struct sockaddr_in serverAddress;
  memset( & serverAddress, '0', sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = inet_addr("192.168.1.24");

  //inserire porta
  printf("\n » Inserire la porta del server: ");
  fflush(stdout);

  do {

    error = false;
    if ((bytesReaded = read(STDIN_FILENO, buffer, BUFFER_STRLEN)) == -1) {
      perror("\n    <!> Errore read");

      return 1;

    }
    buffer[bytesReaded] = '\0';

    /* Verifico se la porta fornita è valida */
    for (int i = 0; i < strlen(buffer) - 1; i++) {
      if (buffer[i] < '0' || buffer[i] > '9') {
        error = true;
        break;
      }
    }

    if (error) {

      printf("\n   <!> Porta non valida, caratteri non ammessi - riprovare: ");

      fflush(stdout);
    } else {
      if ((port = atoi(buffer)) == 0 || !(port >= 0 && port <= 65535)) {

        printf("\n   <!> Porta non valida, out of range - riprovare: ");

        fflush(stdout);
        error = true;
      } else {
        listenerPort = port;

        printf("\n    » Porta %d inserita con successo.\n", port);

        fflush(stdout);
        serverAddress.sin_port = htons(atoi(buffer));
      }

      if (!error) {
        listenerSocket = socket(PF_INET, SOCK_STREAM, 0);
        /* Assegno un'indirizzo alla socket del server */
        if (bind(listenerSocket, (struct sockaddr * ) & serverAddress, sizeof(serverAddress)) == -1) {

          perror("\n <!> Errore bind");
          puts("");

          sleep(1);
          error = true;
        }
        printf("bind riuscito");

        /* Rimango in ascolto di richieste di connessione da parte di client */
        if (listen(listenerSocket, LISTENER_QUEUE_STRLEN) == -1) {

          perror("\n <!> Errore listen");
          puts("");

          sleep(1);
          return 1;
        }

      }
    }
  } while (error != false);

  return 0;

}

void disconnectionManagement(LpClientInfo clientInfo) {
  time_t timestamp = time(NULL);
  char logsBuffer[BUFFER_STRLEN];

  pthread_mutex_lock( & mutexLogs);
  sprintf(logsBuffer, " > Il client %s si è disconnesso dal Server - %s", clientInfo -> clientAddressIPv4, ctime( & timestamp));

  if (write(logs, logsBuffer, strlen(logsBuffer)) == -1) {
    printf("errore nella scirttura dei log");

  }
  pthread_mutex_unlock( & mutexLogs);

  pthread_mutex_unlock( & mutexCursor);
  connectedClients -= 1;
  pthread_mutex_lock( & mutexCursor);
  pthread_mutex_lock( & mutexClientInfo);
  /* Chiudo la socket di comunicazione */
  close(clientInfo -> clientSocket);
  /* Elimino dalla lista dei client , il client disconnesso */
  deleteClientInfo( & listClientInfo, & clientInfo);
  pthread_mutex_unlock( & mutexClientInfo);

}

void * listenerClient(void * arg) {

  LpClientInfo clientInfo = (LpClientInfo) arg;
  char incomingMsg[INCOMING_MSG_STRLEN]; /* Buffer messaggio in entrata                */
  int bytesReaded; /* Numero di bytes letti dalla read           */

  //char msg[GRAPHICS_CHAT_WIDTH];

  bool exited = false;

  /* Invio il messaggio di benvenuto */
  sendMsg(clientInfo, "$Server: Benvenuto/a! Inserisci i comandi.");
  memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);

  while (exited != true && (bytesReaded = read(clientInfo -> clientSocket, incomingMsg, INCOMING_MSG_STRLEN)) > 0) {
    incomingMsg[bytesReaded] = '\0';

    switch (clientInfo -> status) {
    case CLTINF_GUEST:

      if (strncmp(incomingMsg, "signin", 6) == 0) {

        exited = signIn(clientInfo);
        if (exited) {
          exited = false;

        }

      } else if (strncmp(incomingMsg, "login", 5) == 0) {
        exited = login(clientInfo);
        if (exited) {
          exited = false;
        }
      } else if (strncmp(incomingMsg, "exit", 4) == 0) {
        exited = true;
      } else {
        sendMsg(clientInfo, "$Server: Comando non disponibile, riprova!");
      }
      break;

    case CLTINF_LOGGED:

      if (strncmp(incomingMsg, "update", 6) == 0) {

        exited = updateGpsInfo(clientInfo);
        if (exited) {
          exited = false;
        }

      } else if (strncmp(incomingMsg, "list", 4) == 0) {
        exited = requestNear(clientInfo);
        if (exited) {
          exited = false;
        }

      } else if (strncmp(incomingMsg, "exit", 4) == 0) {
        exited = true;

      } else {
        sendMsg(clientInfo, "$Server: Comando non disponibile, riprova!");

      }

      break;

    }
    memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);
  }
  if (exited) {
    sendMsg(clientInfo, "$Server: Torna presto! Disconnessione in corso...");
  }
  disconnectionManagement(clientInfo);
}

void * connectionRequestsManagement(void * arg) {

  int connectSocket;
  pthread_t tidListenerClient;
  struct sockaddr_in clientAddress;
  char logsBuffer[BUFFER_STRLEN];
  LpClientInfo clientInfo;
  char clientAddressIPv4[INET_ADDRSTRLEN];
  int clientAddressSize = sizeof(clientAddress);

  while (true) {

    /* Accetto le richieste di connessione da parte dei client */
    if ((connectSocket = accept(listenerSocket, (struct sockaddr * ) & clientAddress, & clientAddressSize)) == -1) {
      printf(" <!> Impossibile accettare richiesta di connessione");
    } else {
      inet_ntop(AF_INET, & clientAddress, clientAddressIPv4, INET_ADDRSTRLEN);
      clientInfo = NULL;
    }
    /* Creo una struttura clientInfo che conterrà tutte le informazioni riguardo al client */
    if ((clientInfo = newClientInfo(connectSocket, clientAddressIPv4, "")) == NULL) {
      printf(" <!> Impossibile allocare memoria, %s disconnesso.", clientAddressIPv4);

    } else {

      /* Aggiorno le strutture che contengono informazioni sullo status attuale */
      pthread_mutex_lock( & mutexClientInfo);
      insertClientInfo( & listClientInfo, clientInfo);
      pthread_mutex_unlock( & mutexClientInfo);

      /* Creao un thread che gestirà l'accesso del client al Server */
      pthread_create( & tidListenerClient, NULL, listenerClient, clientInfo);
      clientInfo -> tidHandler = tidListenerClient;
      printf("Nuova connessione accettata: [Client: %s] - %s", clientAddressIPv4, ctime( & (clientInfo -> timestamp)));
      totalConnections += 1;
      pthread_mutex_lock( & mutexCursor);

      printf(" \u25cf Total CN: %7.2d", totalConnections);
      pthread_mutex_unlock( & mutexCursor);
      connectedClients += 1;
      pthread_mutex_lock( & mutexCursor);

      printf(" \u25cf Connected: %6.2d", connectedClients);
      pthread_mutex_unlock( & mutexCursor);
      //msg[strlen(msg)-1] = '\0';

      //scrittura file di log

      pthread_mutex_lock( & mutexLogs);
      sprintf(logsBuffer, " > Nuova connessione accettata: [Client: %s] - %s", clientAddressIPv4, ctime( & (clientInfo -> timestamp)));
      if (write(logs, logsBuffer, strlen(logsBuffer)) == -1) {

      }
      pthread_mutex_unlock( & mutexLogs);

    }

  }
}



bool signIn(LpClientInfo clientInfo) {

  char username[CLTINF_USERNAME_STRLEN]; //buffer username
  char password[CLTINF_PASSWORD_STRLEN]; //buffer password
  char incomingMsg[INCOMING_MSG_STRLEN]; //buffer messaggi socket
  char record[GRAPHICS_CHAT_WIDTH];
  char msg[GRAPHICS_CHAT_WIDTH];
  char character[1];
  char * usernameDB;
  bool passed = false;
  int recordIndex; /* Indice di posizione del buffer record      */
  int bytesReaded; /* Numero di bytes letti dalla read           */
  time_t timestamp;
  char logsBuffer[BUFFER_STRLEN];

  memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);
  sendMsg(clientInfo, "$Server: Inserisci l'username - massimo 10 caratteri.");

  do {
    passed = true;
    /* Leggo l'username inserito dal client */
    /* Verifico se l'utente ha inserito il comando di uscita */
    /* Controllo se lo username supera la lunghezza di 10 caratteri stabiliti */
    /* Controllo se nell'username siano presenti caratteri di spazio */
    if ((bytesReaded = read(clientInfo -> clientSocket, incomingMsg, INCOMING_MSG_STRLEN)) <= 0) {
      return false;
    }
    incomingMsg[bytesReaded] = '\0';

    if (!strcmp(incomingMsg, "exit")) {
      return true;
    } else {

      if (strlen(incomingMsg) > CLTINF_USERNAME_STRLEN) {
        sendMsg(clientInfo, "$Server: Attenzione, username troppo lungo - [massimo 10 caratteri], riprovare.");
        passed = false;
      } else {

        for (int i = 0; i < strlen(incomingMsg); i++) {
          if (incomingMsg[i] == ' ') {
            sendMsg(clientInfo, "$Server: Attenzione, gli spazi non sono consentiti, riprovare.");
            passed = false;
            break;
          }
        }
        /* Nel caso in cui l'username fosse ancora valido proseguo con le verifiche */
        if (passed != false) {

          pthread_mutex_lock( & mutexDatabase);
          lseek(database, 0, SEEK_SET);
          memset(record, '\0', GRAPHICS_CHAT_WIDTH);
          recordIndex = 0;
          sendMsg(clientInfo, "1");

          /* Ciclo sul file database per verificare che lo username non esista già */
          /* La lettura avviene un carattere alla volta fintanto non viene trovato un newline oppure \0 */
          do {

            if ((bytesReaded = read(database, character, 1)) > 0) {
              //sendMsg(clientInfo, bytesReaded);
              /* Il carattere è diverso da un newline oppure un fine stringa? */
              if (character[0] != '\n') {

                record[recordIndex++] = character[0];
              } else {

                record[recordIndex] = '\0';
                usernameDB = strtok(record, " ");
                /* Controllo se l'username esista gia nel database, se sì, stampo un errore e riclico il do */
                if (!strcmp(usernameDB, incomingMsg)) {
                  sendMsg(clientInfo, "$Server: Attenzione, username non disponibile, riprovare.");
                  passed = false;
                  break;
                } else {
                  /* Ripulisco le strutture di appoggio */
                  memset(record, '\0', GRAPHICS_CHAT_WIDTH);
                  recordIndex = 0;

                }
              }
            }
          } while (bytesReaded != 0);
          sendMsg(clientInfo, "sonofuori");

          /* Verifico l'eventuale presenza di errori */
          if (bytesReaded == -1) {
            sprintf(msg, " <!> Impossibile leggere dati dal Database.");

            sleep(2);
            /* La terminazione è necessariamente bruta a causa dell'impossibilità di catturare    */
            /* l'exit status del thread chiamante signin (numero di utenti non deterministico).   */
            /* Chiaramente una soluzione sarebbe potuta essere una lista di tid, tuttavia         */
            /* sarebbe stata un'implementazione "inutilmente" costosa, considerando che in certi  */
            /* casi la terminazione è obbligatoria.                                               */
            pthread_kill(pthread_self(), SIGTERM);
          } else if (passed != false) {
            strcpy(username, incomingMsg);
          }
          pthread_mutex_unlock( & mutexDatabase);
        }
      }
      memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);
    }
  } while (passed != true);

  sendMsg(clientInfo, "$Server: Inserisci la password - massimo 10 caratteri.");
  memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);

  do {
    passed = true;
    /* Leggo la password inserita dal client */
    if ((bytesReaded = read(clientInfo -> clientSocket, incomingMsg, INCOMING_MSG_STRLEN)) <= 0) {
      return false;
    }
    incomingMsg[bytesReaded] = '\0';

    /* Verifico se l'utente ha inserito il comando di uscita */
    if (!strcmp(incomingMsg, "exit")) {
      return true;
    } else {
      /* Controllo se la password supera la lunghezza di 10 caratteri stabiliti  */
      if (strlen(incomingMsg) > CLTINF_PASSWORD_STRLEN) {
        sendMsg(clientInfo, "$Server: Attenzione, password troppo lunga - [massimo 10 caratteri], riprovare.");
        passed = false;
      } else {
        /* Controllo che nella password non siano presenti caratteri di spazio */
        for (int i = 0; i < strlen(incomingMsg); i++) {
          if (incomingMsg[i] == ' ') {
            sendMsg(clientInfo, "$Server: Attenzione, gli spazi non sono consentiti, riprovare.");
            passed = false;
            break;
          }
        }
        if (passed != false) {
          strcpy(password, incomingMsg);
        }
      }
      memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);
    }
  } while (passed != true);

  memset(record, '\0', GRAPHICS_CHAT_WIDTH);
  /* Concateno username e password in un solo buffer */
  sprintf(record, "%s %s\n", username, password);

  pthread_mutex_lock( & mutexDatabase);

  /* Scrivo i dati di registazione del client nel database */
  if (write(database, record, strlen(record)) == -1) {
    printf(" <!> Impossibile scrivere dati nel Database.");

    sleep(2);
    /* La terminazione è necessariamente bruta a causa dell'impossibilità di catturare    */
    /* l'exit status del thread chiamante signin (numero di utenti non deterministico)    */
    /* Chiaramente una soluzione sarebbe potuta essere una lista di tid, tuttavia         */
    /* sarebbe stata un'implementazione "inutilmente" costosa, considerando che in certi  */
    /* casi la terminazione è obbligatoria.                                               */
    pthread_kill(pthread_self(), SIGTERM);
  }
  pthread_mutex_unlock( & mutexDatabase);
  /* Registrazione effettuata con successo */
  timestamp = time(NULL);
  pthread_mutex_lock( & mutexLogs);
  sprintf(logsBuffer, " > Registrazione del client %s avvenuta con successo - %s", clientInfo -> clientAddressIPv4, ctime( & (clientInfo -> timestamp)));
  if (write(logs, logsBuffer, strlen(logsBuffer)) == -1) {

  }
  pthread_mutex_unlock( & mutexLogs);
  printf("Registrazione del client %s avvenuta con successo.", clientInfo -> clientAddressIPv4);

  sendMsg(clientInfo, "$Server: Registrazione avvenuta con successo, ora puoi effettuare il login.");
  //clientInfo->status = CLTINF_REGISTERED;

  return false;
}

bool login(LpClientInfo clientInfo) {

  bool passed = false; /* Flag che indica il superamento di una fase */
  char incomingMsg[INCOMING_MSG_STRLEN]; /* Buffer per messaggi in entrata             */
  char record[GRAPHICS_CHAT_WIDTH]; /* Buffer per la lettura di un record del DB  */
  char username[CLTINF_USERNAME_STRLEN]; /* Buffer per username del client             */
  char password[CLTINF_PASSWORD_STRLEN]; /* Buffer per password del client             */
  char incomingRecord[GRAPHICS_CHAT_WIDTH]; /* Username + Password inseriti dall'utente   */
  int recordIndex; /* Indice di posizione del buffer record      */
  char character[1]; /* Array per la lettura dei dati dal DB       */
  int bytesReaded; /* Numero di bytes letti dalla read           */

  bool alreadyLogged;

  char logsBuffer[BUFFER_STRLEN];
  sendMsg(clientInfo, "$Server: Inserisci lo stato");
    memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);

    do {
      passed = true;
      /* Leggo lo stato inserito dal client */
      if ((bytesReaded = read(clientInfo -> clientSocket, incomingMsg, INCOMING_MSG_STRLEN)) <= 0) {
        return false;
      }
      incomingMsg[bytesReaded] = '\0';

      if (!strcmp(incomingMsg, "exit")) {
        return true;
      } else {
        if (!strcmp(incomingMsg, "positive")) {
          clientInfo -> infected = true;

        } else {
          if (!strcmp(incomingMsg, "negative")) {
            clientInfo -> infected = false;

          }

        }

      }
      memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);

    } while (passed != true);

    updateGpsInfo(clientInfo);
  memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);

  do {
    sendMsg(clientInfo, "$Server: Inserisci l'username.");
    memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);

    do {
      passed = true;
      /* Leggo l'username inserito dal client */
      if ((bytesReaded = read(clientInfo -> clientSocket, incomingMsg, INCOMING_MSG_STRLEN)) <= 0) {
        return false;
      }
      incomingMsg[bytesReaded] = '\0';

      if (!strcmp(incomingMsg, "exit")) {
        return true;
      } else {
        if (strlen(incomingMsg) > CLTINF_USERNAME_STRLEN) {
          sendMsg(clientInfo, "$Server: Attenzione, username troppo lungo - [massimo 10 caratteri], riprovare.");
          passed = false;
        } else {
          for (int i = 0; i < strlen(incomingMsg); i++) {
            if (incomingMsg[i] == ' ') {
              sendMsg(clientInfo, "$Server: Attenzione, gli spazi non sono consentiti, riprovare.");
              passed = false;
              break;
            }
          }
          if (passed != false) {
            strcpy(username, incomingMsg);
          }
        }
        memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);
      }
    } while (passed != true);

    sendMsg(clientInfo, "$Server: Inserisci la password. ");
    memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);

    do {
      passed = true;
      /* Leggo la password inserita dal client */
      if ((bytesReaded = read(clientInfo -> clientSocket, incomingMsg, INCOMING_MSG_STRLEN)) <= 0) {
        return false;
      }
      incomingMsg[bytesReaded] = '\0';

      /* Verifico se l'utente ha inserito il comando di uscita */
      if (!strcmp(incomingMsg, "exit")) {
        return true;
      } else {
        /* Controllo se la password supera la lunghezza di 10 caratteri stabiliti  */
        if (strlen(incomingMsg) > CLTINF_PASSWORD_STRLEN) {
          sendMsg(clientInfo, "$Server: Attenzione, password troppo lunga - [massimo 10 caratteri], riprovare.");
          passed = false;
        } else {
          /* Controllo che nella password non siano presenti caratteri di spazio */
          for (int i = 0; i < strlen(incomingMsg); i++) {
            if (incomingMsg[i] == ' ') {
              sendMsg(clientInfo, "$Server: Attenzione, gli spazi non sono consentiti, riprovare.");
              passed = false;
              break;
            }
          }
          if (passed != false) {
            strcpy(password, incomingMsg);
          }
        }
        memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);
      }
    } while (passed != true);

    memset(incomingRecord, '\0', CLTINF_USERNAME_STRLEN * 2);
    sprintf(incomingRecord, "%s %s", username, password);
    pthread_mutex_lock( & mutexDatabase);
    lseek(database, 0, SEEK_SET);
    memset(record, '\0', GRAPHICS_CHAT_WIDTH);
    recordIndex = 0;

    /* Ciclo sul file database per verificare che lo username non esista già */
    do {
      /* La lettura avviene un carattere alla volta fintanto non viene trovato un newline */
      alreadyLogged = false;
      if ((bytesReaded = read(database, character, 1)) > 0) {
        /* Il carattere è diverso da un newline? */
        if (character[0] != '\n') {
          record[recordIndex++] = character[0];
        } else {
          if (!strcmp(record, incomingRecord)) {
            pthread_mutex_lock( & mutexClientInfo);
            LpClientInfo tmp = listClientInfo;
            while (tmp != NULL && alreadyLogged == false) {
              if ((tmp -> status == CLTINF_LOGGED) && !strcmp(tmp -> username, username) && tmp != clientInfo) {
                sendMsg(clientInfo, "$Server: L'utente specificato è già connesso al Server.");
                sleep(1);
                alreadyLogged = true;
              }
              tmp = tmp -> nextClientInfo;
            }
            pthread_mutex_unlock( & mutexClientInfo);

            if (alreadyLogged == false) {
              clientInfo -> status = CLTINF_LOGGED;
              strcpy(clientInfo -> username, username);

              pthread_mutex_lock( & mutexLogs);
              sprintf(logsBuffer, " > Login effettuato con successo: [Client: %s - %s] - %s", clientInfo -> clientAddressIPv4, clientInfo -> username, ctime( & (clientInfo -> timestamp)));
              if (write(logs, logsBuffer, strlen(logsBuffer)) == -1) {
                printf("Errore durante la scrittura nel file di logs.");
              }
              pthread_mutex_unlock( & mutexLogs);
              sendMsg(clientInfo, "$Server: Login effettuato con successo.");
              sleep(1);

            }
            break;
          } else {
            /* Ripulisco le strutture di appoggio */
            memset(record, '\0', GRAPHICS_CHAT_WIDTH);
            recordIndex = 0;
          }
        }
      }
    } while (bytesReaded != 0);
    pthread_mutex_unlock( & mutexDatabase);

    if (clientInfo -> status != CLTINF_LOGGED && alreadyLogged != true) {
      sendMsg(clientInfo, "$Server: Username o password errati, riprovare.");
      sleep(1);
    }

  } while (clientInfo -> status != CLTINF_LOGGED);


  return false;

}

int initFile(void) {

  /* Apro il file di log */
  if ((logs = open("logs.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) == -1) {
    printf("%d", logs);
    return 1;
  }

  /* Apro il file di database */
  if ((database = open("database.txt", O_RDWR | O_CREAT | O_APPEND, S_IRWXU)) == -1) {

    return 1;
  }

  return 0;

}

void sendMsg(LpClientInfo clientInfo, char * outcomingMsg) {

  char buffer[OUTCOMING_MSG_STRLEN];

  //pthread_t tidUpdateChat;

  memset(buffer, '\0', OUTCOMING_MSG_STRLEN);
  strcpy(buffer, outcomingMsg);
  pthread_mutex_lock( & (clientInfo -> mutexSocket));
  write(clientInfo -> clientSocket, buffer, OUTCOMING_MSG_STRLEN);
  pthread_mutex_unlock( & (clientInfo -> mutexSocket));
}

void signalHandler(int signal) {

  LpClientInfo tmp;

  switch (signal) {
  case SIGINT:
    printf("Disconnessione del server in corso!");

    close(listenerSocket);
    close(logs);
    close(database);

    tmp = listClientInfo;
    while (tmp != NULL) {
      close(tmp -> clientSocket);
      tmp = tmp -> nextClientInfo;
    }
    sleep(2);

    raise(SIGTERM);
    break;
  case SIGSTOP:
    printf("Disconnessione del server in corso!");

    close(listenerSocket);
    close(logs);
    close(database);

    tmp = listClientInfo;
    while (tmp != NULL) {
      close(tmp -> clientSocket);
      tmp = tmp -> nextClientInfo;
    }
    sleep(2);

    raise(SIGTERM);
    break;
  }
}

/*********************************DISTANZA*********/
double deg2rad(double deg) {
  return (deg * pi / 180);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts radians to decimal degrees             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double rad2deg(double rad) {
  return (rad * 180 / pi);
}

double distance(double lat1, double lon1, double lat2, double lon2) {
  char unit = 'K';
  double theta, dist;
  if ((lat1 == lat2) && (lon1 == lon2)) {
    return 0;
  } else {
    theta = lon1 - lon2;
    dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
    dist = acos(dist);
    dist = rad2deg(dist);
    dist = dist * 60 * 1.1515;
    switch (unit) {
    case 'M':
      break;
    case 'K':
      dist = dist * 1.609344;
      break;
    case 'N':
      dist = dist * 0.8684;
      break;
    }
    return (dist);
  }
}

//un thread avviato dal main si occuperà di chiamare questa funzione a ogni cilco

void checkInfections() {

  LpClientInfo tmp = listClientInfo;
  struct sockaddr_in clientAddress;
  char logsBuffer[BUFFER_STRLEN];
  time_t current_time = time(NULL);

  LpClientInfo traverse;
  LpClientInfo min;
  double dist;

  pthread_mutex_lock( & mutexClientInfo);

  if(tmp != NULL){

	  while (tmp -> nextClientInfo) {

	      min = tmp;
	      traverse = tmp -> nextClientInfo;
	      while (traverse) {
	        dist = distance(tmp -> latitude, tmp -> longitude,
	          traverse -> latitude, traverse -> longitude);
	        //pthread_mutex_lock( & mutexClientInfo);

	        if (dist < DISTANCE && (tmp -> infected == true || traverse -> infected == true)) { //cicli infezioni
	          if (traverse -> infected == false) {

	            traverse -> cycles = traverse -> cycles + 1;

	          } else {

	            if (tmp -> infected == false) {

	          	tmp -> cycles = tmp -> cycles + 1;
	            }

	          }

	          if (tmp -> cycles == 3 && tmp -> infected == false) {

	            tmp -> infected = true;
	            totalInfections++;

	          }
	          if (traverse -> cycles == 3 && traverse -> infected == false) {

	            traverse -> infected = true;
	            totalInfections++;

	          }
	        }
	        //pthread_mutex_unlock( & mutexClientInfo);
	        if (dist > 10 && (traverse -> cycles < 3 || tmp -> cycles < 3)) {
	          if (tmp -> infected == true && (!strcmp(traverse -> lastContact, tmp -> username)) && traverse -> cycles > 0) {
	           // pthread_mutex_lock( & mutexClientInfo);
	            traverse -> cycles--;
	            //pthread_mutex_unlock( & mutexClientInfo);

	          } else {
	            if (traverse -> infected == true && (!strcmp(tmp -> lastContact, traverse -> username)) && tmp -> cycles > 0) {
	              //pthread_mutex_lock( & mutexClientInfo);
	              tmp -> cycles--;
	              //pthread_mutex_unlock( & mutexClientInfo);

	            }
	          }

	        }
	        traverse = traverse -> nextClientInfo;

	      }
	      tmp = tmp -> nextClientInfo;

	    }
  }

  pthread_mutex_unlock( & mutexClientInfo);
  pthread_mutex_lock( & mutexLogs);
  sprintf(logsBuffer, " totalInfections %d - %s", totalInfections, ctime( & current_time));

  if (write(logs, logsBuffer, strlen(logsBuffer)) == -1) {

  }
  pthread_mutex_unlock( & mutexLogs);
  printf("controllo");
  alarm(20); //ogni 20 secondi chiama la funzione checkInfections()

}

bool requestNear(LpClientInfo clientInfo) {

  LpClientInfo user;
  LpClientInfo current;

  int positive = 0, negative = 0;
  char str[4];

  current = listClientInfo;
  user = clientInfo;

  //invio un carattere di inizializzazione
  sendMsg(clientInfo, "$");

  pthread_mutex_lock( & mutexClientInfo);
  while (current) {

    if (distance(user -> latitude, user -> longitude, current -> latitude, current -> longitude) < DISTANCE) {
      //ad ogni ciclo se un utente è vicino invio il suo username
      sendMsg(clientInfo, current -> username);
      if (current -> infected == true) {
        positive++;

      } else {
        negative++;

      }

    }
    current = current -> nextClientInfo;
  }


  //invio un carattere di fine trasmissione degli username
  sendMsg(clientInfo, "!");
  //invio il numero di positivi e negativi vicini all utente
  sprintf(str, "%d", positive);
  sendMsg(clientInfo, str);
  sprintf(str, "%d", negative);
  sendMsg(clientInfo, str);
  sendMsg(clientInfo, "x");
  //sendMsg(clientInfo, current -> username);

  pthread_mutex_unlock( & mutexClientInfo);

  return true;

}

bool updateGpsInfo(LpClientInfo clientInfo) {

  bool passed;
  char incomingMsg[INCOMING_MSG_STRLEN];
  int bytesReaded;

  memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);
  passed = true;

  sendMsg(clientInfo, "$Server: Inserisci latitudine");
  /* Leggo la latitudine dal client */
  if ((bytesReaded = read(clientInfo -> clientSocket, incomingMsg, INCOMING_MSG_STRLEN)) <= 0) {
    return false;
  }
  incomingMsg[bytesReaded] = '\0';

  pthread_mutex_lock( & mutexClientInfo);
  clientInfo -> latitude = atof(incomingMsg);
  pthread_mutex_unlock( & mutexClientInfo);

  //memset(incomingMsg, '\0', INCOMING_MSG_STRLEN);
  sendMsg(clientInfo, "$Server: Inserisci longitudine");
  /* Leggo la longitudine dal client */
  if ((bytesReaded = read(clientInfo -> clientSocket, incomingMsg, INCOMING_MSG_STRLEN)) <= 0) {
    return false;
  }
  incomingMsg[bytesReaded] = '\0';

  pthread_mutex_lock( & mutexClientInfo);
  clientInfo -> longitude = atof(incomingMsg);
  pthread_mutex_unlock( & mutexClientInfo);

  printf("%f %f", clientInfo -> latitude, clientInfo -> latitude);

  return true;

}
