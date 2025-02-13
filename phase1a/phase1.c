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

    // removed process or Node structs and used pInfo as the 
    // main node struct for the process, have differnt structs was making
    // adding, removing and search difficult 
    struct pInfo *parent;
    struct pInfo *firstChildHead;
    struct pInfo *nextChild;


    // info to work with USLOSS
    USLOSS_Context context;
    void *stack;
    int stackSize;
    int (*startFunc)(void*);
    void *argument;
}pInfo;
// might not be neccessary 

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
    //USLOSS_Console("\n=== uslossWrapper Start ===\n");
    int prevPsr = USLOSS_PsrGet();
     //USLOSS_Console("Wrapper: PID %d starting with PSR=%d\n", getpid(), prevPsr);
    //USLOSS_Console("uslossWrapper: PID %d starting with PSR=%d\n", getpid(), prevPsr);
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }
    
    //USLOSS_Console("uslossWrapper: starting process with PID %d\n", getpid());


    int pid = getpid();
    pInfo *process = &processTable[pid % MAXPROC];
    //USLOSS_Console("Wrapper: Starting process PID %d (name: %s)\n", 
     //              pid, process->name);
    process -> startFunc(process -> argument);
    //USLOSS_Console("Wrapper: Process PID %d completed execution\n", pid);
    process -> dead = true;
    //USLOSS_Console("=== uslossWrapper End ===\n");
    //USLOSS_Console("uslossWrapper: PID %d finished execution; marking as dead.\n", getpid());
    if(USLOSS_PsrSet(prevPsr) != 0) {
        USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
        USLOSS_Halt(1);
    }
    //USLOSS_Halt(retval);
}

int testcase_mainWrapper(void *arg){
     //USLOSS_Console("\n=== testcase_mainWrapper Start ===\n");
    int prevPsr = USLOSS_PsrGet();
     //USLOSS_Console("TestcaseWrapper: Starting with PID %d, PSR=%d\n", 
     //              getpid(), prevPsr);
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }
    //USLOSS_Console("TestcaseWrapper: Calling testcase_main()\n");
    int retval = testcase_main();

    if(USLOSS_PsrSet(prevPsr) != 0) {
        USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
        USLOSS_Halt(1);
    }

    //USLOSS_Console("TestcaseWrapper: testcase_main() returned %d\n", retval);

    USLOSS_Console("Phase 1A TEMPORARY HACK: testcase_main() returned, simulation will now halt.\n");
    USLOSS_Halt(retval);

    //USLOSS_Console("=== testcase_mainWrapper End (Should never see this) ===\n");
    return retval;
}

