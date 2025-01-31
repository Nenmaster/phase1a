#include <stdio.h>
#include <stdlib.h>
#include "phase1.h"
#include <usloss.h>

#define USLOSS_MIN_STACK (80 * 1024)
// helper prototypes 

typedef struct processInfo{
    char *name;
    int (*fp)(void*);
    void *USLOSS_Context;
    int *stackSize;
    int priority;
    int pid;
    int *status;
}pInfo;

 pInfo processTable[MAXPROC];

int init(){
    testcase_main(); 
    USLOSS_ConextSwitch(NULL, testcase_main());

    if(testcase_main() != 0){
        fprintf(stderr, "incorrect user function");
        return -1;
    } else {
        USLOSS_Halt(0);
    }
}

int testcase_main(){
    int c;
    while((c = testcase_main()) != 0) {
            testcase_main();
    }
    return USLOSS_Halt(c);
}

void phase1_init(){
    pInfo newProcess;
    int *size = malloc(USLOSS_MIN_STACK * sizeof(char));

    newProcess.name = "init";
    newProcess.fp = init();
    newProcess.USLOSS_Context = "init";
    newProcess.stackSize = size;
    newProcess.priority = 6;
    newProcess.pid = 1;

    int slot = newProcess.pid % MAXPROC;

    processTable[slot] = newProcess;

}


int spork(char *name, int (*func)(void *), void *arg, int stacksize, int priority){
    return 0;
}

int join(int *status){
    return 0;
}

void quit_phase_1a(int status, int switchToPid){

}

void quit(int status){

}

int getpid(){
    return 0;
}

void dumpProcesses(){

}

void TEMP_switchTo(int pid){

}

// Helper Functions 



int main(){    

    phase1_init();

    return 0;
}
