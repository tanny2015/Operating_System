#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "list.h"
static void setCurrentNode(List* plist, Node *newCurrentNode, enum ListOutOfBounds newCurrentState);
static void cleanNodeForPool(Node *node);
static void cleanListForPool(List *list);
static void initializePools();
static void addToNodePool(Node *node);
static void addToListPool(List *list);
static Node* getNode();
static List* getList();

//node pool(stack)
typedef struct {
    Node * top;
}NodePoolS;

//list head pool(stack)
typedef struct {
    List * top;
}listPoolS;

static Node nodes[LIST_MAX_NUM_NODES];
static List lists[LIST_MAX_NUM_HEADS];
static bool firstCreateList=true;
static NodePoolS nodePool;
static listPoolS listPool;

//to boost space efficiency. can not be moved or deleted manually(NOT FOR USE)
static Node nodePoolTailFixedLayer;
static List listPoolTailFixedLayer;

/////////////////////////////////////////    DATA INITIATION     /////////////////////////////////////////
// Makes a new, empty list, and returns its reference on success.
// Returns a NULL pointer on failure.
List* List_create(){
    if(firstCreateList == true){
        initializePools();
        firstCreateList = false;
    }
    List *newList = getList();
    if (newList != NULL){
        return newList;
    }else{
        return NULL;
    }
}

/////////////////////////////////////////    DATA DISPLAY     /////////////////////////////////////////
// Returns the number of items in pList.
int List_count(List* pList){
    return pList->nodeCount;
}

// Returns a pointer to the current item in pList.
void* List_curr(List* pList){
    if (pList->pCurrentNode != NULL){
        return pList->pCurrentNode->pData;
    } else{
        return NULL;
    }
}

// Returns a pointer to the first item in pList and makes the first item the current item.
// Returns NULL and sets current item to NULL if list is empty.
void* List_first(List* pList){
    if (pList->nodeCount == 0){
        setCurrentNode(pList,NULL,LIST_OOB_EMPTY);
        return NULL;
    }else{
        Node *firstNode = pList->pHeadNode;//head must not be null
        setCurrentNode(pList,firstNode,LIST_OOB_NORMAL_RANGE);
        return firstNode->pData;
    }
}

// Returns a pointer to the last item in pList and makes the last item the current item.
// Returns NULL and sets current item to NULL if list is empty.
void* List_last(List* pList){
    if (pList->nodeCount == 0){
        setCurrentNode(pList,NULL,LIST_OOB_EMPTY);
        return NULL;
    }else{
        Node* lastNode = pList->pTailNode; //last node must not be null
        setCurrentNode(pList,lastNode,LIST_OOB_NORMAL_RANGE);
        return lastNode->pData;
    }
}


/* PARAM: A list(List pointer)
 * Return: The item in current node(after stepping into the next node)
 * Intro: Advances pList's current item by one, and returns a pointer to the new current item.
 * If this operation advances the current item beyond the end of the pList, a NULL pointer
 * is returned and the current item is set to be beyond end of pList.
 * */
void* List_next(List* pList){
    //LIST_OOB_EMPTY
    if (pList->nodeCount == 0){
        return NULL;
    }
    //LIST_OOB_END & LIST_OOB_START
    if(pList->pCurrentNode == NULL){
        if (pList->currentNodeState==LIST_OOB_END){
            return NULL;
        }else{
            //LIST_OOB_START
            pList->pCurrentNode = pList->pHeadNode;//headNode must not be null
            return pList->pCurrentNode->pData;
        }
    }
    //LIST_OOB_NORMAL_RANGE
    else{
        Node *nextNode = pList->pCurrentNode->pNextNode;
        if (nextNode != NULL){
            setCurrentNode(pList,nextNode,LIST_OOB_NORMAL_RANGE);
            return nextNode->pData;
        }else{
            setCurrentNode(pList,NULL,LIST_OOB_END);
            return NULL;
        }

    }
}

