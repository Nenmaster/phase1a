#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "phase1.h"
#include "usloss.h"

typedef struct pInfo{
    // general process info 
    char *name;
    int priority;
    int pid;
    int parentPid;
    int status;
    int state; 
    bool dead;

    struct pInfo *parent;
    struct pInfo *firstChildHead;
    struct pInfo *nextChild;

    USLOSS_Context context;
    void *stack;
    int stackSize;
    int (*startFunc)(void*);
    void *argument;
}pInfo;

// global variables
pInfo processTable[MAXPROC];
pInfo *mainHead = NULL;
bool dead = false;
bool flag = false;
pInfo *currProc = NULL;

int findNextProc();



// this func is meant to be used in the USLOSS_:wContextInit
// it should not return instead it will call startFunc which will
// return and pass that value to quit 
void uslossWrapper(void){
    int prevPsr = USLOSS_PsrGet();
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }


    int pid = getpid();
    pInfo *process = &processTable[pid % MAXPROC];
    process -> startFunc(process -> argument);
    process -> dead = true;

    if(USLOSS_PsrSet(prevPsr) != 0) {
        USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
        USLOSS_Halt(1);
    }
}

int testcase_mainWrapper(void *arg){

     if (USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("ERROR: Unable to enable interrupts in testcase_mainWrapper\n");
        USLOSS_Halt(1);
    }

    int retval = testcase_main();
 
    USLOSS_Console("Phase 1A TEMPORARY HACK: testcase_main() returned, simulation will now halt.\n");
    USLOSS_Halt(retval);

    return retval;
}

void init(void) { 
    int prevPsr = USLOSS_PsrGet();
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }

    phase2_start_service_processes();
    phase3_start_service_processes();
    phase4_start_service_processes();
    phase5_start_service_processes(); 
    int tcm = spork("testcase_main", 
                    testcase_mainWrapper,
                    NULL, 
                    USLOSS_MIN_STACK, 
                    3);

    USLOSS_Console("Phase 1A TEMPORARY HACK: init() manually switching to testcase_main() after using spork() to create it.\n");
    TEMP_switchTo(tcm);
    
    while(1){
        int status;
        printf("status = %d\n", status);
        int pid = join(&status);
        if(pid == -2){
            USLOSS_Console("Init has no more children\n");
            USLOSS_Halt(1);
        }
    }
    if(USLOSS_PsrSet(prevPsr) != 0) {
        USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
        USLOSS_Halt(1);
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
        processTable[i].dead = true; 
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
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Someone attempted to call spork while in user mode!\n");
        USLOSS_Halt(1);
    }

    int prevPsr = USLOSS_PsrGet();
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }

    if(stacksize < USLOSS_MIN_STACK) return -2;
    if(priority < 1 || priority > 5) return -1;
    if(name == NULL || strlen(name) >= MAXNAME) return -1;
    if(startFunc == NULL) return  -1;

    
    static int nextPid = 2;
    int slot = nextPid % MAXPROC;

    if (processTable[slot].pid == -1 && processTable[slot].dead){
        int currPid = nextPid++;
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
    
        if(currProc == NULL){
            newProcess -> parent = &processTable[1];
            newProcess -> parentPid = 1;
        }else {
            newProcess -> parent = currProc;
            newProcess -> parentPid = currProc -> pid;
        }

        if(newProcess -> stack == NULL){
            USLOSS_Console("malloc out of memory error");
            return -1;
        }
        newProcess -> dead = false;
        pInfo *parent = newProcess -> parent;
        if(parent -> firstChildHead == NULL){
            parent -> firstChildHead = newProcess;
        }else {
            pInfo *child = parent -> firstChildHead;
            while(child -> nextChild != NULL && child -> pid != -1){
                child = child -> nextChild;
            }
            child -> nextChild = newProcess;
        }

        USLOSS_ContextInit(&processTable[slot].context,
                       processTable[slot].stack,
                       processTable[slot].stackSize,
                       NULL,
                       uslossWrapper); 

        if(USLOSS_PsrSet(prevPsr) != 0) {
            USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
            USLOSS_Halt(1);
        }
        return currPid;
    } else {
        if(USLOSS_PsrSet(prevPsr) != 0) {
            USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
            USLOSS_Halt(1);
        }
    }
    return -1;
}

