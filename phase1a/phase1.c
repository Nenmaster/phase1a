/*
 *
 *Programmers: Omar Mendivil & Ayman Mohamed
 *Phase1a process system mangages a process table of pInfo structs
 *Handles process creation with spork
 * process termination via quit_phase1a
 * Parent-child relations
 * process tracking
 */

#include "phase1.h"
#include "usloss.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct pInfo {
  // general process info
  char *name;
  int priority;
  int pid;
  int parentPid;
  int status;
  int state;
  int slot;
  bool dead;

  struct pInfo *parent;
  struct pInfo *firstChildHead;
  struct pInfo *nextChild;

  USLOSS_Context context;
  void *stack;
  int stackSize;
  int (*startFunc)(void *);
  void *argument;
} pInfo;

// global variables
pInfo processTable[MAXPROC];
pInfo *currProc = NULL;
int nextPid = 2;
int nextSlot = 3;

// prototpyes
pInfo *reverse(pInfo *head);
int findProcSlot(int pid);
void kernelModeCheck();
int disableInterrupts();
void restoreInterrupts(int prevPsr);
void linkChild(pInfo *parent, pInfo *child);

// errase this function before turning in very helpful for
// parent child relationship printing
void printProcessHierarchy() {
  printf("\nProcess Hierarchy:\n");
  printf("====================\n");

  for (int i = 0; i < MAXPROC; ++i) {
    if (processTable[i].pid != -1 && !processTable[i].dead) {
      printf("Process PID: %d, Name: %s, Parent PID: %d\n", processTable[i].pid,
             processTable[i].name ? processTable[i].name : "(null)",
             processTable[i].parentPid);

      if (processTable[i].firstChildHead != NULL) {
        printf("  Children: ");
        pInfo *child = processTable[i].firstChildHead;
        while (child != NULL) {
          printf("%d (%s) -> ", child->pid, child->name);
          child = child->nextChild;
        }
        printf("NULL\n");
      }
    }
  }
  printf("====================\n");
}

// this func is meant to be used in the USLOSS_ContextInit
// it should not return instead it will call startFunc which will
// return and pass that value to quit
void uslossWrapper(void) {
  int prevPsr = USLOSS_PsrGet();

  // ensures were in kernel mode
  if (!(prevPsr & USLOSS_PSR_CURRENT_MODE)) {
    prevPsr |= USLOSS_PSR_CURRENT_MODE;
  }

  // dont use disbale interrupts here due to modify prev
  // psr above
  if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
    USLOSS_Console("Failed to disable interrupts in wrapper\n");
    USLOSS_Halt(1);
  }

  int slot = -1;
  int pid = getpid();
  for (int i = 0; i <= MAXPROC; i++) {
    if (processTable[i].pid == pid) {
      slot = i;
      break;
    }
  }
  pInfo *process = &processTable[slot];

  if (USLOSS_PsrSet(prevPsr | USLOSS_PSR_CURRENT_INT) != 0) {
    USLOSS_Console("faled to enable interrupts in wrapper");
    USLOSS_Halt(1);
  }

  process->startFunc(process->argument);

  prevPsr |= USLOSS_PSR_CURRENT_MODE;
  if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
    USLOSS_Console("Did not return in kernel mode");
    USLOSS_Halt(1);
  }

  process->dead = true;

  restoreInterrupts(prevPsr);
}

int testcase_mainWrapper(void *arg) {

  if (USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT) != 0) {
    USLOSS_Console(
        "ERROR: Unable to enable interrupts in testcase_mainWrapper\n");
    USLOSS_Halt(1);
  }

  int retval = testcase_main();

  USLOSS_Console("Phase 1A TEMPORARY HACK: testcase_main() returned, "
                 "simulation will now halt.\n");
  USLOSS_Halt(retval);

  return retval;
}

// init is  the first proccess with proirty 6
// creates and manually switch testcase_main
// keeps calling join  on a loop
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
  int tcm =
      spork("testcase_main", testcase_mainWrapper, NULL, USLOSS_MIN_STACK, 3);

  USLOSS_Console("Phase 1A TEMPORARY HACK: init() manually switching to "
                 "testcase_main() after using spork() to create it.\n");

  TEMP_switchTo(tcm);

  while (1) {
    int status = 0;
    int pid = join(&status);
    if (pid == -2) {
      USLOSS_Console("Init has no more children\n");
      USLOSS_Halt(1);
    }
  }

  if (USLOSS_PsrSet(prevPsr) != 0) {
    USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
    USLOSS_Halt(1);
  }
}

