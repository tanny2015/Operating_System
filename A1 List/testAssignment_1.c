//
// Created by tanny on 9/27/2020.
//

#include "list.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
void print_list(List *pList);
void test_list_remove_free_helper(void* pItem);
void test_list_create();//ok ok
void test_list_remove_free();//ok ok ** DEBUG to see the node pool and head pool changes!
void test_list_add();//ok ok
void test_list_append();//ok ok
void test_list_insert();//ok ok
void test_list_prepend();//ok ok
void test_list_trim();//ok ok

//0930 test
void test_list_search();//ok
void test_list_concat();//ok
void test_list_count();//ok
void test_list_first();//ok
void test_list_last();//OK
void test_list_next();//OK
void test_list_prev();//ok
void test_list_current();//ok

  ///////////////////////////////////  NOTE FOR TESTING [START] ////////////////////////////////////////////
 /// Test the following function one by one rather than 'test all by once' which may lead to some errors //
////////////////////////////////////  NOTE FOR TESTING [END  ] ///////////////////////////////////////////

int main() {
//    test_list_create();
//    test_list_add();
//    test_list_insert();
//    test_list_append();
//    test_list_prepend();
//    test_list_remove_free();
//    test_list_trim();
//    test_list_search();
//    test_list_concat();
    test_list_count();
    test_list_first();
    test_list_last();
    test_list_next();
    test_list_prev();
    test_list_current();

    return 0;
}



// Returns the number of items in pList.
void test_list_count(){
    List *list = List_create();
    int a1 = 7;
    int a2 = 6;
    int a3 = 5;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_add(list,b1);
    print_list(list);
    printf("number of item in list: %d \n",List_count(list));
    List_add(list,b2);
    print_list(list);
    printf("number of item in list: %d \n",List_count(list));
    List_add(list,b3);
    print_list(list);
    printf("number of item in list: %d \n",List_count(list));

}

void test_list_first(){
    List *list = List_create();
    int a1 = 7;
    int a2 = 6;
    int a3 = 5;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_insert(list,b1);
    print_list(list);
    printf("first item in list: %d \n",*(int *)List_first(list));

    List_insert(list,b2);
    print_list(list);
    printf("first item in list: %d \n",*(int *)List_first(list));

    List_insert(list,b3);
    print_list(list);
    printf("first item in list: %d \n",*(int *)List_first(list));
}


void test_list_last(){
    List *list = List_create();
    int a1 = 7;
    int a2 = 6;
    int a3 = 5;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_insert(list,b1);
    print_list(list);
    printf("last item in list: %d \n",*(int *)List_last(list));

    List_insert(list,b2);
    print_list(list);
    printf("last item in list: %d \n",*(int *)List_last(list));

    List_insert(list,b3);
    print_list(list);
    printf("last item in list: %d \n",*(int *)List_last(list));
}



void test_list_next(){
    List *list = List_create();
    int a1 = 7;
    int a2 = 6;
    int a3 = 5;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_insert(list,b1);
    print_list(list);
    /*Node *next = List_next(list);
    if (next != NULL){
        printf("next item in list: %d \n",*(int *)next);
    }*/

    List_insert(list,b2);
    print_list(list);
    Node *next = List_next(list);
    if (next != NULL){
        printf("next item in list: %d \n",*(int *)next);
    }

    List_insert(list,b3);
    print_list(list);
    next = List_next(list);
    if (next != NULL){
        printf("next item in list: %d \n",*(int *)next);
    }
}


void test_list_prev(){
    List *list = List_create();
    int a1 = 7;
    int a2 = 6;
    int a3 = 5;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_add(list,b1);
    print_list(list);
    /*Node *pre = List_prev(list);
    if (pre != NULL){
        printf("previous item in list: %d \n",*(int *)pre);
    }*/
    List_add(list,b2);
    print_list(list);
    Node *pre = List_prev(list);
    if (pre != NULL){
        printf("previous item in list: %d \n",*(int *)pre);
    }

    List_add(list,b3);
    print_list(list);
    pre = List_prev(list);
    if (pre != NULL){
        printf("previous item in list: %d \n",*(int *)pre);
    }
}

