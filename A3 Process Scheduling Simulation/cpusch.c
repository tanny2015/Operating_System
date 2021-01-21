//
// Created by tanny on 11/14/2020.
//

#include "cpusch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

processNode* me;
processNode* initProcess;
List* readyQueue_lowP;
List* readyQueue_midP;
List* readyQueue_highP;
List* receiverWaitingQueue;
List* senderWaitingQueue;
static int firstAvailablePID = 0;
sema sem[5];

///////////////////  Process Schedule ///////////////////////
//Switches
bool insertToReadyQueue(processNode* process);
bool suspendProcess(processNode *process, int BlockQueueType);
bool resumeProcessFromSenderAndReceiver(processNode *process, int BlockQueueType);
bool resumeProcessFromSem(int SemIndex, processNode ** returnProcess);
void scheduleNewRunningProcess();
bool scheduleNewAndSuspendPreviousProcess(processNode *processToBeSuspend,int BlockQueueTypeToSwitch);
//Init Process
bool isMEInitProcess();
void initProcessSuspend();
void initProcessResume();
void suspendInitProcessWhenOtherProcessAvailable(processNode* newProcess);

/////////////////////////  Search //////////////////////////
bool isPIDExistInThisProgram(int pid, processNode **returnProcess);
processNode * searchProcessInSemWaitingQueues(int pid);
processNode * searchProcessInReadyQueues(int pid);
processNode * searchProcessInQueue(List * queue, int pid);
processNode * searchProcessInBlockedQueues(int pid, int QueueType);
processNode * searchForTargetSender(int sentTOPID, enum SearchSenderFor searchFor);
processNode *searchProcessInAllQueues(int pid, bool isInitProcessIncluded,bool isMeIncluded);
processNode *searchProcessAllProgramWhoesMsgRecipientMatchPID(int msgReceiverPID);
bool IsEqualToMyPID(void* item, void* myPID);
bool IsEqualToSentToPID(void* item, void* myPID);

/////////////////////////  Validate //////////////////////////
bool isValidCreate_C(int priority);
bool isKBInputMsgInValidLength(char*msg);
bool isValidSemID(int semID);
bool isValidSemInput(int semID, int value);

///////////////////  Process Properties ///////////////////////
void setProcessStateAndQueue(processNode*process, int processState, List* myQ);
void setMsgNode(processNode * process, int sent_to_PID, char* msg);
void removeMsg(processNode* process);
void setMsg(processNode *process, char*msg, int toPID);


///////////////////////  COMMON I/O //////////////////////////
void clearRawString(char * raw);
void clearRawInt(int *raw, int len);
void clear_cache(FILE *fp);
bool readInt(int* raw, int len);
void readString(char * str);
char * stateToChar(int processState);
void KBInputNote();


/////////////////////////  Trivial  //////////////////////////
int availablePID();
bool thereIsNOProcessInReadyQueue();
bool hasNodeAvailable();
void releaseItem(void * pItem);



//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////  Main Implemetation   //////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//Create  | C | int priority (0 =high, 1 = norm, 2= low)
//Action: create a process and put it on the appropriate ready Q.
//Reports: success or failure, the pid of the created process on success.
//Note: set * returnProcess to NULL before using this func
bool Create(int priority, processNode**returnProcess, bool needResult){
    printf("|||||||||| Start to create a new process with priority %d ... \n", priority);
    if(!isValidCreate_C(priority)){
        //error log is inside the validate function.
        return false;
    }
    if(hasNodeAvailable()){
        processNode* resultProcess = (processNode *) malloc(sizeof(processNode));
        resultProcess->PID = availablePID();
        resultProcess->priority = priority;
        resultProcess->processState = ProcessStateReadyBase + priority;
        setMsgNode(resultProcess, NULL_PID, NULL);
        resultProcess->myQueue = NULL;
        if (resultProcess->processState == ProcessStateReady_HighPriority){
            resultProcess->myQueue = readyQueue_highP;
        }else if(resultProcess->processState == ProcessStateReady_MidPriority){
            resultProcess->myQueue = readyQueue_midP;
        }else if(resultProcess->processState == ProcessStateReady_LowPriority){
            resultProcess->myQueue = readyQueue_lowP;
        }else{
            printf("Create Method ERROR!");
            return NULL;
        }

        int status = List_prepend(resultProcess->myQueue,resultProcess);
        if(status == LIST_SUCCESS){
            printf("Process with PID: %d Create Success!\n",resultProcess->PID);
            if (needResult){
                *returnProcess = resultProcess;//********** super bug blocks me 6 hours.
            }
            suspendInitProcessWhenOtherProcessAvailable(resultProcess);
            return true;
        }else{
            printf("Process with PID: %d Create Fail!",resultProcess->PID);
            return NULL;
        }
    }
    else{
        printf("ERROR: Run out of the head node! Create process failed! \n");
        return NULL;
    }
    return NULL;
}

//Characteristic of initProcess --> PID == 0
void CreateInitProcess(){
    initProcess = (processNode *) malloc(sizeof(processNode));
    initProcess->PID = availablePID();
    initProcess->priority = highPriority;
    initProcess->processState = ProcessStateRunning;
    setMsgNode(initProcess, NULL_PID, NULL);
    initProcess -> myQueue = NULL;
    me = initProcess;
    //printf("INIT Process with PID: %d Create Success!",initProcess -> PID);
}


/*Fork  | F  | None
Action: Copy the currently running process and put it on the ready Q corresponding to the original process priority.
Attempting to Fork the "init" process (see below) should fail.
Reports: success or failure, the pid of the resulting (new) process on success.*/
void Fork(){
    printf("|||||||||| Start to Fork current process ... \n");
    if (!isMEInitProcess()){
        processNode *newProcess = NULL;
        if(Create(me->priority,&newProcess,true)){
            setMsgNode(newProcess, me->sendmsg.sentToPID,me->sendmsg.msg);
            printf("SUCCESS: Fork Operation has been done successfully！ \n");
        }
        //error handled inside the create method
    }
    else{
        printf("ERROR: Not allow to fork init process! \n");
    }
}