// sets up init process but doesnt run it
// places init into table as NOTRUNNING
// Manually initialize init proc with USLOSS_ContextInit
void phase1_init() {
  int prevPsr = USLOSS_PsrGet();
  if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
    USLOSS_Console("Failed to disable interrupts in phase1_init\n");
    USLOSS_Halt(1);
  }

  for (int i = 0; i <= MAXPROC; ++i) {
    processTable[i].name = "";
    processTable[i].priority = -1;
    processTable[i].pid = -1;
    processTable[i].dead = true;
    processTable[i].slot = i;
  }

  pInfo *initPrc = &processTable[1];
  char *initName = strdup("init");
  if (initName == NULL) {
    USLOSS_Console("[init] Failed allocate process name\n");
    USLOSS_Halt(1);
  }

  initPrc->name = initName;
  initPrc->priority = 6;
  initPrc->pid = 1;
  initPrc->dead = false;
  initPrc->slot = 1;
  initPrc->startFunc = NULL;
  initPrc->argument = NULL;
  initPrc->stackSize = USLOSS_MIN_STACK;
  initPrc->stack = malloc(initPrc->stackSize);
  if (initPrc->stack == NULL) {
    free(initName);
    free(initPrc->stack);
    USLOSS_Console("Malloc failed in init proc buy more ram");
    USLOSS_Halt(1);
  }

  initPrc->firstChildHead = NULL;
  initPrc->nextChild = NULL;
  initPrc->parent = NULL;

  currProc = initPrc;
  USLOSS_ContextInit(&initPrc->context, initPrc->stack, initPrc->stackSize,
                     NULL, init);

  if (USLOSS_PsrSet(prevPsr) != 0) {
    USLOSS_Console("Error: Failed to restore PSR in phase1_init\n");
    USLOSS_Halt(1);
  }
}

// spork works by pointing a new processes startfunc to that process function
// creates the chilled process of the current  process.
// does not call the dispatcher.
int spork(char *name, int (*startFunc)(void *), void *arg, int stacksize,
          int priority) {
  if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
    USLOSS_Console(
        "ERROR: Someone attempted to call spork while in user mode!\n");
    USLOSS_Halt(1);
  }

  int prevPsr = disableInterrupts();

  if (stacksize < USLOSS_MIN_STACK) {
    restoreInterrupts(prevPsr);
    return -2;
  }
  if (priority < 1 || priority > 5) {
    restoreInterrupts(prevPsr);
    return -1;
  }
  if (name == NULL || strlen(name) >= MAXNAME) {
    restoreInterrupts(prevPsr);
    return -1;
  }
  if (startFunc == NULL) {
    restoreInterrupts(prevPsr);
    return -1;
  }

  int slot = -1;
  if (currProc->pid == 1) {
    slot = 2;
  } else {
    for (int i = 0; i < MAXPROC; i++) {
      int idx = (nextSlot + i) % MAXPROC;

      if (idx == 1 || idx == 2) {
        nextPid++;
        continue;
      }

      if (processTable[idx].pid == -1 && processTable[idx].dead) {
        slot = idx;
        break;
      }
    }
  }

  if (slot == -1) {
    restoreInterrupts(prevPsr);
    return -1;
  }

  nextSlot = (slot + 1) % MAXPROC;

  if (processTable[slot].pid == -1 && processTable[slot].dead) {
    int currPid = nextPid++;

    // new process creation
    pInfo *newProcess = &processTable[slot];
    char *namePointer = strdup(name);
    newProcess->name = namePointer;
    newProcess->pid = currPid;
    newProcess->slot = slot;
    newProcess->priority = priority;
    newProcess->startFunc = startFunc;
    newProcess->argument = arg;
    newProcess->stackSize = stacksize;
    newProcess->stack = malloc(newProcess->stackSize);
    newProcess->dead = false;
    newProcess->parent = NULL;
    newProcess->firstChildHead = NULL;
    newProcess->nextChild = NULL;

    if (newProcess->stack == NULL || namePointer == NULL) {
      USLOSS_Console("Out of Memory error spork for stack");
      free(namePointer);
      USLOSS_Halt(1);
    }

    if (currProc == NULL) {
      newProcess->parent = &processTable[0];
      newProcess->parentPid = 1;
      currProc = newProcess;
    } else {
      linkChild(currProc, newProcess);
    }

    USLOSS_ContextInit(&processTable[slot].context, processTable[slot].stack,
                       processTable[slot].stackSize, NULL, uslossWrapper);

    restoreInterrupts(prevPsr);
    return currPid;
  } else {
    restoreInterrupts(prevPsr);
  }

  return -1;
}