void test_list_current(){
    List *list = List_create();
    int a1 = 7;
    int a2 = 6;
    int a3 = 5;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_add(list,b1);
    print_list(list);
    printf("current item in list: %d \n",*(int *)List_curr(list));

    List_add(list,b2);
    print_list(list);
    printf("current item in list: %d \n",*(int *)List_curr(list));

    List_add(list,b3);
    print_list(list);
    printf("current item in list: %d \n",*(int *)List_curr(list));
}

void test_list_concat(){
    List *list = List_create();
    List *list2 = List_create();
    int a1 = 7;
    int a2 = 6;
    int a3 = 5;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_add(list,b1);
    List_add(list2,b2);
    List_add(list2,b3);

    //printNodeStack();
    //printListStack();
    print_list(list);

    printf("-- List concat - 1 --\n");
    List_concat(list,list2);
    //printNodeStack();
    //printListStack();
    print_list(list);

}


bool isBiggerThanArg(void* pItem, void* pComparisonArg){
    if (*(int *)pItem > *(int *)pComparisonArg){
        printf("found - list_item: %d is bigger than arg: %d",*(int *)pItem,*(int *)pComparisonArg);
        return true;
    }else{
        return false;
    }
}

void test_list_search(){
    List *list = List_create();
    int a1 = 7;
    int a2 = 6;
    int a3 = 5;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    /*List_add(list,b1);
    List_add(list,b2);
    List_add(list,b3);*/
    List_insert(list,b1);
    List_insert(list,b2);
    List_insert(list,b3);
    print_list(list);

    printf("-- List search - 1 --\n");
    int arg = 1;
    void* target = List_search(list,isBiggerThanArg,&arg);
    if (target != NULL){
        printf("\n SUCCESS! found value = %d \n",*(int *)target);
        print_list(list);
    }

    printf("-- List search - 2 --\n");
    arg = 6;
    target = List_search(list,isBiggerThanArg,&arg);
    if (target != NULL){
        printf("\n SUCCESS! found value = %d \n",*(int *)target);
        print_list(list);
    }

    printf("-- List search - 3 --\n");
    arg = 7;
    target = List_search(list,isBiggerThanArg,&arg);
    if (target != NULL){
        printf("\n SUCCESS! found value = %d \n",*(int *)target);
        print_list(list);
    }


}

void test_list_trim(){
    List *list = List_create();
    int a1 = 5;
    int a2 = 6;
    int a3 = 7;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_add(list,b1);
    List_add(list,b2);
    List_add(list,b3);
    print_list(list);

    printf("-- List trim  --\n");
    List_trim(list);
    print_list(list);

    List_trim(list);
    print_list(list);

    List_trim(list);
    print_list(list);
    //edge test
    List_trim(list);
    print_list(list);
}

void test_list_remove_free(){

    printf("--------------1. edge test  -----------\n");
    List *list2 = List_create();
    /*print_list(list2);
    printListStack();
    printNodeStack();*/

    List_remove(list2);
    /*print_list(list2);
    printListStack();
    printNodeStack();*/


    printf("-------------- 2 -----------\n");
    List *list = List_create();
    int a1 = 5;
    int a2 = 6;
    int a3 = 7;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_add(list,b1);
    print_list(list);
    //printNodeStack();

    List_add(list,b2);
    print_list(list);
    //printNodeStack();

    List_remove(list);
    print_list(list);
    //printNodeStack();

    printf("-------------- 3 -----------\n");
    List_add(list,b3);
    print_list(list);
    //printNodeStack();

    List_remove(list);
    print_list(list);
    //printNodeStack();


    printf("-------------- List_Free -- 1  -----------\n");
    //extra test -- List_Free -- DEBUG to see List_Pool dynamic
    List_free(list,test_list_remove_free_helper);
    print_list(list);
    //printNodeStack();
    //printListStack();

    printf("-------------- List_Free -- 2 empty  -----------\n");
    List_free(list2,test_list_remove_free_helper);
    print_list(list2);
    //printNodeStack();
    //printListStack();

    printf("-------------- List_Free -- 3 free twice  -----------\n");
    List_free(list2,test_list_remove_free_helper);
    print_list(list2);
    //printNodeStack();
    //printListStack();
}
void test_list_remove_free_helper(void* pItem){
    printf(" %d -- was removed ! \n", *(int *)pItem);
}


