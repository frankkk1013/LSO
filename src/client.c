/*
 * client.c
 *
 *  Created on: 22 set 2021
 *      Author: francesco
 */



  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <sys/types.h>
  #include "client.h"


   //viene creato un nodo ClientInfo con le informazioni fornite dal client

   LpClientInfo newClientInfo(int clientSocket, char clientAddressIPv4[], char username[]){
     LpClientInfo newClientInfo = (LpClientInfo)malloc(sizeof(ClientInfo));
     if(newClientInfo != NULL){
       newClientInfo->clientSocket = clientSocket;
       strcpy(newClientInfo->clientAddressIPv4, clientAddressIPv4);
       strcpy(newClientInfo->username, username);
       newClientInfo->timestamp = time(NULL);
       newClientInfo->status = CLTINF_GUEST;
       newClientInfo->havePacket = -1;
       newClientInfo->cycles = 0;
       newClientInfo->infected = false;
       newClientInfo->prevClientInfo = NULL;
       newClientInfo->nextClientInfo = NULL;
       pthread_mutex_init(&(newClientInfo->mutexSocket), NULL);
     }
     return newClientInfo;
   }



   //inserisce un nuovo nodo ClientInfo dalla testa della lista

   void insertClientInfo(LpClientInfo* listClientInfo, LpClientInfo newClientInfo){
     if(newClientInfo != NULL){
       newClientInfo->nextClientInfo = *listClientInfo;
       if(*listClientInfo != NULL){
         newClientInfo->prevClientInfo = (*listClientInfo)->prevClientInfo;
         (*listClientInfo)->prevClientInfo = newClientInfo;
         if(newClientInfo->prevClientInfo != NULL){
           newClientInfo->prevClientInfo->nextClientInfo = newClientInfo;
         }
       }
       *listClientInfo = newClientInfo;
     }
   }



   //elimina un nodo ClientInfo specificato in ingresso dalla lista

   void deleteClientInfo(LpClientInfo* listClientInfo, LpClientInfo* targetedClientInfo){
     if(targetedClientInfo != NULL){
       LpClientInfo tmp = *targetedClientInfo;
       *targetedClientInfo = (*targetedClientInfo)->nextClientInfo;
       if(tmp->prevClientInfo != NULL){
         tmp->prevClientInfo->nextClientInfo = *targetedClientInfo;
       }
       if(*targetedClientInfo != NULL){
         (*targetedClientInfo)->prevClientInfo = tmp->prevClientInfo;
       }
       if(*listClientInfo == tmp){
         *listClientInfo = *targetedClientInfo;
       }
       free(tmp);
     }
   }
