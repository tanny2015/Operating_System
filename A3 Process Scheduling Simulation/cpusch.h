//
// Created by tanny on 11/14/2020.
//

#include "list.h"
#define highPriority 0
#define middlePriority 1
#define lowPriority 2

# define NULL_PID -1
# define maxMsgLength 40

//PrecessStateSuperSuspend only for init process
# define ProcessStateRunning 1
# define ProcessStateReadyBase 20
# define ProcessStateReady_HighPriority 20
# define ProcessStateReady_MidPriority 21
# define ProcessStateReady_LowPriority 22
# define ProcessStateSuperSuspend 30
# define ProcessStateSenderBlocked 31
# define ProcessStateReceiverBlocked 32

# define ProcessStateSEMBase 33
# define ProcessStateSEM_0_Blocked 33
# define ProcessStateSEM_1_Blocked 34
# define ProcessStateSEM_2_Blocked 35
# define ProcessStateSEM_3_Blocked 36
# define ProcessStateSEM_4_Blocked 37

# define QueueTypeBlockedReceiver 1110
# define QueueTypeBlockedSender   1111
# define QueueTypeBlockedReceiverAndSender  1112
# define QueueTypeBlockedSEMBase   1113
# define QueueTypeBlockedSEM_0   1113
# define QueueTypeBlockedSEM_1   1114
# define QueueTypeBlockedSEM_2   1115
# define QueueTypeBlockedSEM_3   1116
# define QueueTypeBlockedSEM_4   1117
# define QueueTypeBlockedSuperSuspend   1118

enum SearchSenderFor{
    SearchSenderForEqualMsgPID=1, SearchSenderForEqualPID
};

typedef struct msg{
    char msg[maxMsgLength];
    int sentToPID;
} msgNode;


//IN THIS ASSIGNMENT, PCB MEANS 'PROCESS'
typedef struct PCB{
    int PID;
    int priority; //0, 1, or 2
    int processState;
    msgNode sendmsg;
    List* myQueue;//for printing use
}processNode;

typedef struct semaphore{
    int val;//an integer representing the value of the sem
    List *semWaitingQueue; //a list of processes waiting on that sem
}sema;