//someone intent to kill the init process and the operation is validated as legal
void EndTheProgram(){
    //release malloc stuff -- semWaitingQueue and all the queue and the items in queues
    List_free(readyQueue_lowP,releaseItem);
    List_free(readyQueue_midP,releaseItem);
    List_free(readyQueue_highP,releaseItem);
    List_free(receiverWaitingQueue,releaseItem);
    List_free(senderWaitingQueue,releaseItem);
    for(int i=0; i<5; i++){
        if (sem[i].semWaitingQueue){
            List_free(sem[0].semWaitingQueue,releaseItem);
        }
    }
    free(initProcess);
    initProcess = NULL;
    exit(0);
}

/*Exit  | E  | None
Action: kill the currently running process.
Reports: process scheduling information (eg. which process now gets control of the CPU)*/
void Exit(){
    printf("|||||||||| Start to Exit... \n");
    processNode *processToBeKilled = me;
    if (me!= initProcess){
        scheduleNewRunningProcess();
        int previousPID = processToBeKilled->PID;
        free(processToBeKilled);
        processToBeKilled = NULL;
        printf("Success to Exit the process(PID = %d), Now the newly running process is: PID = %d", previousPID, me->PID);
        return;
    }else{
        //if init process is the last process in this program, then exit the program
        int sum = List_count(senderWaitingQueue) + List_count(senderWaitingQueue);
        for (int i = 0; i<5; i++){
            if(sem[i].semWaitingQueue){
                sum += List_count(sem[i].semWaitingQueue);
            }
        }

        if (sum <= 0 || thereIsNOProcessInReadyQueue()){
            printf("The running process is init process and it is the only process alive now. You attempt to end the program now..! \n");
            EndTheProgram();
        }
        else{
            printf("Failure: There are still some other process alive! you are not allowed to kill init process in this case! \n");
            return;
        }
    }

}
/*Kill  | K | int pid (pid of process to be killed)
Action: kill the named process and remove it from the system.
Reports: action taken as well as success or failure.*/
void Kill(int pidToBeKilled){
    printf("|||||||||| Start to kill a process(PID = %d) .. \n", pidToBeKilled);
    //it will handle the 2 cases when me is init process and not
    // now processToBeKilled must not be me!
    processNode *processToBeKilled = searchProcessInAllQueues(pidToBeKilled,false, false);
    if (processToBeKilled){
        if(List_remove(processToBeKilled->myQueue)) {//must do search before this line
            free(processToBeKilled);
            processToBeKilled = NULL;
            printf("SUCCESS: the process (PID = %d) has been killed! \n",pidToBeKilled);
        }else{
            printf("Failure: sth wrong happens when killing a process, please check! \n");
            return;
        }
    }
    else{
        // now me is not the init process and user wanna kill init process -- not allowed!
        if (pidToBeKilled == initProcess->PID && !isMEInitProcess()){
            printf("Failure: PID = %d(NOT init process) is still running, CAN NOT kill init process! \n", me->PID);
            return;
        }
        if(pidToBeKilled == me->PID){
            Exit();
            return;
        }
        printf("Error: the input pid is invalid currently! please check! \n");
    }
}


/*Quantum  | Q  | None
Action: time quantum of running process expires.
Reports: action taken (eg. process scheduling information)*/
void Quantum(){
    printf("|||||||||| Start to do Quantum operation... \n");
    //init process can not stopped by Quantum.
    if (isMEInitProcess()){
        printf("Failure: Init process can not be stopped by Quantum!  \n");
    }
    else if(!isMEInitProcess() && thereIsNOProcessInReadyQueue()){
        printf("Failure: Only me and init process, no other process available in ready queue, can NOT Quantum me! \n");
    }
    else{
        processNode *processToBeExpired = me;
        scheduleNewRunningProcess();
      if(insertToReadyQueue(processToBeExpired)){
          printf("SUCCESS: the process has already gone to ready queue(priority = %d)", processToBeExpired->priority);
          printf("and the current running process'pid is : %d \n", me->PID);
      }else{
          printf("Failure:  the process is UNABLE to go to ready queue(priority = %d)\n", processToBeExpired->priority);
          return;
      }
    }
}







void printProcessInfo(int pid, bool shouldPrintError){
    //validate the input
    processNode * process = NULL;
    if(isPIDExistInThisProgram(pid, &process)){
        //0.pid
        printf(" ------------ INFO(PID = %d)  \n", pid) ;
        char stateInfo[maxMsgLength];
        memset(stateInfo, 0, maxMsgLength * sizeof(char));
        strcpy(stateInfo, stateToChar(process->processState));
        //1. state 2. priority
        printf("Priority = %d, State Info: %s \n", process->priority, stateInfo);
        //4.message and 5.receiver id
        if (strlen(process->sendmsg.msg)>0){
            printf("Last sended message (receiver PID = %d): %s \n", process->sendmsg.sentToPID, process->sendmsg.msg);
        } else{
            printf("container{No Message} \n");
        }
    }
    else{
        if (shouldPrintError){
            printf("FAILURE: The Process (PID = %d) is currently invalid! \n", pid);
        }
    }
}

/*Procinfo  | I | int pid (pid of process information is to be returned for)
Action: dump complete state information of process to screen (this includes process status and anything else you can think of)
Reports: see Action*/
void Procinfo(int pid ) {
    printf("|||||||||| Start to dump complete state information for process(PID = %d)...  \n",pid);
    printProcessInfo(pid, true);
}

void printProcessesInfoInAQueue(List* queue){
    processNode *iter = (processNode *)List_last(queue);
    while(iter){
        processNode * temp = (processNode *)(iter);
        printProcessInfo(temp->PID, false);
        iter = List_prev(queue);
    }
}

/*Totalinfo  | T  | None
Action: display all process queues and their contents
Reports: see Action*/
void Totalinfo(){
    printf("|||||||||| Start to display all process queues and their contents...  \n" );
    printProcessesInfoInAQueue(readyQueue_highP);
    printProcessesInfoInAQueue(readyQueue_midP);
    printProcessesInfoInAQueue(readyQueue_lowP);
    printProcessesInfoInAQueue(receiverWaitingQueue);
    printProcessesInfoInAQueue(senderWaitingQueue);
    printProcessInfo(initProcess->PID, false) ;
    if (!isMEInitProcess()){
        printProcessInfo(me->PID, false);
    }
    printf("------------【Semaphore】");
    for (int i = 0; i < 5; i++){
        printf("\n【semID: %d --- sem Value = %d】", i,sem[i].val);
        if(sem[i].semWaitingQueue ){
            if(List_count(sem[i].semWaitingQueue)){
                puts("\nBlock queue contains: ");
                printProcessesInfoInAQueue(sem[i].semWaitingQueue);
            }
        }else{
            printf("(NOT Initiate Yet!)");
        }
    }
}

