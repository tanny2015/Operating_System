#include "receiver.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include "sender.h"
#include "SC.h"
 
static pthread_t receiverThreadPID;

//shared
static int socketFD;
List* receiverPool;
pthread_cond_t nonEmptyReceiverPoolCondVar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t accessReceiverPoolMutex = PTHREAD_MUTEX_INITIALIZER;

//net
static unsigned short localPort;

//common
static char *rcvMsg;
void createReceiver(unsigned short myPort);
void* receiveThread();
void releaseItemInReceiverPool(void* item);



void createReceiverPool(unsigned short myPort, int sock){
    socketFD = sock;
    // pthread_mutex_init(&accessReceiverPoolMutex,NULL);
    // pthread_cond_init(&nonEmptyReceiverPoolCondVar,NULL);
    receiverPool = List_create();
    if(receiverPool==NULL){
        printf("receiverPool create failed!\n");
        exit(EXIT_FAILURE);
    }
    createReceiver(myPort);
    createSC(receiverPool); 
    puts("Receiver Pool Ready ..");
}


void createReceiver(unsigned short myPort){
    localPort = myPort;
    if(pthread_create(
        &receiverThreadPID,  //PID(by pointer)
        NULL,  // attributes -almost always null
        receiveThread, // Function
        // arguments. this is the para for 3rd function. void * unused
        // char is a pointer, c has no reference, it has only pointer
        NULL
        ) != 0){
            printf("pthread_create -- receiver create failed\n");
            exit(EXIT_FAILURE);
        } 
}

 

void* receiveThread(){  
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET; 
    sin.sin_addr.s_addr = htonl(INADDR_ANY);// Host to Network long
    sin.sin_port = htons(localPort);//local byte to network stream
 
    //Bind the socket to the port 
    if(bind(socketFD, (struct sockaddr*) &sin, sizeof(sin)) == -1){
        perror("socket binding failed -- receiver.c \n");
        exit(EXIT_FAILURE);
    }
  
    struct sockaddr_in sinRemote;
    unsigned int sin_len = sizeof(sinRemote); 
    
    while(1){ 

        //pthread_testcancel(); //cancel point
        rcvMsg = (char*) malloc(MSG_MAX_LEN * sizeof(char));
        memset(rcvMsg, 0, MSG_MAX_LEN * sizeof(char));
 
        //pthread_testcancel();
        int recvStatus = recvfrom(socketFD, rcvMsg, MSG_MAX_LEN * sizeof(char),0, (struct sockaddr *) &sinRemote, &sin_len);
        if(recvStatus == -1){
            printf("recvfrom -- receiver.c failed \n");
        }

        /* strlen will count \n  
        printf("--%s-- %ld--- %c: \n", rcvMsg,strlen(rcvMsg), (char)(rcvMsg[0]));
        printf("--%d-- %d--- : \n",strlen(rcvMsg)== 2,(char)rcvMsg[0]=='!'); */
        if(rcvMsg &&  (strlen(rcvMsg)== 2) && ((char)rcvMsg[0]=='!')){ 
            puts("--------------------'!' catched! -- terminate this program! --------------------");
            // fflush(stdout); 

            pthread_mutex_lock(&accessReceiverPoolMutex); 
            // NOT REQUIRED TO HANDLE
            if(List_count(receiverPool) >= LIST_MAX_NUM_NODES){
                printf("NO ENOUGH NODES! CAN NOT ADD UDP INPUT, OMIT!--- %d---\n",List_count(receiverPool));
                //pthread_cond_signal(&nonEmptyReceiverPoolCondVar);
            }
            //add the node and signal screen thread to wake up
            //this operation not seems necessary, just to reduce the blocking condition
            //if there is a single thread was blocked, the mutex and convar might not be capable of releasing.     
            if(List_append(receiverPool,rcvMsg)!=0){
                printf("Node appending failed!");
            } 
            rcvMsg = NULL;
            pthread_cond_signal(&nonEmptyReceiverPoolCondVar); 
            pthread_mutex_unlock(&accessReceiverPoolMutex); 

            senderPoolShutdown(); 
            receiverPoolShutdown(); 
            break; 
        }  
        pthread_mutex_lock(&accessReceiverPoolMutex); 

        // NOT REQUIRED TO HANDLE
        if(List_count(receiverPool) >= LIST_MAX_NUM_NODES){
            printf("NO ENOUGH NODES! CAN NOT ADD UDP INPUT, OMIT!--- %d---\n",List_count(receiverPool));
            //pthread_cond_signal(&nonEmptyReceiverPoolCondVar);
        }     
        if(List_append(receiverPool,rcvMsg)!=0){
            printf("Node appending failed!");
        }
        rcvMsg = NULL;
        pthread_cond_signal(&nonEmptyReceiverPoolCondVar); 
        pthread_mutex_unlock(&accessReceiverPoolMutex); 
     
    }  
    return NULL;
}

void receiverShutdown(){ 
    printf("Receiver_shutdown!\n"); 
    pthread_cancel(receiverThreadPID); 
}

void receiverPoolShutdown(){ 
    SCShutdown();
    // sleep(1);
    receiverShutdown(); 
} 


void receiverPoolWaitForShutdown(){
    //Waits for thread to finish
    pthread_join(receiverThreadPID,NULL);
    SCWaitForShutdown();

    if(rcvMsg){
        free(rcvMsg);
        rcvMsg=NULL;
    }  
    if(receiverPool){
        List_free(receiverPool,releaseItemInReceiverPool);//list
    }
    pthread_mutex_unlock(&accessReceiverPoolMutex);
    pthread_mutex_destroy(&accessReceiverPoolMutex);//lock
} 

void releaseItemInReceiverPool(void* item){
    if(!item){
        free(item);
        item = NULL;
    }
    
}


void receiverPoolConVarWait(){
    pthread_cond_wait(&nonEmptyReceiverPoolCondVar, &accessReceiverPoolMutex);
}

void receiverPoolMutexLock(){
    pthread_mutex_lock(&accessReceiverPoolMutex); 
}

void receiverPoolMutexUnLock(){
    pthread_mutex_unlock(&accessReceiverPoolMutex);
}