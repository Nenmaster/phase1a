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
    int (*tcm)(void);
}pInfo;


pInfo processTable[MAXPROC];

 void wrapper(void){
    int currProcPid = getpid();
    pInfo findproc = processTable[currProcPid % MAXPROC];
    findproc.fp();
}

pInfo findPID(int pid){
    pInfo targetProc;
    for(int i = 0; i < MAXPROC; i++){
        if(processTable[i].pid == pid){
            targetProc = processTable[i];
        }
    }
    if(targetProc.tcm){
        quit_phase_1a(targetProc.status, targetProc.pid);
    }
    return targetProc;
}

void insertToTable(pInfo process){
    
   // printf("[insertTotabel] instert to table ran and process[table] = %s\n", processTable[slot].name);
}



void init(void){ 
  phase2_start_service_processes();
  phase3_start_service_processes();
  phase4_start_service_processes();
  phase5_start_service_processes(); 

  //call spork 
  int tcm = spork("testcase_main", NULL, NULL, USLOSS_MIN_STACK , 3);
  tcm = getpid();
  printf("tcm is in spot %d",tcm);
}

void phase1_init(){
    // set up all slot in table to not running state
    pInfo emptyProc; 
    emptyProc.state = NOTRUNING;
    for(int i = 0; i < MAXPROC; ++i){
            processTable[i] = emptyProc;
    }

    // set up but dont initialize initPrc 
    pInfo initPrc;
    initPrc.name = "init";
    initPrc.priority = 6;
    initPrc.pid = 1;
    initPrc.state = NOTRUNING;
    initPrc.fp = init;
    int slot = initPrc.pid % MAXPROC;
    processTable[slot] = initPrc;

    //testing purposes 
    printf("[phase1_a] init inserted in to process table %d\n", processTable[slot].pid);  
    printf("[phase1_a] state = %d\n", processTable[slot].state);
}

int spork(char *name, int (*func)(void *), void *arg, int stacksize, int priority){
    pInfo newProcess;
    newProcess.name = name;
    newProcess.priority = priority;
    newProcess.pid = pid;
    pid++;
    newProcess.state = READY;
    newProcess.stackSize = USLOSS_MIN_STACK;
    newProcess.stack = malloc(newProcess.stackSize * sizeof(pInfo));

    // creates child relation ship and inserts into table 
    int slot = newProcess.pid % MAXPROC;
    processTable[slot] = newProcess;
    USLOSS_ContextInit(&processTable[slot].new, processTable[slot].stack, processTable[slot].stackSize,NULL, &wrapper);
    TEMP_switchTo(newProcess.pid);
    return newProcess.pid;
}

int join(int *status){
    //printf("[join] ran\n");
    return 0;
}

void quit_phase_1a(int status, int switchToPid){
    // not started
    int *p = &status;
    join(p);
    //printf("[quitphase1a] ran\n"); 
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
   pInfo currProcPid = processTable[pid % MAXPROC];
   return currProcPid.pid;
}

void dumpProcesses(){
    printf("Not started\n");
}

void TEMP_switchTo(int pid){
    printf("[TEMP_switchTO]\n");
    printf("[TEMP_switchTO]this should be init = %s\n", processTable[pid % MAXPROC].name);
    //printf("[initContext] ran\n");
    USLOSS_ContextSwitch(NULL, &processTable[pid % MAXPROC].new);
    //printf("[tempToSwitch] ran\n");
}


