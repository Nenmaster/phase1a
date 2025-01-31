#include <stdio.h>
#include <stdlib.h>
#include "phase1.h"
#include "src/usloss.h"

#define USLOSS_MIN_STACK (80 * 1024)
// helper prototypes 

typedef struct processInfo{
    // general process info 
    char *name;
    int priority;
    int pid;
    int *status;

    // info to work with USLOSS
    USLOSS_Context *context;
    void *stack;
    int stackSize;
    void (*fp)(void);
   
}pInfo;

pInfo processTable[MAXPROC];

int init(){
    testcase_main();	
    pInfo np;
    np.fp = &testcase_main();
    USLOSS_ContextInit(np.context, np.stack, np.stackSize,NULL, testcase_main()); 
    processTable[0] = np;
    if(testcase_main() != 0){
        fprintf(stderr, "incorrect user function");
        return -1;
    } else {
        USLOSS_Halt(0);
    
    }
}

void *testcase_main(){
    int c;
    while((c = testcase_main()) != 0) {
            testcase_main();
    }
    USLOSS_Halt(c);
}

void phase1_init(){
    int *size = malloc(USLOSS_MIN_STACK * sizeof(char));

    np.name = "init";
    np.stackSize = USLOSS_MIN_STACK;
    newProcess.priority = 6;
    newProcess.pid = 1;

    init();

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

    printf("Not started");
}

void quit(int status){
    printf("Not started");
}

int getpid(){
    return 0;
}

void dumpProcesses(){
    printf("Not started");
}

void TEMP_switchTo(int pid){
    printf("Not started");
}

int main() {

    return 0;        
}

