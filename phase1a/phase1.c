#include <stdio.h>
#include <stdlib.h>
#include "phase1.h"
//#include "src/usloss.h"
#include "usloss.h"
#define RUNNING 0
#define NOTRUNING -1
#define READY 2
int pid = 1;

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
}pInfo;


pInfo processTable[MAXPROC];

void wrapper(void){
    printf("[wrapper] ran");
    int currProcPid = getpid();
    printf("[wrapper] ran for pid %d\n]", currProcPid); 
    pInfo findproc = processTable[currProcPid % MAXPROC];
    findproc.fp();
}



void init(void){ 
  printf("[init] ran\n");

  phase2_start_service_processes();
  phase3_start_service_processes();
  phase4_start_service_processes();
  phase5_start_service_processes(); 

  //call spork 
  int tcm = spork("testcase_main", NULL, NULL, USLOSS_MIN_STACK , 3);
  tcm = getpid();
  printf("tcm is in spot %d\n",tcm);
}

void phase1_init(){
    //set up but dont initialize initPrc 
    pInfo initPrc;
    initPrc.name = "init";
    initPrc.priority = 6;
    initPrc.pid = 1;
    initPrc.state = NOTRUNING;
    initPrc.fp = init;
    initPrc.stackSize = USLOSS_MIN_STACK;
    initPrc.stack = malloc(initPrc.stackSize);

    // maybe should be made global 
    int slot = pid % MAXPROC;
    processTable[slot] = initPrc;

    // usloss call that allowed init to actually run
    USLOSS_ContextInit(&processTable[slot].new, processTable[slot].stack, processTable[slot].stackSize,NULL, &wrapper);

    


    //testing purposes 
    printf("[phase1_a] init inserted in to process table %d\n", slot);  
}

int spork(char *name, int (*func)(void *), void *arg, int stacksize, int priority){
    printf("[spork] ran\n");
    pInfo newProcess;
    newProcess.name = name;
    newProcess.priority = priority;
    newProcess.pid = pid;
    pid++;

    newProcess.fp = wrapper; 
    newProcess.state = READY;
    newProcess.stackSize = stacksize;
    newProcess.stack = malloc(newProcess.stackSize);

    // inserts into table 
    int slot = newProcess.pid % MAXPROC;
    processTable[slot] = newProcess;
    
    USLOSS_ContextInit(&processTable[slot].new, processTable[slot].stack, processTable[slot].stackSize,NULL, &wrapper);
  
    TEMP_switchTo(newProcess.pid);
    printf("temp to swtich ran for pid %d\n", newProcess.pid);
    return newProcess.pid;
}

int join(int *status){
    printf("[join] ran\n");
    return 0;
}

void quit_phase_1a(int status, int switchToPid){
    printf("[quit_phase_1a] ran\n");
    // not started
    int *p = &status;
    join(p);
    //printf("[quitphase1a] ran\n"); 
    exit(1);
}

void quit(int status){
    printf("[quit] ran\n");
    // not started
    int *p = &status;
    join(p);
    //printf("[quit] ran\n");
    exit(1);
}

int getpid(){ 
   printf("[getpid] ran\n");
   pInfo currProcPid = processTable[pid % MAXPROC];
   printf("[getpid] ran for process %d\n", currProcPid.pid);
   return currProcPid.pid;
}

void dumpProcesses(){
    printf("Not started\n");
}

void TEMP_switchTo(int pid){
    printf("[TEMP_switchTO]\n");
    printf("[TEMP_switchTO]this should be init = %s\n", processTable[pid % MAXPROC].name);
    printf("[TEMP_switchTO]this should be init = %d\n", processTable[pid % MAXPROC].pid);


    //printf("[initContext] ran\n");
    USLOSS_ContextSwitch(NULL, &processTable[pid % MAXPROC].new);
    //printf("[tempToSwitch] ran\n");
}