// Backs up pList's current item by one, and returns a pointer to the new current item.
// If this operation backs up the current item beyond the start of the pList, a NULL pointer
// is returned and the current item is set to be before the start of pList.
void* List_prev(List* pList){
    //LIST_OOB_EMPTY
    if (pList->nodeCount == 0){
        return NULL;
    }
    //LIST_OOB_END & LIST_OOB_START
    if(pList->pCurrentNode == NULL){
        if (pList->currentNodeState==LIST_OOB_START){
            return NULL;
        }else{
            //LIST_OOB_END
            pList->pCurrentNode = pList->pTailNode;//tailNode must not be null
            return pList->pCurrentNode->pData;
        }
    }
    //LIST_OOB_NORMAL_RANGE
    else{
        Node *preNode = pList->pCurrentNode->pPreNode;
        if (preNode != NULL){
            setCurrentNode(pList,preNode,LIST_OOB_NORMAL_RANGE);
            return preNode->pData;
        }else{
            setCurrentNode(pList,NULL,LIST_OOB_START);
            return NULL;
        }

    }
}


/////////////////////////////////////////    DATA GET     /////////////////////////////////////////
// Adds the new item to pList directly after the current item, and makes item the current item.
// If the current pointer is before the start of the pList, the item is added at the start. If
// the current pointer is beyond the end of the pList, the item is added at the end.
// Returns 0 on success, -1 on failure.
int List_add(List* pList, void* pItem){
    Node *newNode = getNode();//
    if (newNode != NULL){
        newNode->pData = pItem;
        if (pList->nodeCount == 0){
            pList->pHeadNode = newNode;
            pList->pTailNode = newNode;
        }else{
            if (pList->currentNodeState == LIST_OOB_START){
                newNode->pNextNode = pList -> pHeadNode; //headNode must not be NULL as nodeCount>0
                pList -> pHeadNode -> pPreNode = newNode;
                pList->pHeadNode = newNode;
            }
            else if(pList->currentNodeState == LIST_OOB_END){
                newNode->pPreNode = pList -> pTailNode; //tailNode must not be NULL as nodeCount>0
                pList -> pTailNode -> pNextNode = newNode;
                pList -> pTailNode = newNode;
            }
            else{
                newNode->pNextNode = pList -> pCurrentNode -> pNextNode;
                if (newNode->pNextNode != NULL){
                    newNode->pNextNode -> pPreNode = newNode;
                } else{
                    pList->pTailNode = newNode;//
                }
                newNode->pPreNode = pList -> pCurrentNode;
                pList -> pCurrentNode -> pNextNode = newNode;
            }
        }
        setCurrentNode(pList,newNode,LIST_OOB_NORMAL_RANGE);
        pList -> nodeCount = pList -> nodeCount + 1;
        return LIST_SUCCESS;
    }else{
        return LIST_FAIL;
    }

}

// Adds item to pList directly before the current item, and makes the new item the current one.
// If the current pointer is before the start of the pList, the item is added at the start.
// If the current pointer is beyond the end of the pList, the item is added at the end.
// Returns 0 on success, -1 on failure.
int List_insert(List* pList, void* pItem){
    Node *newNode = getNode();//
    if (newNode != NULL){
        newNode->pData = pItem;
        if (pList->nodeCount == 0){
            pList->pHeadNode = newNode;
            pList->pTailNode = newNode;
        }else{
            if (pList->currentNodeState == LIST_OOB_START){
                newNode->pNextNode = pList -> pHeadNode; //headNode must not be NULL as nodeCount>0
                pList -> pHeadNode -> pPreNode = newNode;
                pList->pHeadNode = newNode;
            }
            else if(pList->currentNodeState == LIST_OOB_END){
                newNode->pPreNode = pList -> pTailNode; //tailNode must not be NULL as nodeCount>0
                pList -> pTailNode -> pNextNode = newNode;
                pList -> pTailNode = newNode;
            }
            else{
                newNode->pPreNode = pList -> pCurrentNode -> pPreNode;//currentNode must not be NULL
                if (newNode->pPreNode != NULL){
                    newNode->pPreNode -> pNextNode = newNode;
                } else{
                    pList->pHeadNode = newNode;//
                }
                newNode->pNextNode = pList -> pCurrentNode;
                pList -> pCurrentNode -> pPreNode = newNode;
            }
        }
        setCurrentNode(pList,newNode,LIST_OOB_NORMAL_RANGE);
        pList -> nodeCount = pList -> nodeCount + 1;
        return LIST_SUCCESS;
    }else{
        return LIST_FAIL;
    }
}