pInfo *reverse(pInfo *head){
    pInfo *prev = NULL;
    pInfo *curr = head;
    pInfo *next = NULL;

    while(curr != NULL && curr ->pid >= 0){
        next = curr -> nextChild;
        curr -> nextChild = prev;
        prev = curr;
        curr = next;
    }
    return prev;
}

int join(int *status){
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Not in kernel mode. Call aborted.\n");
        USLOSS_Halt(1);
    }

    int prevPsr = USLOSS_PsrGet();
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }

    if(currProc == NULL || currProc -> firstChildHead == NULL || status == NULL){
        return -2;
    }
    
    pInfo *revList = reverse(currProc -> firstChildHead);
    currProc -> firstChildHead = revList;

    pInfo *prev = NULL;
    pInfo *child = revList;

    while(child != NULL){
        if(child -> dead ) {
            if(prev == NULL){
                currProc -> firstChildHead = child -> nextChild;
            }else {
                prev -> nextChild = child -> nextChild; 
            }

            *status = child -> status; 
            int pid = child -> pid;
     
            free(child -> stack); 
            processTable[pid % MAXPROC].pid = -1;
 
            return pid;
        }
        prev = child;
        child = child -> nextChild;
        
    }

    if(USLOSS_PsrSet(prevPsr) != 0) {
        USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
        USLOSS_Halt(1);
    }
    currProc -> firstChildHead = reverse(revList);
    return -1;
}

void quit_phase_1a(int status, int switchToPid){ 
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Not in kernel mode. Call aborted.\n");
        USLOSS_Halt(1);
    }

    int prevPsr = USLOSS_PsrGet();
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }

    if(currProc == NULL){
        USLOSS_Halt(1);
    }

    if(currProc -> pid == 1){
        USLOSS_Halt(1);
    }

    if(currProc -> firstChildHead != NULL){
        USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n", currProc -> pid);
        USLOSS_Halt(1);
    }

    if(currProc -> firstChildHead != NULL){
        USLOSS_Console("DEBUG: Process %d has children, checking their status\n", currProc->pid);
        pInfo *child = currProc -> firstChildHead;
        while (child  != NULL) {
            USLOSS_Console("DEBUG: Checking child %d, dead=%d\n", child->pid, child->dead);
            if(!child -> dead){
                USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n",currProc -> pid);
                USLOSS_Halt(1);
            }
            child = child -> nextChild;
        
        }
    }

    currProc -> status = status; 
    currProc -> dead = true;

    if(switchToPid >= 0 && 
       switchToPid < MAXPROC && processTable[switchToPid % MAXPROC].pid != -1 && !processTable[switchToPid % MAXPROC].dead){

        TEMP_switchTo(switchToPid);
    }else {
        USLOSS_Halt(0);
    } 

    if(USLOSS_PsrSet(prevPsr) != 0) {
        USLOSS_Halt(1);
    }
    while(1){}
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
    while(1){}
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
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Not in kernel mode. Call aborted.\n");
        USLOSS_Halt(1);
    }

    int prevPsr = USLOSS_PsrGet();
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }

    if(pid < 0 ||pid >=  MAXPROC || processTable[pid % MAXPROC].pid == -1 || processTable[pid % MAXPROC].dead){
        USLOSS_Halt(1);
    }
    
    USLOSS_Context *old = (currProc == NULL) ? NULL : &currProc -> context;
    currProc = &processTable[pid % MAXPROC];
    
    if(USLOSS_PsrSet(prevPsr) != 0) {
        USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
        USLOSS_Console("TEMP_switchTo: returned to PID %d (should not happen)\n", getpid());

        USLOSS_Halt(1);
    }

    USLOSS_ContextSwitch(old, &currProc -> context);
}