/*Send  | S | pid (pid of process to send message to), char *msg (null terminated message string, 40 char max)
Action: send a message to another process - block until reply
Reports: success or failure, scheduling information, and reply source and text (once reply arrives)*/
void Send(int receiverPID, char*msg){
    if(!isKBInputMsgInValidLength(msg)){
        printf("ERROR: The message length is more than %d, Message sending failed. \n", maxMsgLength);
        return;
    }
    printf("I(PID=%d) am going to send the following message: %s to process(PID=%d)\n", me->PID,msg,receiverPID );
    processNode *processToBeSuspended = me;
    //Go to all queues to see if the specific receiver is valid
    processNode *receiverProcess = NULL;
    if(isPIDExistInThisProgram(receiverPID, &receiverProcess)){
        //if the PID is valid, message can definitely sent successfully
        printf("Message Sending Successfully! \n");
        //a) if receiver process is blocked in the receiver blocking queue, unblock it, switch it to ready queue
        if (receiverProcess->myQueue == receiverWaitingQueue){
            //before the following line, have to run search function to move the cursor
            if(resumeProcessFromSenderAndReceiver(receiverProcess, QueueTypeBlockedReceiver)){
                printf("The process(PID = %d): who was waiting for a message has been unblock now! \n",receiverPID);
                printf("and now it goes to ready queue \n");//**please check!
                suspendInitProcessWhenOtherProcessAvailable(receiverProcess);
            }
        }
        //b) if it is valid process, but not a receiver blocked process. then just save msg to my msg container,
        // and block myself
        else{
            setMsg(processToBeSuspended,msg,receiverPID);
            if (isMEInitProcess()){
                printf("Attention: NOT allowed to block init process! \n");
                return;
            }

            //process switch
            if(scheduleNewAndSuspendPreviousProcess(processToBeSuspended, QueueTypeBlockedSender)){
                printf("The process(PID = %d): has been blocked to sender waiting queue now! \n",processToBeSuspended->PID);
                printf("Current running process is PID = %d\n",me->PID);
            }
            else{
                printf("Schedule new process failed, please check!\n" );
                return;
            }
        }
        return;
    }
    //invalid target pid
    else{
        printf("ERROR! the target PID: %d is invalid, please check", receiverPID);
        return;
    }
}

/*Receive  | R  | None
Action: receive a message - block until one arrives
Reports: scheduling information and (once msg is received) the message text and source of message*/

//Go to the sender blocked queue to check if there is some one once send me a message
//a). if there is a process send me a message, print the sender's information and message to the screen, remove the message
//b). if there is no process once send me a message, block myself
void Receive(){
    printf("Try to Receive a Message... \n");
    processNode *processToBeBlocked = me;
    //processNode *targetSender = searchForTargetSender(me->PID,SearchSenderForEqualMsgPID);
    //we need to search the sender in all queue, because some senders can be unblocked by any replier, but only specific receiver can remove the msg from those sender.
    processNode *targetSender = searchProcessAllProgramWhoesMsgRecipientMatchPID(processToBeBlocked->PID);
    if(targetSender){
         printf("Have received a message with content: %s from sender(PID = %d)\n", targetSender->sendmsg.msg, targetSender->PID);
         removeMsg(targetSender);
    }else{
        printf("No Sender once send me a message!");
        if (isMEInitProcess()){
            printf("\nAttention: NOT allowed to block init process! \n");
            return;
        }
        printf(" I am gonna block myself!\n");
        if(scheduleNewAndSuspendPreviousProcess(processToBeBlocked,QueueTypeBlockedReceiver)){
            printf("SUCCESS:Process(PID=%d) has been blocked to the receiver waiting queue! \n", processToBeBlocked->PID);
            printf("Current running process is PID = %d \n",me->PID);
        }else{
            printf("FAILURE:Process(PID=%d) is NOT blocked to the receiver waiting queue! \n", processToBeBlocked->PID);
        }
    }
}

/*Reply  | Y | int pid (pid of process to make the reply to), char *msg (null terminated reply string, 40 char max)
Action: unblocks sender and delivers reply
Reports: success or failure*/
//Go to the sender blocked queue to check if the specific process is in that queue
//a). if that process found, print the reply message
void Reply(int targetSenderPID,char *msg){
    if(!isKBInputMsgInValidLength(msg)){
        printf("ERROR: The message length is more than %d, Message reply failed. \n", maxMsgLength);
        return;
    }
    printf("Try to reply a message(content: %s) to sender (PID = %d)... \n", msg,targetSenderPID);
    processNode *processToBeBlocked = me;
    //processNode *targetSender = searchForTargetSender(targetSenderPID,SearchSenderForEqualPID);
    processNode *targetSender = searchProcessInAllQueues(targetSenderPID,true,true);
    if(targetSender){
        printf("SUCCESS：Message is already replied to Process (PID = %d)", targetSender->PID);
        if (targetSender->myQueue == senderWaitingQueue){
            printf("Target is in sender waiting queue...\n");
            //unblock the sender
            if(resumeProcessFromSenderAndReceiver(targetSender,QueueTypeBlockedSender)){
                printf("SUCCESS: the target sender(PID = %d) has been unblocked \n",targetSender->PID);
                suspendInitProcessWhenOtherProcessAvailable(targetSender);
            }else{
                printf("FAIL: reply failed  \n");
            }
        }
        else{
            printf("Target is in NOT sender waiting queue. Nothing to do.\n" );
        }
    }
    //error handling is inside the searchForTargetSender Func
}