void test_list_prepend(){
    List *list = List_create();
    print_list(list);
    int a1 = 5;
    int a2 = 6;
    int a3 = 7;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_prepend(list,b1);
    print_list(list);

    List_prepend(list,b2);
    print_list(list);

    List_prepend(list,b3);
    print_list(list);

    //edge test okay
    printf("-- edge test --\n");
    int a4 = 7;
    void *b4 = &a4;
    if(List_prepend(list,b4) == 0){
        print_list(list);
    }
}

void test_list_append(){
    List *list = List_create();
    int a1 = 5;
    int a2 = 6;
    int a3 = 7;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_append(list,b1);
    print_list(list);
    List_append(list,b2);
    print_list(list);
    List_append(list,b3);
    print_list(list);

    //edge test okay
    printf("-- edge test --\n");
    int a4 = 7;
    void *b4 = &a4;
    if (List_append(list,b4) == 0){
        print_list(list);
    }

}

void test_list_insert(){
    List *list = List_create();
    int a1 = 5;
    int a2 = 6;
    int a3 = 7;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    List_insert(list,b1);
    print_list(list);
    List_insert(list,b2);
    print_list(list);
    List_insert(list,b3);
    print_list(list);

    //edge test okay
    printf("-- edge test --\n");
    int a4 = 7;
    void *b4 = &a4;
    if (List_insert(list,b4) == 0){
        print_list(list);
    };

}


void test_list_add(){
    List *list = List_create();
    int a1 = 5;
    int a2 = 6;
    int a3 = 7;
    int a4 = 12;
    void *b1 = &a1;
    void *b2 = &a2;
    void *b3 = &a3;
    void *b4 = &a4;
    List_add(list,b1);
    print_list(list);

    List_add(list,b2);
    print_list(list);
    //printNodeStack();

    /*List_remove(list);
    print_list(list);
    printNodeStack();*/

    List_add(list,b4);
    print_list(list);

    //edge test okay
    /*printf("-- edge test ---\n");
    int a5 = 65;
    void *b5 = &a5;
    if(List_add(list,b5)==0){
        print_list(list);
    };*/
}


void test_list_create(){
    List* l1 = List_create();
    //print_list(l1);
    //printListStack();

    List* l2 = List_create();
    //print_list(l2);
    //printListStack();

    List* l3 = List_create();
    //print_list(l3);
    //printListStack();

    //edge test
    //printf("-- l4-- \n");
    List* l4 = List_create();
    //printListStack();
    print_list(l4);
}


//int
void print_list(List *pList){
    if (pList == NULL){
        printf("pList is NULL！\n");
    } else{
        if (pList->nodeCount == 0){
            printf("There is NO node\n");
        }else{
            Node * temp = pList->pHeadNode;
            printf("List Print： \n");
            while (temp!=NULL){
                printf("%d -- ",*(int*)(temp->pData));//
                temp = temp -> pNextNode;
            }
            printf("#node： %d \n", pList->nodeCount);
            if (pList->pCurrentNode != NULL){
                printf("currentNode = : %d \n", *(int*)(pList->pCurrentNode->pData));//
            }else{
                printf("currentNode = : NULL STATE=%d \n", pList->currentNodeState);
            }
        }
    }
}