void init(void) { 
    //USLOSS_Console("\n=== Init Start ===\n");
    int prevPsr = USLOSS_PsrGet();
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }

    phase2_start_service_processes();
    phase3_start_service_processes();
    phase4_start_service_processes();
    phase5_start_service_processes(); 
    //USLOSS_Console("Init: Creating testcase_main\n");
    int tcm = spork("testcase_main", 
                    testcase_mainWrapper,
                    NULL, 
                    USLOSS_MIN_STACK, 
                    3);
    //USLOSS_Console("Init: testcase_main created with PID %d\n", tcm);
    //USLOSS_Console("Init: Switching to testcase_main\n");
    //printf("curr proc pid %d tcm pid %d\n", getpid(), tcm);   
    USLOSS_Console("Phase 1A TEMPORARY HACK: init() manually switching to testcase_main() after using spork() to create it.\n");
    TEMP_switchTo(tcm);
    //USLOSS_Console("Init: Entered join loop\n");
    //printf("before while\n");

    while(1){
        int status;
        printf("status = %d\n", status);
        // USLOSS_Console("Init: Attempting join\n");
        int pid = join(&status);
       //printf("join = %d\n", pid);
        //USLOSS_Console("Init: Join returned PID %d with status %d\n", pid, status);
        if(pid == -2){
            USLOSS_Console("Init has no more children\n");
            USLOSS_Halt(1);
        }
    }
    //USLOSS_Console("=== Init End ===\n");
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
    int prevPsr = USLOSS_PsrGet();
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }
    //USLOSS_Console("Spork: Validating inputs:\n");
    //USLOSS_Console("  - Stack size: %d (min: %d)\n", stacksize, USLOSS_MIN_STACK);
    //USLOSS_Console("  - Priority: %d (range: 1-5)\n", priority);
    //USLOSS_Console("  - Name: %s (max length: %d)\n", name, MAXNAME-1); 


    if(stacksize < USLOSS_MIN_STACK) return -2;
    if(priority < 1 || priority > 5) return -1;
    if(name == NULL || strlen(name) >= MAXNAME) return -1;
    if(startFunc == NULL) return  -1;

    
    static int nextPid = 2;
    int slot = nextPid % MAXPROC;

    if (processTable[slot].pid == -1 && processTable[slot].dead){
        int currPid = nextPid++;
        pInfo *newProcess = &processTable[slot];
        //USLOSS_Console("Spork: Initializing new process:\n");
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
    
       // USLOSS_Console("Spork: Setting up parent relationship:\n");
        if(currProc == NULL){
            //USLOSS_Console("  - Setting init (PID 1) as parent\n");
            newProcess -> parent = &processTable[1];
            newProcess -> parentPid = 1;
        }else {
            //USLOSS_Console("  - Setting current process (PID %d) as parent\n", currProc->pid);
            newProcess -> parent = currProc;
            newProcess -> parentPid = currProc -> pid;
        }

        if(newProcess -> stack == NULL){
            USLOSS_Console("malloc out of memory error");
            return -1;
        }
        newProcess -> dead = false;
        pInfo *parent = newProcess -> parent;
        //USLOSS_Console("Spork: Adding to parent's child list:\n");
        //USLOSS_Console("  - Parent PID: %d\n", parent->pid);
        //USLOSS_Console("  - Parent's current first child: %p\n", parent->firstChildHead);
    //printf("parnet name = %s\n", parent -> name);
    //printf("parent pid = %d\n", parent -> pid);
        if(parent -> firstChildHead == NULL){
            //USLOSS_Console("  - Adding as first child\n");
            parent -> firstChildHead = newProcess;
        //printf("firstChild name = %s\n", parent -> firstChildHead -> name);
        //printf("firstChild pid = %d\n", parent -> firstChildHead -> pid);
        }else {
            //USLOSS_Console("  - Adding to end of child list\n");
            pInfo *child = parent -> firstChildHead;
        //printf("[spork else] child pid = %d\n", child -> pid);
            while(child -> nextChild != NULL && child -> pid != -1){
               // USLOSS_Console("    - Traversing past child PID: %d\n", child->pid);
                child = child -> nextChild;
            }
            child -> nextChild = newProcess;
           // USLOSS_Console("    - Added after child PID: %d\n", child->pid);
            //printf("child pid = %d\n", child -> pid);
        }
        //USLOSS_Console("=== Spork Complete: Created PID %d ===\n\n", currPid);

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
        //USLOSS_Console("ERROR: Slot %d not free (pid=%d, dead=%d)\n", 
         //             slot, processTable[slot].pid, processTable[slot].dead);   

    //addToTree(&processTable[newProcess.pid % MAXPROC], newProcess.parentPid);
        if(USLOSS_PsrSet(prevPsr) != 0) {
            USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
            USLOSS_Halt(1);
        }
    }
    return -1;
}

int join(int *status){
    int prevPsr = USLOSS_PsrGet();
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }

    //USLOSS_Console("\n=== Join: Started by PID %d ===\n", getpid());

    if(currProc == NULL || currProc -> firstChildHead == NULL || status == NULL){
        return -2;
    }
    
    //USLOSS_Console("Join: Parent PID %d checking children. First child: %d\n", 
     //              getpid(), currProc->firstChildHead ->pid);


    pInfo *prev = NULL;
    pInfo *child = currProc -> firstChildHead;
    //printf("child name = %s\n", child -> name);
    //printf("child pid = %d\n", child -> pid);

    while(child != NULL){
         //USLOSS_Console("Join: Examining child - PID: %d, Name: %s, Dead: %d\n", 
                     // child->pid, child->name, child->dead);
        if(child -> dead ) {
             //USLOSS_Console("Join: Found dead child PID %d\n", child->pid);
            if(prev == NULL){
                //USLOSS_Console("Join: Removing first child from list\n");
                currProc -> firstChildHead = child -> nextChild;
                //USLOSS_Console("join: scanning children of PID %d; current child PID = %d\n", getpid(), child->pid);

               // printf("currProc name = %s\n", currProc -> firstChildHead-> name);
               // printf("currProc pid = %d\n", currProc -> firstChildHead -> pid);
            }else {
                //USLOSS_Console("Join: Removing child from middle/end of list\n");
                prev -> nextChild = child -> nextChild;
               // printf("prev name = %s and prev pid = %d\n", prev -> nextChild -> name, prev -> nextChild -> pid);
            }

            *status = child -> status; 
            //printf("status = %d\n", *status);
            int pid = child -> pid;
            //printf("child pid = %d\n", pid);
            //USLOSS_Console("Join: Before cleanup - PID: %d, Status: %d\n", pid, *status);
            //USLOSS_Console("Join: Process table entry before cleanup: %d\n", 
                         // processTable[pid % MAXPROC].pid);

            free(child -> stack); 
            processTable[pid % MAXPROC].pid = -1;

             //USLOSS_Console("Join: After cleanup - Process table entry: %d\n", 
              //            processTable[pid % MAXPROC].pid);
            //USLOSS_Console("=== Join: Returning PID %d ===\n", pid);

            return pid;
        }
        //USLOSS_Console("Join: Child PID %d still alive, moving to next child\n", child->pid); 
        prev = child;
        child = child -> nextChild;
        
    }

    if(USLOSS_PsrSet(prevPsr) != 0) {
        USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
        USLOSS_Halt(1);
    }
    //USLOSS_Console("=== Join: No dead children found, returning -1 ===\n");
    return -1;
}

