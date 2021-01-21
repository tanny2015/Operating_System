#ifndef _RECEIVER_H_
#define _RECEIVER_H_
#include <pthread.h>
#include <stdbool.h>
#include "list.h"
#define MSG_MAX_LEN 1024

void createReceiverPool(unsigned short myPort, int sock);


void receiverPoolWaitForShutdown();
void receiverPoolShutdown();


//locks
void receiverPoolConVarWait();
void receiverPoolMutexLock();
void receiverPoolMutexUnLock();

#endif