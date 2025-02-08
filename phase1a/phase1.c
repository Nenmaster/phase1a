#include <stdio.h>
#include <stdlib.h>
#include "phase1.h"
#include "src/usloss.h"
#include "usloss.h"
#include <string.h>
#define RUNNING 0
#define NOTRUNING -1
#define READY 2
int pId = 1;

typedef struct Node {
    struct Node *parent;
    struct Node *child;
    struct Node *nextChild;
}Node; 

typedef struct{
    // general process info 
    char *name;
    int priority;
    int pid;
    int parentPid;
    int status;
    int state; 
    // info to work with USLOSS
    USLOSS_Context new;
    void *stack;
    int stackSize;
    void (*fp)(void);
    int (*startFunc)(void*);
    void *argument;
}pInfo;


pInfo processTable[MAXPROC];

// this func is meant to be used in the USLOSS_ContextInit
// it should not return instead it will call startFunc which will
//
void uslossWrapper(void){
    int pid = getpid();
    pInfo process = processTable[pid % MAXPROC];
    printf("[uslossWrapper] pid = %d\n", pid);

    // this is allowing us to pass any type of func to uslosswrapper 
    // allowing us to use USLOSS_ContextInit
    // ill leave all the print statements so you can see were adding all counting process correctly just need to implement the rest of the function join and quit and creating children 
    int retval;
    if((retval = process.startFunc(process.argument)) == 0){
        quit_phase_1a(retval, process.parentPid);
    }
}

void init(void) { 
    phase2_start_service_processes();
    phase3_start_service_processes();
    phase4_start_service_processes();
    phase5_start_service_processes(); 

    pInfo test;
    test.argument = NULL;
    test.startFunc = (void *) testcase_main;
  
    int tcm = spork("testcase_main", test.startFunc,NULL, USLOSS_MIN_STACK, 2);
    // must call temp switch to according to 1a instrcutions for init
    TEMP_switchTo(tcm);
}

void phase1_init(){
    //set up but dont initialize initPrc 
    pInfo initPrc;
    initPrc.name = "init";
    initPrc.priority = 6;
    initPrc.pid = 1;    
    initPrc.state = NOTRUNING;
    initPrc.startFunc = (void *) init;
    initPrc.argument = NULL;
    initPrc.fp = uslossWrapper;
    initPrc.stackSize = USLOSS_MIN_STACK;
    initPrc.stack = malloc(initPrc.stackSize);

    // maybe should be made global 
    int slot = initPrc.pid % MAXPROC;
    //printf("slot = %d\n", slot);
    processTable[slot] = initPrc;
    
    // usloss call that allowed init to actually run
    USLOSS_ContextInit(&processTable[slot].new, processTable[slot].stack, processTable[slot].stackSize,NULL, uslossWrapper);
}

int spork(char *name, int (*func)(void *), void *arg, int stacksize, int priority){
    pInfo newProcess;
    newProcess.name = name;
    newProcess.priority = priority; 
    newProcess.startFunc = func; 
    newProcess.argument = arg;
    newProcess.stackSize = stacksize;
    newProcess.stack = malloc(newProcess.stackSize);

    newProcess.fp = uslossWrapper;
    printf("process name = %s\n", newProcess.name);

    pId++;
    newProcess.pid = pId;   
    printf("[spork] pid = %d\n", newProcess.pid);

    newProcess.parentPid = newProcess.pid - 1;
    newProcess.state = READY;
    printf("[spork] newProcess pid = %d\n", newProcess.pid);

    // inserts into table 
    int slot = newProcess.pid % MAXPROC;
    processTable[slot] = newProcess;
    
    USLOSS_ContextInit(&processTable[slot].new, processTable[slot].stack,
                       processTable[slot].stackSize,NULL, uslossWrapper);
  
    printf("[spork] pid = %d\n", newProcess.pid);
    return newProcess.pid;
}

int join(int *status){
    return 0;
}

void quit_phase_1a(int status, int switchToPid){
    // not started
    int *p = &status;
    join(p);
    printf("[quitphase1a] ran\n"); 
    exit(1);
}

void quit(int status){
    // not started
    int *p = &status;
    join(p);
    //printf("[quit] ran\n");
    exit(1);
}

int getpid(){ 
   pInfo currProcPid = processTable[pId % MAXPROC];
   return currProcPid.pid;
}

void dumpProcesses(){
}

void TEMP_switchTo(int pid){
    printf("[TEMP_switchTO]\n");
    printf("[TEMP_switchTO]this is func = %s\n", processTable[pid % MAXPROC].name);
    printf("[TEMP_switchTO]this is func pid = %d\n", processTable[pid % MAXPROC].pid);


    USLOSS_ContextSwitch(NULL, &processTable[pid % MAXPROC].new);
}