//New Semaphore | N | int sem (semphore ID) -- sem index, initial value (0 or higher)
//Action: Initialize the named sem with the value given. ID s can take a value from 0 to 4.
//This can only be done once for a sem - subsequent attempts result in error.
//Reports: action taken as well as success or failure.
void semNew(int semaphore, int value){
    if(!isValidSemInput(semaphore, value)){
        //error log is inside the validate function
        return;
    }
    if(sem[semaphore].semWaitingQueue != NULL){
        printf("ERROR: The semaphore with this ID has been created before! \n");
        return;
    }
    sema* semNode = &sem[semaphore];
    semNode->val = value;
    if (!semNode->semWaitingQueue){
        semNode->semWaitingQueue = List_create();
    }
    printf("New Semaphore has been created: ID = %d, Initial Value = %d", semaphore, value);
}

/*Sempahore  | P | int sem (semphore ID)
Action: execute the sem P operation on behalf of the running process. You can assume sempahores IDs numbered 0 through 4.
Reports: action taken (blocked or not) as well as success or failure. */
void semP(int semaphore){
    if(!isValidSemID(semaphore)){
        //error log is inside the validate function
        return;
    }
    if(sem[semaphore].semWaitingQueue == NULL){
        printf("ERROR: The semaphore with this ID has NOT been initiated before! \n");
        return;
    }
    sema* semNode = &sem[semaphore];
    //1.if value <=0, then block current process,schedule new process, then do sem--
    if (semNode->val <=0){
        processNode *processToBeSuspended = me;
        if(scheduleNewAndSuspendPreviousProcess(processToBeSuspended, (QueueTypeBlockedSEMBase + semaphore))){

            printf("Success! Process(PID = %d) has been blocked! by Semaphore[ID = %d], sem value = %d\n",processToBeSuspended->PID, semaphore, semNode->val);
            printf("Current running process has switched to process(PID = %d) \n", me->PID);
            //semNode->val-- can not be done here! because if so, the sem value can be -1! but the actual least sem value
            //can only be zero! -- have to move this operation to 'semV' function(after unblock the waiting process!)
        }else{
            puts("P operation has been failed. please check!\n");
            return;
        }
    }
    else{
        printf("Semaphore[ID = %d] has a sem value = %d > 0\n", semaphore, semNode->val--);
        printf("So I(PID=%d) am now going into critical section and update the sem value to [ %d ]\n", me->PID, semNode->val);
    }
}

/*Sempahore | V | int sem (semphore ID)
Action: execute the sem V operation on behalf of the running process. You can assume sempahores IDs numbered 0 through 4.
Reports: action taken (whether/ which process was readied) as well as success or failure. */
void semV(int semaphore){
    if(!isValidSemID(semaphore)){
        //error log is inside the validate function
        return;
    }
    if(sem[semaphore].semWaitingQueue == NULL){
        printf("ERROR: The semaphore with this ID has NOT been initiated before! \n");
        return;
    }
    sema* semNode = &sem[semaphore];
    semNode->val++;

    if (semNode->val > 0){
        processNode *tempPro = NULL;
        //definitely there exists processes being blocked! unblock it!
        printf("Semaphore[%d]'s  sem value update to %d > 0, Go to check the Semaphore[%d] waiting queue\n", semaphore,semNode->val,semaphore );
        if(resumeProcessFromSem(semaphore,&tempPro)){
            printf("Success! the process(PID = %d) has been unblocked from SEM[%d] waiting queue and go to ready queue(priority = %d) \n",tempPro->PID,semaphore,tempPro->priority);
            printf("After unblock the waiting process and this waiting process execute 'sem.value--'(P operation), Semaphore[ID = %d]'s  sem value = %d \n", semaphore,--semNode->val );
            suspendInitProcessWhenOtherProcessAvailable(tempPro);
        }else{
            printf("V operation did NOT unblock any process in this operation! Sem[%d]'s  sem value = %d \n", semaphore,semNode->val);
        }

    }else{
        //the following line should not happened, because sem.value should always >= 0 here.
        printf("semV did NOT unblock any process in this operation! \n");
    }

}

void prepareConversation(){
    CreateInitProcess();
    readyQueue_lowP = List_create();
    readyQueue_midP = List_create();
    readyQueue_highP = List_create();
    receiverWaitingQueue = List_create();
    senderWaitingQueue = List_create();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////  ENTRY   /////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void Entry(){
    prepareConversation();
    char command[maxMsgLength];
    char raw[maxMsgLength];
    KBInputNote();
    int whileLoop = 1;
    while (whileLoop) {
        puts("\n||||||||||||||||||| Please Enter a Command... |||||||||||||||||||||||||");
        readString(command);
        if (strlen(command) != 1) {
            puts("COMMAND LENGTH ERROR: Command should have only 1 char!");
            continue;
        }
        char cmd = tolower(command[0]);
        switch (cmd) {
            case 'c': {
                //C(Create a Process)
                int priority;
                puts("Please type the priority(0 || 1 || 2): ");
                if(readInt(&priority,1)){
                    if(!(priority == 0 || priority == 1 || priority == 2)){
                        puts("Error: priority should be 0 or 1 or 2 ! ");
                        continue;
                    }
                    else{
                        Create(priority,NULL,false);
                        continue;
                    }
                }
                continue;
            }
            case 'f': {
                //F(Fork)
                Fork();
                continue;
            }
            case 'k': {
                //K(Kill any process)
                int  pid ;
                puts("Please type the process ID to kill it : ");
                if(readInt(&pid, 1)){
                    Kill(pid);
                }
                continue;
            }
            case 'e': {
                //E(Exit current process)
                Exit();
                continue;
            }
            case 'q': {
                //Q(Quantum)
                Quantum();
                continue;
            }
            case 's': {
                //S(Send a message)
                int receiverPID ;
                puts("Please type the recevier's PID: ");
                if(readInt(&receiverPID, 1)){
                    puts("Please type the message(Limited to 40 characters!):  ");
                    readString(raw);
                    if (strlen(raw) >= maxMsgLength) {
                        puts("the message is Limited to 40 characters! ");
                    }
                    else{
                        Send(receiverPID, raw);
                    }
                }
                continue;
            }
            case 'r': {
                //R(Receive a message)
                Receive();
                continue;
            }
            case 'y': {
                //Y(Reply a Message Sender)
                int senderPID ;
                puts("Please type the target sender's PID:");
                if(readInt(&senderPID, 1)){
                    puts("Please type the message(Limited to 40 characters!):");
                    readString(raw);
                    if (strlen(raw) >= maxMsgLength) {
                        puts("the message is Limited to 40 characters!");
                    }
                    else{
                        Reply(senderPID, raw);
                    }
                }
                continue;
            }
            case 'n': {
                //N(Create new Semaphore)
                int  semID ;
                int  value ;
                puts("Please type the target new semaphore's ID(0-4):");
                if(readInt(&semID, 1)) {
                    puts("Please type the target new semaphore's  initial value(>=0):");
                    if (readInt(&value, 1)) {
                        //the validate work has been done inside the following function
                        semNew( semID,  value);
                    }
                }
                continue;
            }
            case 'p': {
                //P(Semaphore P Function)
                int  semID ;
                puts("Please type the semaphore's ID(0-4): ");
                if(readInt(&semID, 1)){
                    semP( semID);
                }
                continue;
            }
            case 'v': {
                //V(Semaphore V Function
                int  semID ;
                puts("Please type the semaphore's ID(0-4):");
                if(readInt(&semID, 1)){
                    semV( semID);
                }
                continue;
            }
            case 'i': {
                //I(Print any process info)
                int  pid  ;
                puts("Please type the process's id:");
                if(readInt(&pid, 1)){
                    Procinfo( pid);
                }
                continue;
            }
            case 't': {
                //T(Print total processes in this program)
                Totalinfo();
                continue;
            }
            default: {
                //ERROR input
                puts("COMMAND INPUT ERROR: Invalid Command! please read the command list above! ");
                continue;
            }
        }
    }
}

