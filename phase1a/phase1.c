#include <stdio.h>
#include <stdlib.h>
#include "phase1.h"
//#include "src/usloss.h"
#include "usloss.h"
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
    int *status;
    
    // info to work with USLOSS
    USLOSS_Context *new;
    void *stack;
    int stackSize;
    void (*fp)(void);
    int (*tcm)(void);
}pInfo;

pInfo processTable[MAXPROC];

pInfo createNewProcces(char *name,int priority){
    pInfo newProcess;
    newProcess.name = name;
    newProcess.priority = priority;
    newProcess.pid = pid;
    ++pid;
    newProcess.stackSize = USLOSS_MIN_STACK;
    newProcess.stack = malloc(newProcess.stackSize * sizeof(pInfo));
    //printf("[creatNewProcess] ran\n");
    return newProcess;
}

pInfo findPID(int pid){
    pInfo targetProc;
    for(int i = 0; i < MAXPROC; i++){
        if(processTable[i].pid == pid){
            targetProc = processTable[i];
           // printf("process table name %s\n", targetProc.name);
        }
    }
    //printf("find pid ran\n");
    return targetProc;
}

void insertToTable(pInfo process){
    int slot = process.pid % MAXPROC;
    processTable[slot] = process;
   // printf("[insertTotabel] instert to table ran and process[table] = %s\n", processTable[slot].name);
}

void init(void){ 
  phase2_start_service_processes();
  phase3_start_service_processes();
  phase4_start_service_processes();
  phase5_start_service_processes(); 
//call spork 
  pInfo newProcess = createNewProcces("testcase_main",3);
  //printf("[inint] process name is %s and process id is %d\n", newProcess.name, newProcess.pid);
  newProcess.tcm = testcase_main;
  insertToTable(newProcess);

  TEMP_switchTo(newProcess.pid);
  if(!testcase_main()){
    printf("halt ran ****************************\n");
    USLOSS_Halt(testcase_main());
  }

}

void phase1_init(){
    pInfo newProcess = createNewProcces("init",6);
   // printf("[phase_1] process name is %s and process id is %d\n", newProcess.name, newProcess.pid); 
    newProcess.fp = init;
    insertToTable(newProcess);
    init();
    //printf("[phase1_int] ran\n");
}


int spork(char *name, int (*func)(void *), void *arg, int stacksize, int priority){
   // printf("[spork] ran\n"); 
    return 0;
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
   int pid = 1;
   pInfo process = processTable[pid % MAXPROC];
   return process.pid;
}

void dumpProcesses(){
    printf("Not started\n");
}

void TEMP_switchTo(int pid){
    pInfo newProc = findPID(pid);
    
    USLOSS_ContextInit(newProc.new, newProc.stack, newProc.stackSize,NULL, newProc.fp);
    //printf("[initContext] ran\n");
    USLOSS_ContextSwitch(NULL, newProc.new);
    //printf("[tempToSwitch] ran\n");
}


