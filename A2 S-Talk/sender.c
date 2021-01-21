#include "sender.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include<signal.h>
#include<pthread.h> 
#include <stdbool.h>
#include "receiver.h"
#include "list.h"
#include "KBInput.h"
 
static pthread_t senderThreadPID;

//shared
static List* senderPool;
static pthread_cond_t nonSenderEmptyPoolCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t accessSenderPoolMutex = PTHREAD_MUTEX_INITIALIZER;

//net
static int socketFD;
static char* remoteName ;
static char* remotePortChar;
struct addrinfo *remote;

//common 
static char* msg;
void createSender(char* remoteHost , char* remotePort);
void* sendThread();
void releaseItemInSenderPool(void* item);


void createSenderPool(char* remoteHost , char* remotePort, int sock){
    socketFD = sock;
    // pthread_mutex_init(&accessSenderPoolMutex,NULL);
    // pthread_cond_init(&nonSenderEmptyPoolCondVar,NULL);
    senderPool = List_create();
    if(senderPool==NULL){
        printf("senderPool create failed!\n");
        exit(EXIT_FAILURE);
    } 
    createSender(remoteHost,remotePort);
    createKB(senderPool); 
    puts("Sender Pool Ready ..");
    puts("----- Start ------");
}


void createSender(char* remoteHost , char* remotePort){
    remoteName = remoteHost;
    remotePortChar = remotePort;
    // printf("---%s -------------%s-------\n",pack.remoteHost, pack.remotePort);
    // the struct type as void* seems unstable to convert here, better use straightforward attributes.
    if(pthread_create(&senderThreadPID, NULL, sendThread,  NULL)!= 0){
        printf("pthread_create -- sender create failed\n");
        exit(EXIT_FAILURE);
    }
}



void* sendThread(){
    struct addrinfo hs;
    memset(&hs,0,sizeof(hs));
    hs.ai_socktype = SOCK_DGRAM;
    hs.ai_family = AF_INET;
    hs.ai_protocol = IPPROTO_UDP;

    
    if(getaddrinfo(remoteName, remotePortChar, &hs, &remote)!=0){ 
        perror("getaddrinfo failed!  -- sender.c -- ");
        exit(EXIT_FAILURE);
    }
   
    
    // char msg[MSG_MAX_LEN]; DONT USE THIS!
	while(1) { 
        //pthread_testcancel();
        pthread_mutex_lock(&accessSenderPoolMutex);  

        //empty pool -- wait for kb's signal to get node to read
        if(!List_first(senderPool)){
            //printf("WAIT");
            pthread_cond_wait(&nonSenderEmptyPoolCondVar, &accessSenderPoolMutex);
            //it maybe wake up by the SIGNAL
        }  
        if(List_first(senderPool)){
            //remove one item out to send
            msg = (char*)List_remove(senderPool);
        } 

        pthread_mutex_unlock(&accessSenderPoolMutex);
         
        /* if(msg && strlen(msg)>0){
           printf("sending msg = %s",msg); 
        } */

        if(msg && strlen(msg)==2 && (char)msg[0]=='!'){
            puts("--------------------'!' typed! -- terminate this program!--------------------");
            
            //before end my self, I should sent the other sides a 'i' to tell him to end this talk
            int sendStatus = sendto(socketFD, msg, MSG_MAX_LEN,0,remote->ai_addr,remote->ai_addrlen);
            if(sendStatus == -1){       
                printf("Message sending is failure!\n");
            }  

            //every time you pick one node out, you need to release the memory no matter 
            //whether or not you've successfully send it out! 
            if(msg){
                free(msg); 
                msg = NULL;
            }

            //shutdown my two pools
            receiverPoolShutdown();
            senderPoolShutdown(); 
            break;
        }

        int sendStatus = sendto(socketFD, msg, MSG_MAX_LEN,0,remote->ai_addr,remote->ai_addrlen);
        // printf("sendStatus == %d\n",sendStatus);
        if(sendStatus == -1){       
            printf("Message sending is failure!\n");
        } 

        if(msg){
            free(msg);
            msg = NULL;
        }
          
	}
    // receiverPoolShutdown();
    // senderPoolShutdown();
    return NULL;
}



void senderShutdown(){
    printf("sender_shutdown!\n"); 
    pthread_cancel(senderThreadPID); 
}


void senderPoolShutdown(){ 
    //kb go first
    KBShutdown();
    // sleep(1); 
    senderShutdown();
} 


 void senderPoolWaitForShutdown(){
    //Waits for thread to finish
    pthread_join(senderThreadPID,NULL); 

    //make sure sender should be joined before KB because KB has seldom operation when terminated.
    KBWaitForShutdown();

    // make sure you are freeing any dynamically allocated memory, 
    // and have destroyed mutexes and condition variables before exiting, closed the socket, etc.
    freeaddrinfo(remote);

    // this is for the possible last non-null msg in the loop(handle leak)
    if(msg){
        free(msg);
        msg=NULL;
    } 
    
    if(senderPool){
        List_free(senderPool,releaseItemInSenderPool); 
    }
    //ensure no threads was blocked! so better put on the last
    pthread_mutex_destroy(&accessSenderPoolMutex);//lock
    pthread_cond_destroy(&nonSenderEmptyPoolCondVar);//convar 
}
 
 
  
void releaseItemInSenderPool(void* item){
    if(!item){
        free(item);
        item = NULL; 
    }
}
 

//locks
void senderPoolConVarSignal(){
    pthread_cond_signal(&nonSenderEmptyPoolCondVar);
}
void senderPoolMutexLock(){
    pthread_mutex_lock(&accessSenderPoolMutex); 
}
void senderPoolMutexUnLock(){
    pthread_mutex_unlock(&accessSenderPoolMutex);
}