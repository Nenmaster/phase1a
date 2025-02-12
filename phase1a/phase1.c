#include <stdint.h>
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
bool flag = false;
pInfo *currProc = NULL;

void printList(pInfo *proc, int parentpid);

// this func is meant to be used in the USLOSS_ContextInit
// it should not return instead it will call startFunc which will
// return and pass that value to quit 
void uslossWrapper(void){
    int pid = getpid();
    pInfo process = processTable[pid % MAXPROC];
    process.startFunc(process.argument);
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

    currProc = initPrc;

    addToTree(initPrc, 0);

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
int spork(char *name, int (*func)(void *), void *arg, int stacksize, int priority){
    pInfo newProcess;
    newProcess.name = name;
    newProcess.priority = priority; 
    newProcess.startFunc = func; 
    newProcess.argument = arg;
    newProcess.stackSize = stacksize;
    newProcess.stack = malloc(newProcess.stackSize);
    newProcess.dead = false;

    newProcess.firstChildHead = NULL;
    newProcess.nextChild = NULL;

    
    newProcess.parentPid = getpid();

    pId++;
    newProcess.pid = pId;   

    newProcess.status = -1;

    processTable[newProcess.pid % MAXPROC] = newProcess;

    addToTree(&processTable[newProcess.pid % MAXPROC], newProcess.parentPid);

    USLOSS_ContextInit(&processTable[newProcess.pid % MAXPROC].context,
                       processTable[newProcess.pid % MAXPROC].stack,
                       processTable[newProcess.pid % MAXPROC].stackSize,
                       NULL,
                       uslossWrapper);

    return newProcess.pid;
}

int join(int *status){
    int parentPid = getpid();
    bool hasChildren = false;

    if(status == NULL){
        return -3;
    }

    //for(int i = 0; i < MAXPROC; ++i){
    //    if(processTable[i].parentPid == parentPid && !processTable[i].dead){
    //        hasChildren = true;
    //        break;
    //    }
    //}

    //if(!hasChildren){
    //    *status = -1;
    //    return -2;
    //}


    for(int i = 0; i < MAXPROC; ++i){
        pInfo *proc = &processTable[i];

        if(proc -> parentPid == parentPid && proc -> dead){
            *status = proc ->status; 
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

    TEMP_switchTo(switchToPid); 
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
        int parentPID = processTable[pid % MAXPROC].parentPid;  
        currProc = &processTable[pid % MAXPROC];
        //&processTable[parentPID % MAXPROC].context

        //USLOSS_ContextSwitch(&currProc -> context,
        //                     &processTable[pid % MAXPROC].context);


        if(pid == 1){
            USLOSS_ContextSwitch(NULL, &processTable[pid % MAXPROC].context);
        }else {
            USLOSS_ContextSwitch(&processTable[parentPID % MAXPROC].context,
                             &processTable[pid % MAXPROC].context);

        }
}



