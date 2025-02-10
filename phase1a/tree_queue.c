#include <stdio.h>
#include "phase1.h"
#include "usloss.h"
#include "tree_queue.h"


void addToTableList(pInfo **head, pInfo proc){
    if(*head == NULL) {
        *head = &proc;
        printf("[addToTableList] first if ran proc name = %s\n", proc.name);
        return;
    } else {
        pInfo *curr = *head;
        while(curr->next != NULL){
            curr = curr -> next;
        }
        printf("[addToTableList] second if ran proc name = %s\n", proc.name);
        curr -> next = &proc;
  }
}

