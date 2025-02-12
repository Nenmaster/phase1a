#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "phase1.h"
#include "src/usloss.h"
#include "tree_queue.h"
#include "usloss.h"

// might not be neccessary 

// global variables
pInfo processTable[MAXPROC];
pInfo *mainHead = NULL;
bool dead = false;
bool flag = false;
pInfo *currProc = NULL;

void printList(pInfo *proc, int parentpid);
int findNextProc();
// this func is meant to be used in the USLOSS_:wContextInit
// it should not return instead it will call startFunc which will
// return and pass that value to quit 
void uslossWrapper(void){
    int pid = getpid();
    pInfo *process = &processTable[pid % MAXPROC]; 
    int retval = process -> startFunc(process -> argument);
    process -> dead = true;

    //USLOSS_Halt(retval);
}

int testcase_mainWrapper(void *arg){
    int retval = testcase_main();
    USLOSS_Halt(retval);
    return retval;
}

void init(void) { 
    phase2_start_service_processes();
    phase3_start_service_processes();
    phase4_start_service_processes();
    phase5_start_service_processes(); 
    
    int tcm = spork("testcase_main", 
                    testcase_mainWrapper,
                    NULL, 
                    USLOSS_MIN_STACK, 
                    3);

    TEMP_switchTo(tcm);

    while(1){
        int status;
        int pid = join(&status);

        if(pid == -2){
            USLOSS_Console("Init has no more children\n");
            USLOSS_Halt(1);
        }
    }
}

// sets up init process but doesnt run it 
// places init into table as NOTRUNNING
// Manually initialize init proc with USLOSS_ContextInit
void phase1_init(){
    int prevPsr = USLOSS_PsrGet();
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }

    for(int i = 0; i < MAXPROC; ++i){
        processTable[i].pid = -1;
    }

    pInfo *initPrc = &processTable[1];
    initPrc -> name = "init";
    initPrc -> priority = 6;
    initPrc ->pid = 1;    
    initPrc -> dead = false;
    initPrc -> startFunc = NULL;
    initPrc -> argument = NULL;
    initPrc -> stackSize = USLOSS_MIN_STACK;
    initPrc -> stack = malloc(initPrc -> stackSize);
    if(initPrc -> stack == NULL){
        USLOSS_Console("Malloc failed in init proc buy more ram");
        USLOSS_Halt(1);
    }

    initPrc -> firstChildHead = NULL;
    initPrc -> nextChild = NULL;
    initPrc -> parent = NULL;
    initPrc -> parentPid = -1;

    currProc = NULL;

    //addToTree(initPrc, 0);

    USLOSS_ContextInit(&initPrc -> context,
                       initPrc -> stack,
                       initPrc -> stackSize,
                       NULL,
                       init);

    if(USLOSS_PsrSet(prevPsr) != 0) {
        USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
        USLOSS_Halt(1);
    }
}


// spork works by pointing a new processes startfunc to that process function
int spork(char *name, int (*startFunc)(void *), void *arg, int stacksize, int priority){
    if(stacksize < USLOSS_MIN_STACK) return -2;
    if(priority < 1 || priority > 5) return -1;
    if(name == NULL || strlen(name) >= MAXNAME) return -1;
    if(startFunc == NULL) return  -1;

    
    static int nextPid = 2;
    int currPid = nextPid++;
    int slot = currPid % MAXPROC;

    if (processTable[slot].pid != -1) return -1;

    pInfo *newProcess = &processTable[slot];
    newProcess -> name = strdup(name);
    newProcess -> pid = currPid;
    newProcess -> priority = priority; 
    newProcess -> startFunc = startFunc; 
    newProcess -> argument = arg;
    newProcess -> stackSize = stacksize;
    newProcess -> stack = malloc(newProcess -> stackSize);
    
    newProcess -> parent = NULL;
    newProcess -> firstChildHead = NULL;
    newProcess -> nextChild = NULL;

    if(newProcess -> stack == NULL){
        USLOSS_Console("malloc out of memory error");
        return -1;
    }
    newProcess -> dead = false;
    
    USLOSS_ContextInit(&processTable[slot].context,
                       processTable[slot].stack,
                       processTable[slot].stackSize,
                       NULL,
                       uslossWrapper);
    
    if(currProc == NULL){
        newProcess -> parent = &processTable[1];
        newProcess -> parentPid = 1;
    }else {
         newProcess -> parent = currProc;
         newProcess -> parentPid = currProc -> pid;
    }

    //printf("newProc name = %s\n", newProcess -> name);
    //printf("newProc pid = %d\n", newProcess -> pid);

    pInfo *parent = newProcess -> parent;
    //printf("parnet name = %s\n", parent -> name);
    //printf("parent pid = %d\n", parent -> pid);
    if(parent -> firstChildHead == NULL){
        parent -> firstChildHead = newProcess;
        //printf("firstChild name = %s\n", parent -> firstChildHead -> name);
        //printf("firstChild pid = %d\n", parent -> firstChildHead -> pid);
    }else {
        pInfo *child = parent -> firstChildHead;
        //printf("[spork else] child pid = %d\n", child -> pid);
        while(child -> nextChild != NULL && child -> pid != -1){
            child = child -> nextChild;
        }
        child -> nextChild = newProcess;
        //printf("child pid = %d\n", child -> pid);
    }

    //addToTree(&processTable[newProcess.pid % MAXPROC], newProcess.parentPid);

    return currPid;
}

