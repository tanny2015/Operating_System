#ifndef _SENDER_H_
#define _SENDER_H_
#include <pthread.h>
#include <stdbool.h>
#include "list.h"
 
#define MSG_MAX_LEN 1024

/****  LOCK CAN NOT SHARE! SHOULD NOT DO THESE! ****/
// static pthread_cond_t nonSenderEmptyPoolCondVar = PTHREAD_COND_INITIALIZER;
// static pthread_mutex_t accessSenderPoolMutex = PTHREAD_MUTEX_INITIALIZER;
// static pthread_mutex_t accessSenderStatusMutex = PTHREAD_MUTEX_INITIALIZER;

void createSenderPool(char* remoteHost , char* remotePort, int sock);
void senderPoolShutdown();
void senderPoolWaitForShutdown();

void senderPoolConVarSignal();
void senderPoolMutexLock();
void senderPoolMutexUnLock();

#endif