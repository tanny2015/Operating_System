#include <stdio.h>
#include <stdlib.h>
#include "receiver.h"
#include "sender.h"
#include "string.h"
#include <netdb.h>
#include <unistd.h>
 
// s-talk 6060 csil-cpu3 6001
// [executing file][arg0--null][arg1--Myport][arg2-RemoteIP][arg3-RemotePort]
int main(int argc, char** args){
    if(argc<4){
        return EXIT_FAILURE;
    }

    unsigned short myPort = atoi(args[1]);//char to int
    char* remoteName = args[2];

    // unsigned short remotePort = atoi(args[3]); 
    char* remotePort = args[3];

    //Create the socket for UDP 
    int socketFD = socket(PF_INET, SOCK_DGRAM, 0);
    if(socketFD==-1){
        printf("socketDescriptor failed -- receiver.c \n");
        exit(EXIT_FAILURE);
    }  

    //Set up two pools
    createReceiverPool(myPort,socketFD);
    createSenderPool(remoteName,remotePort,socketFD);
    

    //printf("Starting..");

 
    //wait for two pools to finish
    receiverPoolWaitForShutdown();
    senderPoolWaitForShutdown();

    close(socketFD);
    remoteName = NULL;
    remotePort = NULL;
    puts("----- End ------");
    return 0;
}