int join(int *status){
    if(currProc == NULL || currProc -> firstChildHead == NULL || status == NULL){
        return -2;
    }

    pInfo *prev = NULL;
    pInfo *child = currProc -> firstChildHead;
    //printf("child name = %s\n", child -> name);
    //printf("child pid = %d\n", child -> pid);

    while(child != NULL){
        if(child -> dead ) {
            if(prev == NULL){
                currProc -> firstChildHead = child -> nextChild;
               // printf("currProc name = %s\n", currProc -> firstChildHead-> name);
               // printf("currProc pid = %d\n", currProc -> firstChildHead -> pid);
            }else {
                prev -> nextChild = child -> nextChild;
               // printf("prev name = %s and prev pid = %d\n", prev -> nextChild -> name, prev -> nextChild -> pid);
                if(child -> nextChild != NULL){
                   // printf("updating nextchild %s\n", child -> nextChild-> name);
                }
            }

            *status = child -> status; 
            //printf("status = %d\n", *status);
            int pid = child -> pid;
            //printf("child pid = %d\n", pid);

            free(child -> stack);
            processTable[pid % MAXPROC].pid = -1;
            //printf("join pid return = %d\n", pid);
            return pid;
        }
        prev = child;
        child = child -> nextChild;
        
    }

    return -1;
}

void quit_phase_1a(int status, int switchToPid){ 
    if(currProc == NULL){
        USLOSS_Halt(1);
    }

    if(currProc -> pid == 1){
        USLOSS_Halt(1);
    }

    currProc -> status = status; 
    currProc -> dead = true;

    if(switchToPid >= 0 && 
       switchToPid < MAXPROC && processTable[switchToPid % MAXPROC].pid != -1 && !processTable[switchToPid % MAXPROC].dead){
        TEMP_switchTo(switchToPid);
    }else {
        USLOSS_Halt(0);
    } 
}

int findNextProc(){
    for(int i = 0; i < MAXPROC; ++i){
        if(processTable[i].pid != -1 && !processTable[i].dead){
            return processTable[i].pid;
        }
    }
    return -1;
}

void quit(int status){
    //does nothing
}

int getpid(){
    if(currProc == NULL){
        return -1;
    }
    return currProc -> pid;
}

pInfo *getCurrProc(){
    return currProc;
}

void dumpProcesses(){
}

void TEMP_switchTo(int pid){
    if(pid < 0 ||pid >=  MAXPROC || processTable[pid % MAXPROC].pid == -1 || processTable[pid % MAXPROC].dead){
        USLOSS_Halt(1);
    }
    
    USLOSS_Context *old = (currProc == NULL) ? NULL : &currProc -> context;
    currProc = &processTable[pid % MAXPROC];

    USLOSS_ContextSwitch(old, &currProc -> context);
        //&processTable[parentPID % MAXPROC].context

        //USLOSS_ContextSwitch(&currProc -> context,
        //                     &processTable[pid % MAXPROC].context);


        //if(pid == 1){
        //    USLOSS_ContextSwitch(NULL, &processTable[pid % MAXPROC].context);
        //}else {
        //   (&processTable[parentPID % MAXPROC].context,
        //                     &processTable[pid % MAXPROC].context);

        //}
}



