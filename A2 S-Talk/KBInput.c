#include "KBInput.h" 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static pthread_t KBThreadPID;
static List* senderPool; 
static char* kbMsg;  
 
void* KBThread(); 


void createKB(List* sharedList){
    senderPool = sharedList; 
    if(pthread_create(&KBThreadPID, NULL, KBThread, NULL) != 0){
             printf("pthread_create -- KB create failed\n");
             exit(EXIT_FAILURE);
    }  
}


void* KBThread(){ 
	while(1) {    
        //pthread_testcancel();
        kbMsg = (char*)malloc(MSG_MAX_LEN * sizeof(char));
        
        //read from keyboard
		memset(kbMsg, 0, MSG_MAX_LEN * sizeof(char));
		// read(STDIN_FILENO, msg, sizeof(msg));//buggy!!
        read(STDIN_FILENO, kbMsg, MSG_MAX_LEN * sizeof(char));//从键盘读取用户输入信息 
        /* if(kbMsg){
            printf("--- %s--- \n",kbMsg);
        } */

        senderPoolMutexLock();  
        //add to list(Note:even the keyboard send me a '\n', I will treat it as a valid item)
        //there will be at least a '\n' in msg, so no empty char
        if(List_count(senderPool) >= LIST_MAX_NUM_NODES){
            printf("NO ENOUGH NODES! CAN NOT ADD KB INPUT, OMIT! --- %d---\n",List_count(senderPool));
            //senderPoolConVarSignal();
        } 

        if(List_append(senderPool,kbMsg)!=0){
            printf("Node appending failed!");; 
        } 
        kbMsg = NULL;
        //the signal would be better to operate in a condition such as 'successful adding the nodes'
        //but in A2, we treat every keyborad input as valid content, basically no need to check and 
        //also for the perpose that the signal should not be put in a situation which is full of constraints.
        senderPoolConVarSignal();
        senderPoolMutexUnLock(); 

	}
    return NULL;
}


 void KBWaitForShutdown(){
     //Waits for thread to finish
    pthread_join(KBThreadPID,NULL);
    if(kbMsg!=NULL){free(kbMsg);}
    kbMsg = NULL;
 }

void KBShutdown(){
    printf("kb_shutdown!\n");  
    pthread_cancel(KBThreadPID);
}
