//  join wakes up the perent proccess if the child process died.
//  join reports the status of that dead child .
//  join never blocks
int join(int *status) {
  kernelModeCheck();
  int prevPsr = disableInterrupts();

  if (currProc == NULL || status == NULL) {
    USLOSS_Console("Error got NULL\n");
    restoreInterrupts(prevPsr);
    return -2;
  }

  pInfo *revList = reverse(currProc->firstChildHead);
  currProc->firstChildHead = revList;

  pInfo *prev = NULL;
  pInfo *child = revList;

  int count = 0;
  while (child != NULL) {
    if (child->dead) {
      if (prev == NULL) {
        currProc->firstChildHead = child->nextChild;
      } else {
        count++;
        prev->nextChild = child->nextChild;
      }

      count++;

      *status = child->status;
      int pid = child->pid;
      int childSlot = child->slot;

      free(child->name);
      free(child->stack);
      processTable[childSlot].pid = -1;
      processTable[childSlot].dead = true;

      currProc->firstChildHead = reverse(currProc->firstChildHead);

      restoreInterrupts(prevPsr);
      return pid;
    }
    prev = child;
    child = child->nextChild;
  }
  child = currProc->firstChildHead;
  while (child != NULL) {
    if (!child->dead) {
      // this should block for phase1b
      restoreInterrupts(prevPsr);
      return -2;
    }
    child = child->nextChild;
  }

  restoreInterrupts(prevPsr);
  pInfo *orginalList = reverse(currProc->firstChildHead);
  currProc->firstChildHead = orginalList;
  // this should block for phase1b
  return -2;
}

// quits the current proccess
// switch to the process pass to it in the second arrgument.
void quit_phase_1a(int status, int switchToPid) {
  if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
    USLOSS_Console(
        "ERROR: Someone attempted to call quit_phase_1a while in user mode!\n");
    USLOSS_Halt(1);
  }

  int prevPsr = disableInterrupts();

  if (currProc == NULL) {
    USLOSS_Halt(1);
  }

  if (currProc->pid == 1) {
    USLOSS_Halt(1);
  }

  if (currProc->firstChildHead != NULL) {
    USLOSS_Console(
        "ERROR: Process pid %d called quit() while it still had children.\n",
        currProc->pid);
    USLOSS_Halt(1);
  }

  int slot = -1;
  for (int i = 0; i <= MAXPROC; i++) {
    if (&processTable[i].pid == &switchToPid) {
      slot = i;
      break;
    }
  }

  currProc->status = status;
  currProc->dead = true;

  if (switchToPid >= 0 && processTable[slot].pid != -1 &&
      !processTable[slot].dead) {
    TEMP_switchTo(switchToPid);
  } else {
    USLOSS_Halt(0);
  }

  if (USLOSS_PsrSet(prevPsr) != 0) {
    USLOSS_Halt(1);
  }
  restoreInterrupts(prevPsr);
  while (1) {
  }
}

// does nothing in this phase
// to be implemented in next phase
void quit(int status) {
  // does nothing
  while (1) {
  }
}

// returns the pid of the current running process.
int getpid() {
  if (currProc == NULL) {
    return -1;
  }
  return currProc->pid;
}