// Adds item to the end of pList, and makes the new item the current one.
// Returns 0 on success, -1 on failure.
int List_append(List* pList, void* pItem){
    Node *newNode = getNode();//
    if (newNode != NULL){
        newNode->pData = pItem;
        if (pList->nodeCount == 0){
            pList->pHeadNode = newNode;
            pList->pTailNode = newNode;
        }else {
            newNode->pPreNode = pList->pTailNode; //tailNode must not be null
            pList->pTailNode->pNextNode = newNode;
            pList->pTailNode = newNode;
        }
        setCurrentNode(pList,newNode,LIST_OOB_NORMAL_RANGE);
        pList -> nodeCount = pList -> nodeCount + 1;
        return LIST_SUCCESS;
    }else{
        return LIST_FAIL;
    }
}

// Adds item to the front of pList, and makes the new item the current one.
// Returns 0 on success, -1 on failure.
int List_prepend(List* pList, void* pItem){
    Node *newNode = getNode();//
    if (newNode != NULL){
        newNode->pData = pItem;
        if (pList->nodeCount == 0){
            pList->pHeadNode = newNode;
            pList->pTailNode = newNode;
        }else {
            newNode->pNextNode = pList->pHeadNode; //headNode must not be null
            pList->pHeadNode->pPreNode = newNode;
            pList->pHeadNode = newNode;
        }
        setCurrentNode(pList,newNode,LIST_OOB_NORMAL_RANGE);
        pList -> nodeCount = pList -> nodeCount + 1;
        return LIST_SUCCESS;
    } else{
        return LIST_FAIL;
    }
}


/////////////////////////////////////////    DATA REMOVE     /////////////////////////////////////////

// Adds pList2 to the end of pList1. The current pointer is set to the current pointer of pList1.
// pList2 no longer exists after the operation; its head is available
// for future operations.
void List_concat(List* pList1, List* pList2){
    if (pList2->nextListInPool != NULL){
        printf("CAN NOT FREE SAME MEMORY FOR TWICE(List)! \n");
        return;
    }

    Node* plist2HeadNode = pList2->pHeadNode;
    Node* plist2TailNode = pList2->pTailNode;

    if (pList1->nodeCount == 0){
        pList1->pHeadNode = plist2HeadNode;
        pList1->pTailNode = plist2TailNode;
        //no need to set current node and its state in plist1
    }else{
        //plist1->tailNode must not be NULL caz nodeCount of p1 is not 0
        pList1->pTailNode->pNextNode = plist2HeadNode;
        if (plist2HeadNode!=NULL){
            plist2HeadNode->pPreNode = pList1->pTailNode;
        }
        if (pList2->pTailNode!=NULL){
            pList1->pTailNode = plist2TailNode;
        }
        //no need to set current node and its state in plist1
    }
    pList1->nodeCount = pList1->nodeCount + pList2->nodeCount;
    addToListPool(pList2);
}

// Delete pList. pItemFreeFn is a pointer to a routine that frees an item.
// It should be invoked (within List_free) as: (*pItemFreeFn)(itemToBeFreedFromNode);
// pList and all its nodes no longer exists after the operation; its head and nodes are
// available for future operations.
typedef void (*FREE_FN)(void* pItem);
void List_free(List* pList, FREE_FN pItemFreeFn){
    if (pList->nextListInPool != NULL){
        printf("CAN NOT FREE SAME MEMORY FOR TWICE(List)! \n");
        return;
    }
    if (pList->nodeCount == 0){
        addToListPool(pList);//
    } else{
        Node *pIter =  pList->pHeadNode;
        while (pIter != NULL){
            Node *nodeToBeFree = pIter;
            pIter = nodeToBeFree->pNextNode;
            void *itemToBeFreedFromNode = nodeToBeFree->pData;
            addToNodePool(nodeToBeFree);//

            (*pItemFreeFn)(itemToBeFreedFromNode);
        }
        addToListPool(pList);//
    }
}


