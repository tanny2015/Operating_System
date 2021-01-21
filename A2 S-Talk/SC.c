#include "SC.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static pthread_t SCThreadPID;
List* receiverPool;

static char * msg; 
void* SCThread(); 

void createSC(List* sharedList){  
    receiverPool = sharedList;
    if(pthread_create(&SCThreadPID, NULL, SCThread, NULL) != 0){
        printf("pthread_create -- SC create failed\n");
        exit(EXIT_FAILURE);
    } 
}


void* SCThread(){
     msg = NULL;
    //IF OTHER MUDULE RESPONSIBLE FOR SHUTING THIS MODULE
    //NEVER DO THIS SHUTDOWN BY MORE THAN ONE MODULE!
	while(1) {   
        
        //pthread_testcancel();
        receiverPoolMutexLock();  

        //empty pool -- wait for UDP receiver's signal to get node to read
        if(List_first(receiverPool)==NULL){
            //printf("screen empty!");
            receiverPoolConVarWait();
            //printf("!!!!!!!!signal received!!!!!!!!\n");
        }  
        // move the cursor to the list head if not empty
        if(List_first(receiverPool)!=NULL){
            //remove one item out to send
            msg = (char*)List_remove(receiverPool);
        } 

        receiverPoolMutexUnLock();
        

        if(msg){
            //print anything other than a single '!'
            if(!(strlen(msg) == 2 && msg[0]=='!') && puts(msg) == EOF){
                printf("screen printing error -- receiver.c ---\n");
            } 
        } 
        free(msg);
        msg=NULL;
         
    }
    return NULL;
}


void SCShutdown(){
    printf("sc_shutdown!\n"); 
    pthread_cancel(SCThreadPID);
}


void SCWaitForShutdown(){
    pthread_join(SCThreadPID,NULL);
    if(msg){
        free(msg);
        msg=NULL;
    }
} 






