// prints out the process information
// gets the process information from the process table
void dumpProcesses() {
  printf(" PID  PPID  NAME              PRIORITY  STATE\n");

  for (int i = 0; i < MAXPROC; ++i) {
    if (processTable[i].pid != -1) {
      printf("%4d ", processTable[i].pid);
      printf("%5d  ", processTable[i].parentPid);

      if (processTable[i].name != NULL) {
        int count = 0;
        char *nameChar = processTable[i].name;
        while (*nameChar && count < 16) {
          putchar(*nameChar);
          nameChar++;
          count++;
        }
        while (count < 16) {
          putchar(' ');
          count++;
        }
      } else {
        printf("(null)          ");
      }
      printf(" ");
      printf("%2d", processTable[i].priority);
      if (processTable[i].dead) {
        printf("         Terminated(%d)\n", processTable[i].status);
      } else if (currProc == &processTable[i]) {
        printf("         Running\n");
      } else {
        printf("         Runnable\n");
      }
    }
  }
}

// TEMP_switchTo is a temprory function untill dispatcher is impletmented
// this will use USLOSS_ContextSwitch() to preform a context switch to the
// process id passed in the argument
void TEMP_switchTo(int pid) {
  kernelModeCheck();
  int prevPsr = USLOSS_PsrGet();

  if (prevPsr == 0) {
    prevPsr = USLOSS_PSR_CURRENT_MODE;
    USLOSS_Console("PSR was set to 0 change to kernel mode");
  }

  int psrRes = USLOSS_PsrSet(prevPsr);
  if (psrRes != USLOSS_DEV_OK) {
    USLOSS_Console("Did not set Psr before context switch");
    USLOSS_Halt(1);
  }

  if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
    USLOSS_Console("Failed to disable interrupts in phase1_init\n");
    USLOSS_Halt(1);
  }

  int slot = -1;
  for (int i = 0; i <= MAXPROC; i++) {
    if (processTable[i].pid == pid && !processTable[i].dead) {
      slot = i;
      break;
    }
  }

  // special check for first proccess(init)
  if (currProc->parent == NULL) {
    currProc = &processTable[slot];
    USLOSS_ContextSwitch(NULL, &currProc->context);
  }

  if (pid < 0 || processTable[slot].pid == -1 || processTable[slot].dead) {
    USLOSS_Console(
        "Invalid switch, proc is either dead, terminated or invalid slot\n");
    USLOSS_Halt(1);
  }

  USLOSS_Context *old = &currProc->context;
  currProc = &processTable[slot];

  restoreInterrupts(prevPsr);
  USLOSS_ContextSwitch(old, &currProc->context);
}

////// HELPER FUNCTIONS ////

// reverses the child link list
pInfo *reverse(pInfo *head) {
  pInfo *prev = NULL;
  pInfo *curr = head;
  pInfo *next = NULL;

  while (curr != NULL && curr->pid > 0) {
    next = curr->nextChild;
    curr->nextChild = prev;
    prev = curr;
    curr = next;
  }
  return prev;
}

// finds the proces slot for the given pid
int findProcSlot(int pid) {
  for (int i = 0; i < MAXPROC; i++) {
    if (processTable[i].pid == pid) {
      return i;
    }
  }
  USLOSS_Console("Proccess Pid not found");
  USLOSS_Halt(1);
  return -1;
}

void kernelModeCheck() {
  int pid = getpid();
  int slot = findProcSlot(pid);
  pInfo *proc = &processTable[slot];
  if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
    USLOSS_Console("ERROR: Someone attempted to call %s while in user mode!\n",
                   proc->name);
    USLOSS_Halt(1);
  }
}

int disableInterrupts() {
  int prevPsr = USLOSS_PsrGet();
  if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) != 0) {
    USLOSS_Console("Failed to diable Interrupts\n");
    USLOSS_Halt(1);
  }

  return prevPsr;
}

void restoreInterrupts(int prevPsr) {
  if (USLOSS_PsrSet(prevPsr) != 0) {
    USLOSS_Console("Failed to restore Interrupts\n");
    USLOSS_Halt(1);
  }
}

// Same logic as before just encapsulated into a funciton
// adds process to head of child list
void linkChild(pInfo *parent, pInfo *child) {
  child->parent = parent;
  child->parentPid = parent->pid;

  if (parent->firstChildHead == NULL) {
    parent->firstChildHead = child;
  } else {
    pInfo *kid = parent->firstChildHead;
    while (kid->nextChild != NULL) {
      kid = kid->nextChild;
    }
    kid->nextChild = child;
  }
}