// Return last item and take it out of pList. Make the new last item the current one.
// Return NULL if pList is initially empty.
void* List_trim(List* pList){
    if (pList->nodeCount == 0) {
        return NULL;
    }else{
        Node *lastNode = pList->pTailNode; //must not be null
        Node *lastPre = lastNode->pPreNode;
        if(lastPre != NULL){
            lastPre->pNextNode = NULL;
            pList->pTailNode = lastPre;
            setCurrentNode(pList,lastPre,LIST_OOB_NORMAL_RANGE);
        } else{
            //empty list after remove the last node
            pList->pHeadNode = NULL;
            pList->pTailNode = NULL;
            setCurrentNode(pList,NULL,LIST_OOB_EMPTY);
        }
        pList->nodeCount = pList->nodeCount - 1;
        printf("data: %d is removed! \n", *(int *)lastNode->pData);
        addToNodePool(lastNode);//
        return lastNode->pData;
    }
}

// Return current item and take it out of pList. Make the next item the current one.
// If the current pointer is before the start of the pList, or beyond the end of the pList,
// then do not change the pList and return NULL.
//If current is pointing to the last node when you do the List_remove(), then return its item and move current beyond the end of the list.
void* List_remove(List* pList){
    //1. currentNode == NULL
    if (pList->nextListInPool != NULL){
        printf("CAN NOT FREE SAME MEMORY FOR TWICE(List)! \n");
        return NULL;
    }

    if (pList->nodeCount == 0 || pList->currentNodeState == LIST_OOB_START || pList->currentNodeState == LIST_OOB_END){
        return NULL;
    }
    //2. currentNode != NULL
    else{
        Node *nodeToBeRemoved = pList->pCurrentNode;
        Node *pre = nodeToBeRemoved->pPreNode;
        Node *next = nodeToBeRemoved->pNextNode;

        if (pre!=NULL){
            pre->pNextNode = next;
        }else{
            pList->pHeadNode = next;
        }
        if (next!=NULL){
            setCurrentNode(pList,next,LIST_OOB_NORMAL_RANGE);
            next->pPreNode = pre;
        }else{
            setCurrentNode(pList,next,LIST_OOB_END);
            pList->pTailNode = pre;
        }
        pList -> nodeCount = pList -> nodeCount - 1;
        void *data = nodeToBeRemoved->pData;
        addToNodePool(nodeToBeRemoved);//
        printf("data: %d is removed! \n", *(int *)data);
        return data;
    }
}


/////////////////////////////////////////    DATA SEARCH     /////////////////////////////////////////
// Search pList, starting at the current item, until the end is reached or a match is found.
// In this context, a match is determined by the comparator parameter. This parameter is a
// pointer to a routine that takes as its first argument an item pointer, and as its second
// argument pComparisonArg. Comparator returns 0 if the item and comparisonArg don't match,
// or 1 if they do. Exactly what constitutes a match is up to the implementor of comparator.
//
// If a match is found, the current pointer is left at the matched item and the pointer to
// that item is returned. If no match is found, the current pointer is left beyond the end of
// the list and a NULL pointer is returned.
//
// If the current pointer is before the start of the pList, then start searching from
// the first node in the list (if any).

typedef bool (*COMPARATOR_FN)(void* pItem, void* pComparisonArg);
void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg){
    if (pList->nodeCount == 0){
        setCurrentNode(pList,NULL,LIST_OOB_EMPTY);
        return NULL;
    }
    else{
        Node *pIter = pList->pCurrentNode;
        Node *targetNode = NULL;
        while (pIter != NULL){
            //1.find target
            if ((*pComparator)(pIter->pData, pComparisonArg) == true){
                targetNode = pIter;
                break;
            }
            //2.not find
            else{
                pIter = pIter->pNextNode;
                continue;
            }
        }
        //1. FOUND
        if (targetNode != NULL){
            setCurrentNode(pList,targetNode,LIST_OOB_NORMAL_RANGE);
            return targetNode->pData;
        }
        //2. NOT FOUND
        else{
            setCurrentNode(pList,NULL,LIST_OOB_END);
            return NULL;
        }
    }
}

