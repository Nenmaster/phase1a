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
int pId = 1;
pInfo processTable[MAXPROC];
pInfo *mainHead = NULL;
bool dead = false;
int active = 0;
int notactive = -1;
bool flag = false;
pInfo *currProc = NULL;

void printList(pInfo *proc, int parentpid);

// this func is meant to be used in the USLOSS_ContextInit
// it should not return instead it will call startFunc which will
// return and pass that value to quit 
void uslossWrapper(void){
    int pid = getpid();
    pInfo process = processTable[pid % MAXPROC];

    //printf("[uslosswrapper] proces name = %s\n", process.name);
    int retval = process.startFunc(process.argument);
    //printf("[init] Testcase main terminated normally 5\n");
    USLOSS_Halt(retval);

}

void init(void) { 
    //printf("[init] hi again 1\n");
    phase2_start_service_processes();
    phase3_start_service_processes();
    phase4_start_service_processes();
    phase5_start_service_processes(); 
    //printf("[init] hi again 2\n");
    
    pInfo test;
    test.argument = NULL;
    test.startFunc = (void *) testcase_main;
    //printf("[init] hi again3 \n"); 

    int tcm = spork("testcase_main", 
                    test.startFunc,
                    NULL, 
                    USLOSS_MIN_STACK, 
                    2);

    // must call temp switch to according to 1a instrcutions for init
    //printf("[init] tcm done here 4\n");
    if(flag) {
         //printf("[init] Testcase main terminated normally 5\n");
         USLOSS_Halt(tcm);
    }else {
         USLOSS_ContextInit(&processTable[2 % MAXPROC].context,
                       processTable[2 % MAXPROC].stack,
                       processTable[2 % MAXPROC].stackSize,
                       NULL,
                       uslossWrapper);
         TEMP_switchTo(tcm);
    } 
}

// sets up init process but doesnt run it 
// places init into table as NOTRUNNING
// Manually initialize init proc with USLOSS_ContextInit
void phase1_init(){
    pInfo initPrc;
    initPrc.name = "init";
    initPrc.priority = 6;
    initPrc.pid = 1;    
    initPrc.dead = true;
    initPrc.state = notactive;
    initPrc.startFunc = (void *) init;
    initPrc.argument = NULL;
    initPrc.stackSize = USLOSS_MIN_STACK;
    initPrc.stack = malloc(initPrc.stackSize);

    // maybe should be made global 
    int slot = initPrc.pid % MAXPROC; 
    processTable[slot] = initPrc;
    currProc = &processTable[slot];
   // printf("slot = %d\n", slot);
    addToTree(&initPrc, 0);
    

    USLOSS_ContextInit(&processTable[slot].context,
                       processTable[slot].stack,
                       processTable[slot].stackSize,
                       NULL,
                       uslossWrapper);

    //addToTree(&initPrc, 0);
    // usloss call that allowed init to actually run
    //printf("[phase1] here i am\n");
}


// spork works by pointing a new processes startfunc to that process function
// we pass  
int spork(char *name, int (*func)(void *), void *arg, int stacksize, int priority){
    pInfo newProcess;
    newProcess.name = name;
    newProcess.priority = priority; 
    newProcess.startFunc = func; 
    newProcess.argument = arg;
    newProcess.stackSize = stacksize;
    newProcess.stack = malloc(newProcess.stackSize);
    newProcess.dead = false;

    //newProcess.parent = NULL;
    newProcess.firstChildHead = NULL;
    newProcess.nextChild = NULL;

    //printf("[spork] process name = %s\n", newProcess.name);
    
    newProcess.parentPid = getpid();
    //printf("parent pid = %d\n", newProcess.parentPid);
    pId++;
    newProcess.pid = pId;   
    //printf("[spork] pid = %d\n", newProcess.pid);

    newProcess.state = active;
    newProcess.status = -1;
    //printf("[spork] newProcess parent pid = %d\n", newProcess.parentPid);

    // inserts into table 
    int slot = newProcess.pid % MAXPROC;
    processTable[slot] = newProcess;

    addToTree(&processTable[slot], newProcess.parentPid);
    USLOSS_ContextInit(&processTable[slot].context,
                       processTable[slot].stack,
                       processTable[slot].stackSize,
                       NULL,
                       uslossWrapper);
    //printf("[spork]here i am\n"); 
    return newProcess.pid;
}

int join(int *status){
    int parentPid = getpid();
    pInfo *parent = currProc;
    bool hasChildren = false;

    if(status == NULL){
        return -3;
    }

    for(int i = 0; i < MAXPROC; ++i){
        if(processTable[i].parentPid == parentPid && processTable[i].state != notactive){
            hasChildren = true;
            break;
        }
    }

    if(!hasChildren){
        *status = -1;
        return -2;
    }


    for(int i = 0; i < MAXPROC; ++i){
        pInfo *proc = &processTable[i];

        if(proc -> parentPid == parentPid && proc -> dead && proc -> state != notactive){
            *status = proc ->status; 
            proc -> state = notactive;
            return proc -> pid;
        }
    }

    *status = -1;
    return  *status;
}

void quit_phase_1a(int status, int switchToPid){ 
    int pid = getpid();

    processTable[pid % MAXPROC].status = status; 
    processTable[pid % MAXPROC].dead = true;
   // printf("[quit] here i am 1\n"); 
    TEMP_switchTo(switchToPid); 
   // printf("[quit] here i am 2\n"); 
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
       // printf("[tempswitch] enter context swtich\n");
        int parentPID = processTable[pid % MAXPROC].parentPid;  
        
        currProc = &processTable[pid % MAXPROC];
        //printf("[temptswitch] pid == %d\n",parentPID);
        //printf("[temptoswitch] parent name = %s\n", processTable[parentPID % MAXPROC].name);
        //pInfo *parent = processTable[pid % MAXPROC].parent -> name;
        USLOSS_ContextSwitch(&processTable[parentPID].context,
                             &processTable[pid % MAXPROC].context);

}