int main() {
    Entry();
    return 0;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////  Process Schedule  //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//////////////// Switches //////////////////
bool insertToReadyQueue(processNode* process){
    int state = LIST_FAIL;
    switch (process->priority) {
        case 0:{
            state = List_prepend(readyQueue_highP,process);
            if(state == LIST_SUCCESS){
                setProcessStateAndQueue(process,ProcessStateReady_HighPriority,readyQueue_highP);
            }
            return  state == LIST_SUCCESS;
        }
        case 1:{
            state = List_prepend(readyQueue_midP,process);
            if(state == LIST_SUCCESS){
                setProcessStateAndQueue(process,ProcessStateReady_MidPriority,readyQueue_midP);
            }
            return  state == LIST_SUCCESS;
        }
        case 2:{
            state = List_prepend(readyQueue_lowP,process);
            if(state == LIST_SUCCESS){
                setProcessStateAndQueue(process,ProcessStateReady_LowPriority,readyQueue_lowP);
            }
            return  state == LIST_SUCCESS;
        }
        default:{
            printf("Invalid process pid! \n");
            return false;
        }
    }
}
//Note： schedule new process should be done before current process was blocked!
//Note: the process below must be the running process(or currentProcess)
//Note: in the blocking queues, I don't do any sort when insert a process
bool suspendProcess(processNode *process, int BlockQueueType){
    List *futureQueue = NULL;
    int futureProcessState;
    if (BlockQueueType == QueueTypeBlockedReceiver){
        futureQueue = receiverWaitingQueue;
        futureProcessState = ProcessStateReceiverBlocked;
    }
    else if(BlockQueueType == QueueTypeBlockedSender){
        futureQueue = senderWaitingQueue;
        futureProcessState = ProcessStateSenderBlocked;
    }else if (BlockQueueType == QueueTypeBlockedSEM_0){
        futureQueue = sem[0].semWaitingQueue;
        futureProcessState = ProcessStateSEM_0_Blocked;
    }else if (BlockQueueType == QueueTypeBlockedSEM_1){
        futureQueue = sem[1].semWaitingQueue;
        futureProcessState = ProcessStateSEM_1_Blocked;
    }else if (BlockQueueType == QueueTypeBlockedSEM_2){
        futureQueue = sem[2].semWaitingQueue;
        futureProcessState = ProcessStateSEM_2_Blocked;
    }else if (BlockQueueType == QueueTypeBlockedSEM_3){
        futureQueue = sem[3].semWaitingQueue;
        futureProcessState = ProcessStateSEM_3_Blocked;
    }else if (BlockQueueType == QueueTypeBlockedSEM_4){
        futureQueue = sem[4].semWaitingQueue;
        futureProcessState = ProcessStateSEM_4_Blocked;
    }
        //special
    else if (BlockQueueType == QueueTypeBlockedSuperSuspend){
        if (thereIsNOProcessInReadyQueue() && me==initProcess ){
            puts("You attempt to suspend initProcess while no process available on the ready queue and init process is running process! Failed!");
            return false;
        } else{
            initProcessSuspend();
            return true;
        }
    }
    else{
        printf("input error!");
        return false ;
    }

    int result = LIST_FAIL;
    if (futureQueue){
        //ATTENTION: List_func return 0-success! -1-failure!
        result = List_prepend(futureQueue,process);//***********check!
        if (result == LIST_SUCCESS){
            /*process->processState = futureProcessState;
            process->myQueue = futureQueue;*/
            setProcessStateAndQueue(process,futureProcessState,futureQueue);
        }
    }else{
        printf("ERROR: You can NOT do this operation because sem[%d] has not be initiated!\n", (BlockQueueType - QueueTypeBlockedSEMBase));
    }
    return result == LIST_SUCCESS;
}

//*** super important: you remove item in the unblock function
//*** you have to call this after a search operation -- to make the current cursor moving to the target (to be removed) node!
bool resumeProcessFromSenderAndReceiver(processNode *process, int BlockQueueType){
    List *originalQueue = NULL;
    if (BlockQueueType == QueueTypeBlockedReceiver){
        originalQueue = receiverWaitingQueue;
    }else if (BlockQueueType == QueueTypeBlockedSender){
        originalQueue = senderWaitingQueue;
    }
    else{
        printf("input error!");
        return false ;
    }
    List *futureQueue = NULL;
    int futureState;

    int originalPriority = process->priority;
    if (originalPriority == highPriority){
        futureQueue = readyQueue_highP;
        futureState = ProcessStateReady_HighPriority;
    }else if (originalPriority == middlePriority){
        futureQueue = readyQueue_midP;
        futureState = ProcessStateReady_MidPriority;
    }else if (originalPriority == lowPriority){
        futureQueue = readyQueue_lowP;
        futureState = ProcessStateReady_LowPriority;
    }else{
        printf("input error!");
        return false ;
    }

    //remove [*** before calling this function, mush call search list!]
    List_remove(originalQueue);
    int statusInsert = List_prepend(futureQueue,process);
    if (statusInsert == LIST_SUCCESS){
        process->myQueue = futureQueue;
        process->processState = futureState;
        return true;
    }
    return false;
}

bool resumeProcessFromSem(int SemIndex, processNode ** returnProcess){
    processNode *newProcess = NULL;
    List *originalQueue = NULL;
    if(SemIndex > -1 && SemIndex < 5) {
        originalQueue = sem[SemIndex].semWaitingQueue;
        if (originalQueue && List_count(originalQueue) > 0) {
            //move cursor to the last item, to facilitate removing the last one in the next step
            newProcess = (processNode *) List_trim(originalQueue);
            if (newProcess) {
                int futureState;
                List *futureQueue = NULL;
                int originalPriority = newProcess->priority;
                if (originalPriority == highPriority) {
                    futureQueue = readyQueue_highP;
                    futureState = ProcessStateReady_HighPriority;
                } else if (originalPriority == middlePriority) {
                    futureQueue = readyQueue_midP;
                    futureState = ProcessStateReady_MidPriority;
                } else if (originalPriority == lowPriority) {
                    futureQueue = readyQueue_lowP;
                    futureState = ProcessStateReady_HighPriority;
                } else {
                    printf("input error!");
                    return false;
                }

                int statusInsert = List_prepend(futureQueue, newProcess);

                if (statusInsert == LIST_SUCCESS) {
                    newProcess->myQueue = futureQueue;
                    newProcess->processState = futureState;
                    *returnProcess = newProcess;
                    return true;
                }
            } else {
                printf("ERROR Happens While Resume Process From SEM, Please Check! \n");
                return false;
            }
        }
        else {
            printf("Failed: No Process was blocked by semaphore[%d], No unblock operation was done! \n",SemIndex);
            return false;
        }
    }else{
        printf("ERROR: Incorrect Sem Index Input! \n");
        return false;
    }
    return false;
}

// Switch current running process to the proper queue(this can definitely find a replaced process)
// Schedule new running process(after this you go to block the former running process), but never use the
//'notation -- me' afterwards(backup 'me' before this operation if you need the previous running process to be suspended afterwards),
// because it has already changed! Note:'me' changes indicates that the new process
// has already went into work!
// Insert from the first --  prepend | remove from the end  -- trim
// Don't do any printing job in the helper method!
void scheduleNewRunningProcess(){
    //1.check from the three ready queues
    if (List_count(readyQueue_highP)){
        me = List_trim(readyQueue_highP);
    }
    else if (List_count(readyQueue_midP)){
        me = List_trim(readyQueue_midP);
    }
    else if (List_count(readyQueue_lowP)){
        me = List_trim(readyQueue_lowP);
    }
    else{
        me =  initProcess;
    }
    //if the new process is initProcess
    if (me == initProcess){
        initProcessResume();
    }else{
        setProcessStateAndQueue(me,ProcessStateRunning,NULL);
    }
}

bool scheduleNewAndSuspendPreviousProcess(processNode *processToBeSuspend,int BlockQueueTypeToSwitch){

    scheduleNewRunningProcess();
    return suspendProcess(processToBeSuspend, BlockQueueTypeToSwitch);
}

//////////////// Init Process //////////////////
bool isMEInitProcess(){
    return me == initProcess;
}

void suspendInitProcessWhenOtherProcessAvailable(processNode* newProcess){
    if(me == initProcess){
        if(scheduleNewAndSuspendPreviousProcess(initProcess,QueueTypeBlockedSuperSuspend)){
            printf("New Process is now available, suspend the init process and make the new process(PID = %d) to be running process. \n",newProcess->PID);
        }
    }
}
void initProcessSuspend(){
    setProcessStateAndQueue(initProcess,ProcessStateSuperSuspend,NULL);
}

void initProcessResume(){
    me = initProcess;
    setProcessStateAndQueue(initProcess,ProcessStateRunning,NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////  Search    ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//Note: set *returnProcess to NULL before using this func
bool isPIDExistInThisProgram(int pid, processNode **returnProcess){
    processNode* resultProcess = NULL;
    resultProcess = searchProcessInAllQueues(pid, true,true);
    if (resultProcess ){
        *returnProcess = resultProcess;
        return true;
    }else{
        return false;
    }
}

// 5 semaphores
processNode * searchProcessInSemWaitingQueues(int pid){
    for (int i = 0; i < 5; i++){
        List * semWaitingQ = sem[i].semWaitingQueue;
        //current pointer go to the first place
        if (semWaitingQ && List_first(semWaitingQ)){
            void* targetNode = List_search(semWaitingQ,IsEqualToMyPID,&pid);
            if (targetNode != NULL){
                //***if the node need to remove, should be done outside and immediately after calling this function!
                //because it is the only time you can confirm the current pointer still point to this to-be removed item in msgcenter
                return (processNode *)targetNode;
            }
        }
    }
    //printf("NOT find Process In Sem Waiting Queue!\n");
    return NULL;
}

//3 ready queues
processNode * searchProcessInReadyQueues(int pid){
    void* targetNode = searchProcessInQueue(readyQueue_highP,pid);
    if (targetNode){
        //printf("find Process In readyQueue_highP!");
        return targetNode;
    }
    targetNode = searchProcessInQueue(readyQueue_midP,pid);
    if (targetNode){
        //printf("find Process In readyQueue_middleP!");
        return targetNode;
    }
    targetNode = searchProcessInQueue(readyQueue_lowP,pid);
    if (targetNode){
        //printf("find Process In readyQueue_lowP!");
        return targetNode;
    }
    return NULL;
}

processNode * searchProcessInQueue(List * queue, int pid){
    void* targetNode = NULL;
    if (List_first(queue)) {
        targetNode = List_search(queue, IsEqualToMyPID, &pid);
        if (targetNode != NULL) {
            //***if the node need to remove, should be done outside and immediately after calling this function!
            //because it is the only time you can confirm the current pointer still point to this to-be removed item in msgcenter
            return (processNode *) targetNode;
        }
    }else{
        //printf("search in an empty queue!");
    }
    return  NULL;
}

//2 blocked queue [receiverQueue and senderQueue]
processNode * searchProcessInBlockedQueues(int pid, int QueueType){
    void* targetNode = NULL;
    if (QueueType == QueueTypeBlockedReceiver ){
        targetNode = searchProcessInQueue(receiverWaitingQueue,pid);
        if (targetNode){
            //printf("find Process In receiverWaitingQueue!");
            return targetNode;
        }
    }else if(QueueType == QueueTypeBlockedSender){
        targetNode = searchProcessInQueue(senderWaitingQueue,pid);
        if (targetNode){
            //printf("find Process In senderWaitingQueue!");
            return targetNode;
        }
    }else if(QueueType == QueueTypeBlockedReceiverAndSender){
        targetNode = searchProcessInQueue(receiverWaitingQueue,pid);
        if (targetNode){
            //printf("find Process In receiverWaitingQueue!");
            return targetNode;
        }

        targetNode = searchProcessInQueue(senderWaitingQueue,pid);
        if (targetNode){
            //printf("find Process In senderWaitingQueue!");
            return targetNode;
        }
    }else{
        printf("SearchBlockQueueType INPUT ERROR!\n");
    }
    return NULL;
}

processNode * searchForTargetSender(int sentTOPID, enum SearchSenderFor searchFor){
    void* targetNode = NULL;
    if (List_first(senderWaitingQueue)) {
        if (searchFor == SearchSenderForEqualMsgPID){
            targetNode = List_search(senderWaitingQueue, IsEqualToSentToPID, &sentTOPID);
        }else if(searchFor == SearchSenderForEqualPID){
            targetNode = List_search(senderWaitingQueue, IsEqualToMyPID, &sentTOPID);
        }else{
            printf("SearchSenderFor input incorrect type! \n");
            return NULL;
        }
        if (targetNode != NULL) {
            //***if the node need to remove, should be done outside and immediately after calling this function!
            //because it is the only time you can confirm the current pointer still point to this to-be removed item in msgcenter
            return (processNode *) targetNode;
        }
    }else{
        printf("Search Sender Failed: Send Waiting Queue is  empty! No search result!");
        return NULL;
    }
    return NULL;
}


processNode *searchProcessInAllQueues(int pid, bool isInitProcessIncluded,bool isMeIncluded){
    void* targetNode = searchProcessInReadyQueues(pid);
    if (targetNode){
        return targetNode;
    }
    targetNode = searchProcessInSemWaitingQueues(pid);
    if (targetNode){
        return targetNode;
    }
    targetNode = searchProcessInBlockedQueues(pid,QueueTypeBlockedReceiverAndSender);
    if (targetNode){
        return targetNode;
    }else{
        if (isInitProcessIncluded == true && pid == initProcess -> PID){
            //printf("\n(ATTENSION: INIT PROCESS. TAKE CARE OF YOUR OPERATION! ) \n");
            return initProcess;
        }else if(isMeIncluded == true && pid == me -> PID){
            //printf("\n(ATTENSION: Current Running Process. TAKE CARE OF YOUR OPERATION!) \n");
            return me;
        }
        else{
            //printf("\nThe Process with pid = %d does not exist, please check! \n",pid);
        }
    }
    return NULL;
}

//only use for search MsgtoPID
processNode * searchPotentialSenderProcessInQueue(List * queue, int toPid){
    void* targetNode = NULL;
    if (List_first(queue)) {
        targetNode = List_search(queue, IsEqualToSentToPID, &toPid);
        if (targetNode != NULL) {
            //***if the node need to remove, should be done outside and immediately after calling this function!
            //because it is the only time you can confirm the current pointer still point to this to-be removed item in msgcenter
            return (processNode *) targetNode;
        }
    }else{
        //printf("search in an empty queue!");
    }
    return  NULL;
}

//find potential sender
processNode *searchProcessAllProgramWhoesMsgRecipientMatchPID(int msgReceiverPID){
    void* targetNode = searchPotentialSenderProcessInQueue(readyQueue_highP,msgReceiverPID);
    if (targetNode){
        return targetNode;
    }
    targetNode = searchPotentialSenderProcessInQueue(readyQueue_midP,msgReceiverPID);
    if (targetNode){
        return targetNode;
    }
    targetNode = searchPotentialSenderProcessInQueue(readyQueue_lowP,msgReceiverPID);
    if (targetNode){
        return targetNode;
    }
    targetNode = searchPotentialSenderProcessInQueue(senderWaitingQueue,msgReceiverPID);
    if (targetNode){
        return targetNode;
    }
    targetNode = searchPotentialSenderProcessInQueue(receiverWaitingQueue,msgReceiverPID);
    if (targetNode){
        return targetNode;
    }
    for (int i = 0; i < 5; ++i) {
        if (sem[i].semWaitingQueue){
            targetNode = searchPotentialSenderProcessInQueue(sem[i].semWaitingQueue,msgReceiverPID);
            if (targetNode){
                return targetNode;
            }
        }
    }
    if (initProcess->sendmsg.sentToPID == msgReceiverPID){
        return initProcess;
    }
    if (!isMEInitProcess() && me->sendmsg.sentToPID == msgReceiverPID){
        return me;
    }
    //printf("pid = %d",*msgReceiverPID);
    printf("Could not find corresponding process who has the same msg receier pid \n");
    return NULL;
}

//////////////// Compare //////////////////
bool IsEqualToMyPID(void* item, void* myPID){
    processNode *process = (processNode *)item;
    if (process->PID == *(int *)myPID){
        //printf("found - list_item: %d is bigger than arg: %d",*(int *)item,*(int *)myPID);
        return true;
    }else{
        return false;
    }
}
bool IsEqualToSentToPID(void* item, void* myPID){
    processNode *process = (processNode *)item;
    //printf("%d ----- %d --\n",process->sendmsg.sentToPID, *(int *)myPID);
    if (process->sendmsg.sentToPID == *(int *)myPID){
        //printf("found - list_item: %d is bigger than arg: %d",*(int *)item,*(int *)myPID);
        return true;
    }else{
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////  Validate   //////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
bool isValidCreate_C(int priority){
    if (priority == 0 || priority == 1 || priority == 2){
        return true;
    }
    else{
        printf("ERROR: priority has to be 0 || 1 || 2 !\n");
    }
    return false;
}

bool isKBInputMsgInValidLength(char*msg){
    return strlen(msg) <= maxMsgLength;
}

bool isValidSemID(int semID){
    bool valid = true;
    if(!(semID<=4 && semID>=0)){
        printf("Failure: Semaphore ID should be >=0 and <=4 ! \n");
        valid = false;
    }
    return valid;
}

bool isValidSemInput(int semID, int value){
    bool valid = true;
    valid = isValidSemID(semID);
    if (value<0){
        printf("Failure: Initial semaphore value should be >=0 ! \n");
        valid = false;
    }
    return valid;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////  Process Properties   //////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
void setProcessStateAndQueue(processNode*process, int processState, List* myQ){
    process->processState = processState;
    process->myQueue = myQ;
}

// This must be called IN Create() function  | CreateInitProcess
void setMsgNode(processNode * process, int sent_to_PID, char* msg){
    msgNode *msgNode = &process->sendmsg;
    msgNode->sentToPID = sent_to_PID;
    memset(&(msgNode->msg), 0, maxMsgLength * sizeof(char));
    if (msg!=NULL){
        strcpy(msgNode->msg, msg);
    }
}

void removeMsg(processNode* process){

    memset(process->sendmsg.msg , 0, maxMsgLength * sizeof(char));
    process->sendmsg.sentToPID = NULL_PID;
}

//1.clean cache  2.write in msg
void setMsg(processNode *process, char*msg, int toPID){
    removeMsg(process);
    strcpy(process->sendmsg.msg, msg);
    process->sendmsg.sentToPID = toPID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////  COMMON I/O   //////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
void clearRawString(char * raw){
    memset(raw, 0, maxMsgLength * sizeof(char));
    //raw[0]= '\0';
}
void clearRawInt(int *raw, int len){
    memset(raw, 0, len * sizeof(int));

}

void clear_cache(FILE *fp){
    int str;
    while( (str = fgetc(fp)) != EOF && str != '\n' );
}

bool readInt(int* raw, int len){
    int temp;//DO NOT DO 'int temp[len]';
    clearRawInt(raw,len);
    while ((scanf("%d",  raw))!= 1)
    {
        puts("Input error,please type only digits!\n");
        clear_cache(stdin);
        return false;
    }
    clear_cache(stdin);
    return true;
}

void readString(char * str){
    char raw[maxMsgLength];
    clearRawString(str);
    scanf("%[^\n]s",str);
    clear_cache(stdin);
}

char * stateToChar(int processState){
    switch(processState){
        case ProcessStateRunning:{
            return "Running";
        }
        case ProcessStateReady_HighPriority:{
            return "Ready Queue: High Priority";
        }
        case ProcessStateReady_MidPriority:{
            return "Ready Queue: Mid Priority";
        }
        case ProcessStateReady_LowPriority:{
            return "Ready Queue: Low Priority";
        }
        case ProcessStateSuperSuspend:{
            return " Init Process: Suspended";
        }
        case ProcessStateSenderBlocked:{
            return "Sender Queue: Blocked";
        }
        case ProcessStateReceiverBlocked:{
            return "Receiver Queue: Blocked";
        }
        case ProcessStateSEM_0_Blocked:{
            return "Semaphore Waiting Queue[0]: Blocked";
        }
        case ProcessStateSEM_1_Blocked:{
            return "Semaphore Waiting Queue[1]: Blocked";
        }
        case ProcessStateSEM_2_Blocked:{
            return "Semaphore Waiting Queue[2]: Blocked";
        }
        case ProcessStateSEM_3_Blocked:{
            return "Semaphore Waiting Queue[3]: Blocked";
        }
        case ProcessStateSEM_4_Blocked:{
            return "Semaphore Waiting Queue[4]: Blocked";
        }
        default:{
            printf("ERROR: invalid Input! \n");
            return " ";
        }

    }
}

void KBInputNote(){
    printf("\n====================================== NOTICE ======================================= \n");
    printf("Please choose one of the signal command below(1 signal char per time - Case Insensitive)\n");
    printf("====================================== COMMANDS ======================================= \n");
    printf("C(Create a Process) || F(Fork) || K(Kill any process) || E(Exit current process) \n");
    printf("Q(Quantum) || S(Send a message) || R(Receive a message) || Y(Reply a Message Sender) \n");
    printf("N(Create new Semaphore) || P(Semaphore P Function) || V(Semaphore V Function \n");
    printf("I(Print any process info) || T(Print total processes in this program)\n");
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////  Trivial   ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//return current one, then add 1 for next turn
int availablePID(){
    firstAvailablePID = firstAvailablePID + 1;
    return firstAvailablePID - 1;
}

bool thereIsNOProcessInReadyQueue(){
    int sum = List_count(readyQueue_highP) + List_count(readyQueue_midP) + List_count(readyQueue_lowP);
    return sum<=0;
}

bool hasNodeAvailable(){
    int nodeCount = 1; //current process
    if(!isMEInitProcess()){
        nodeCount++; //current running process
    }
    nodeCount = nodeCount + List_count(readyQueue_highP);
    nodeCount = nodeCount + List_count(readyQueue_midP);
    nodeCount = nodeCount + List_count(readyQueue_lowP);
    nodeCount = nodeCount + List_count(receiverWaitingQueue);
    nodeCount = nodeCount + List_count(senderWaitingQueue);
    //nodeCount = nodeCount + List_count(&msgCenter);
    for(int i = 0; i < 5; i++){
        if (sem[i].semWaitingQueue){
            nodeCount = nodeCount + List_count(sem[i].semWaitingQueue);
        }
    }
    if (nodeCount < LIST_MAX_NUM_NODES){
        return true;
    }else{
        return false;
    }
}

void releaseItem(void * pItem){
    if (pItem){
        free(pItem);
        pItem = NULL;
    }
}