/////////////////////////////////////////    DATA POOL     /////////////////////////////////////////
////////////////// 1. Data Input [all cleaning routine included]
static void initializePools(){
    //NODES
    nodePoolTailFixedLayer.pNextNode = NULL;
    nodePool.top = &nodePoolTailFixedLayer;
    for (int i=0; i<LIST_MAX_NUM_NODES; i++){
        nodes[i].nextNodeInPool = NULL;//
        addToNodePool(&nodes[i]);//
    }
    //LISTS
    listPoolTailFixedLayer.nextListInPool = NULL;
    listPool.top = &listPoolTailFixedLayer;
    for (int i=0; i<LIST_MAX_NUM_HEADS; i++){
        lists[i].nextListInPool=NULL;//
        addToListPool(&lists[i]);//
    }
}

static void addToNodePool(Node *node) {
    if (node->nextNodeInPool != NULL){
        printf("CAN NOT FREE SAME MEMORY FOR TWICE(NODE)! \n");
        return;
    }
    else{
        cleanNodeForPool(node);
        node->nextNodeInPool = nodePool.top;//
        nodePool.top = node;
        return;
    }
}

static void addToListPool(List *list) {
    if (list->nextListInPool != NULL){
        printf("CAN NOT FREE SAME MEMORY FOR TWICE(LIST)! \n");
        return;
    }
    else{
        cleanListForPool(list);
        list->nextListInPool = listPool.top;//
        listPool.top = list;
        return;
    }
}



////////////////// 2. DATA OUTPUT
static Node* getNode(){
    //empty stack(only tail layer inside)
    if (nodePool.top->nextNodeInPool == NULL){
        return NULL;
    }
        //not empty
    else{
        Node *newNode = nodePool.top;
        nodePool.top = nodePool.top->nextNodeInPool;
        //clean pool link
        newNode->nextNodeInPool = NULL;//
    }
}


static List* getList(){
    //empty stack(only tail layer inside)
    if (listPool.top->nextListInPool == NULL){
        return NULL;
    }
        //not empty
    else{
        List *newList = listPool.top;
        listPool.top = listPool.top->nextListInPool;
        //clean pool link
        newList->nextListInPool = NULL;//
    }
}




////////////////// Others
static void setCurrentNode(List* plist, Node *newCurrentNode, enum ListOutOfBounds newCurrentState){
    if (plist!=NULL){
        plist->pCurrentNode = newCurrentNode;
        plist->currentNodeState = newCurrentState;
    }else{
        printf("pList should not be NULL!");
    }
}
static void cleanNodeForPool(Node *node) {
    node->pPreNode=NULL;
    node->pNextNode=NULL;
    node->pData = NULL;
}


static void cleanListForPool(List *list) {
    setCurrentNode(list,NULL,LIST_OOB_EMPTY);
    list->pHeadNode = NULL;
    list->pTailNode = NULL;
    list->nodeCount = 0;
}

/*static void printListStack(){
    //printf("------ Print list pool ---- \n");
    if (!listPool.top){
        printf("DON NOT USE BEFORE INITIATION!! \n");
        return;
    }

    if (listPool.top->nextListInPool == NULL) {
        printf("empty list pool \n");
        return;
    }

    //printf("LISTS：\n");
    List* itList = listPool.top;
    while (itList->nextListInPool != NULL) {
        printf("%p " , itList);
        itList = itList->nextListInPool;
    }
}*/

/*
static void printNodeStack() {
    //printf("------ Print node pool ---- \n");
    if (!nodePool.top){
        printf("DON NOT USE BEFORE INITIATION!! \n");
        return;
    }

    if (nodePool.top->nextNodeInPool == NULL) {
        printf("empty node pool \n");
        return;
    }

    //printf("NODES：\n");
    Node* itNode = nodePool.top;
    while (itNode->nextNodeInPool != NULL) {
        printf("%p " , itNode);
        itNode = itNode->nextNodeInPool;
    }
}*/
