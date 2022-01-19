/*
 * client.h
 *
 *  Created on: 22 set 2021
 *      Author: francesco
 */

#ifndef CLIENT_H_
#define CLIENT_H_



#endif /* CLIENT_H_ */
#include <netinet/in.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>



#define CLTINF_USERNAME_STRLEN   10
#define CLTINF_PASSWORD_STRLEN   10

#define CLTINF_GUEST              1
#define CLTINF_LOGGED             3

struct ClientInfo{

    pthread_t tidHandler;
    int clientSocket;
    char clientAddressIPv4[INET_ADDRSTRLEN];
    char username[CLTINF_USERNAME_STRLEN];
    time_t timestamp;
    int status;
    double latitude;
    double longitude;
    char lastContact[CLTINF_USERNAME_STRLEN];
    int cycles;
    bool infected;
    pthread_mutex_t mutexSocket;

    //lista doppiamente puntata
    struct ClientInfo* prevClientInfo;
    struct ClientInfo* nextClientInfo;
  };
  typedef struct ClientInfo ClientInfo;
  typedef ClientInfo* LpClientInfo;





  /**
    * @param tidHandler: TID del thread principale che gestisce un determinato client.
    * @param clientSocket: Socket descriptor della socket associata ad un determinato client.
    * @param clientAddressIPv4: IPv4 del client.
    * @param username: Username del client.
    *
    * @return newClientInfo: Puntatore al nodo clientInfo allocato.
    *
    * La funzione newClientInfo alloca un nuovo nodo clientInfo con gli attributi passati
    * in ingresso.
    */

  LpClientInfo newClientInfo(int, char[], char[]);



  /**
    * @param listClientInfo: Doppio puntatore alla lista doppiamente concatenata di clientInfo.
    * @param newClientInfo: Nodo clientInfo da inserire.
    *
    * @return void.
    *
    * La funzione insertClientInfo inserisce un nuovo nodo clientInfo in testa alla lista
    * listClientInfo.
    */

  void insertClientInfo(LpClientInfo*, LpClientInfo);



  /**
    * @param targetedClientInfo: Doppio puntatore al nodo clientInfo da eliminare.
    *
    * @return void.
    *
    * La funzione deleteClientInfo elimina il nodo specificato da targetedClientInfo dalla
    * lista in cui risiede lo stesso.
    */

  void deleteClientInfo(LpClientInfo*, LpClientInfo*);
