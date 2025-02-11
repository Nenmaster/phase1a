#include <stdio.h>
#include "phase1.h"
#include "usloss.h"
#include "tree_queue.h"

void addToTree(pInfo *proc, int parentId){
    pInfo *parent = &processTable[parentId % MAXPROC];
    proc -> nextChild = parent -> firstChildHead;
    parent -> firstChildHead = proc;
}

//prints out table linked list 
void printList(pInfo *proc, int parentpid){
    pInfo *curr;
    pInfo *parentHead = &processTable[parentpid % MAXPROC];
    curr = parentHead -> firstChildHead;
    printf("[printlist] parent = %s\n", parentHead -> name);
    while(curr != NULL){
        printf("  -->");
        printf("child process = %s\n", curr-> name);
        curr = curr -> nextChild;
    }
    
}




//void addToTableList(pInfo **head, pInfo proc){
//    if(*head == NULL) {
//        *head = &proc;
//        printf("[addToTableList] first if ran proc name = %s\n", proc.name);
//        return;
//    } else {
//        pInfo *curr = *head;
//        while(curr->next != NULL){
//            curr = curr -> next;
//        }
//        printf("[addToTableList] second if ran proc name = %s\n", proc.name);
//        proc.parentPid = proc.pid - 1;
//        curr -> next = &proc;
//  }
//}