void quit_phase_1a(int status, int switchToPid){ 
    int prevPsr = USLOSS_PsrGet();
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }

   // USLOSS_Console("\n=== Quit Started for PID %d ===\n", getpid());
   // USLOSS_Console("Quit: Current process info:\n");
   // USLOSS_Console("  - Name: %s\n", currProc->name);
   // USLOSS_Console("  - Parent PID: %d\n", currProc->parentPid);
   // USLOSS_Console("  - Has children: %s\n", currProc->firstChildHead ? "Yes" : "No");
   // USLOSS_Console("  - Status before quit: %d\n", currProc->status);
   // USLOSS_Console("  - Dead flag before quit: %d\n", currProc->dead);
    //USLOSS_Console("quit_phase_1a: PID %d, current PSR=%d, terminating with status %d; switching to PID %d\n", getpid(), USLOSS_PsrGet(), status, switchToPid);

    if(currProc == NULL){
        //USLOSS_Console("Quit Error: currProc is NULL\n");
        USLOSS_Halt(1);
    }

    if(currProc -> pid == 1){
       // USLOSS_Console("Quit Error: Attempting to quit init process\n");  
        USLOSS_Halt(1);
    }
   // USLOSS_Console("Quit: Setting status %d and marking process dead\n", status);

    currProc -> status = status; 
    currProc -> dead = true;
   // USLOSS_Console("Quit: About to switch to PID %d\n", switchToPid);
   // USLOSS_Console("Quit: Switch-to process state: pid=%d, dead=%d\n", 
   //                processTable[switchToPid % MAXPROC].pid,
   //                processTable[switchToPid % MAXPROC].dead);

    if(switchToPid >= 0 && 
       switchToPid < MAXPROC && processTable[switchToPid % MAXPROC].pid != -1 && !processTable[switchToPid % MAXPROC].dead){

        //USLOSS_Console("quit_phase_1a: PID %d terminating with status %d; switching to PID %d\n", getpid(), status, switchToPid);
        TEMP_switchTo(switchToPid);
    }else {
        //USLOSS_Console("Quit Error: Invalid switch-to PID %d\n", switchToPid);
        USLOSS_Halt(0);
    } 

    if(USLOSS_PsrSet(prevPsr) != 0) {
        //USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
        USLOSS_Halt(1);
    }
    //USLOSS_Console("=== Quit: Should not reach this point ===\n");
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
    // USLOSS_Console("\n=== TEMP_switchTo Start ===\n");
    int prevPsr = USLOSS_PsrGet();
       //USLOSS_Console("Switch: Current PID: %d, Switching to PID: %d\n", 
             //      currProc ? currProc->pid : -1, pid);
    if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
        USLOSS_Console("Failed to disable interrupts in phase1_init\n");
        USLOSS_Halt(1);
    }

    if(pid < 0 ||pid >=  MAXPROC || processTable[pid % MAXPROC].pid == -1 || processTable[pid % MAXPROC].dead){
        USLOSS_Halt(1);
    }
    
    USLOSS_Context *old = (currProc == NULL) ? NULL : &currProc -> context;
   // USLOSS_Console("Switch: Old context: %p\n", (void*)old);
    currProc = &processTable[pid % MAXPROC];
   // USLOSS_Console("Switch: New context: %p\n", (void*)&currProc->context);
    
    //USLOSS_Console("TEMP_switchTo: switching from PID %d to PID %d\n", currProc->parentPid, pid);
     
    //USLOSS_Console("TEMP_switchTo: about to switch from PID %d (PSR=%d) to PID %d\n", getpid(), prevPsr, pid);
    //USLOSS_Console("Switch: Performing context switch\n");
    if(USLOSS_PsrSet(prevPsr) != 0) {
        USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
        USLOSS_Console("TEMP_switchTo: returned to PID %d (should not happen)\n", getpid());

        USLOSS_Halt(1);
    }

    USLOSS_ContextSwitch(old, &currProc -> context);
    //USLOSS_Console("Switch: Returned from context switch (PID %d)\n", getpid());
    //USLOSS_Console("=== TEMP_switchTo End ===\n");
    //USLOSS_Console("TEMP_switchTo: returned to PID %d (should not happen)\n", getpid());

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



