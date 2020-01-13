#include "Task.h"
#include "Descriptor.h" // for kernel segment

//
static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;


/*
  Init TCB Pool
*/
static void kInitializeTCBPOOL(void)
{
  int i;
  // Init Mem
  kMemSet(&(gs_stTCBPoolManager), 0, sizeof(gs_stTCBPoolManager));

  gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
  kMemSet(TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

  // assign ID
  for (i = 0; i < TASK_MAXCOUNT; i++) {
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
  }

  gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
  gs_stTCBPoolManager.iAllocatedCount = 1;
}

/*
  Allocate TCB from TCB Pool
*/
static TCB* kAllocateTCB(void)
{
  TCB* pstEmptyTCB;
  int i;

  // all TCB allocated
  if (gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount) {
    return NULL;
  }

  // find TCB to allocate
  for (i = 0; i < gs_stTCBPoolManager.iMaxCount; i++) {
    if ((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0) {
      pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
      break;
    }
  }

  // sign TCB[i] is allocated
  pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount << 32) | i;
  gs_stTCBPoolManager.iUseCount++;
  gs_stTCBPoolManager.iAllocatedCount++;
  if (gs_stTCBPoolManager.iAllocatedCount == 0)
    gs_stTCBPoolManager.iAllocatedCount = 1;

  return pstEmptyTCB;
}

/*
  Free TCB to TCB Pool
*/
static void kFreeTCB(QWORD qwID)
{
  int i;

  i = GETTCBOFFSET(qwID);

  kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
  // sign TCB[i] is free
  gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
  gs_stTCBPoolManager.iUseCount--;
}

/*
  Allocate TCB and set up Task
*/
TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress)
{
  TCB* pstTask, *pstProcess;
  void* pvStackAddress;

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stScheduler.stSpinLock));

  pstTask = kAllocateTCB();
  if (pstTask == NULL) {
    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
    // --- CRITCAL SECTION END ---
    return NULL;
  }

  pstProcess = kGetProcessByThread(kGetRunningTask());
  if (pstProcess == NULL) {
    kFreeTCB(pstTask->stLink.qwID);
    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
    // --- CRITCAL SECTION END ---
    return NULL;
  }

  // create Thread
  if (qwFlags & TASK_FLAGS_THREAD) {
    pstTask->qwParentProcessID = pstProcess->stLink.qwID;
    pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
    pstTask->qwMemorySize = pstProcess->qwMemorySize;

    kAddListToTail(&(pstProcess->stChildThreadList), &(pstTask->stThreadLink));
  }
  // create Process
  else {
    pstTask->qwParentProcessID = pstProcess->stLink.qwID;
    pstTask->pvMemoryAddress =  pvMemoryAddress;
    pstTask->qwMemorySize = qwMemorySize;
  }

  // Thread id = task id
  pstTask->stThreadLink.qwID = pstTask->stLink.qwID;

  kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
  // --- CRITCAL SECTION END ---

  pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * (GETTCBOFFSET(pstTask->stLink.qwID))));
  kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

  // init Child Thread List
  kInitializeList(&(pstTask->stChildThreadList));

  // Init FPU
  pstTask->bFPUUsed = FALSE;

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stScheduler.stSpinLock));

  kAddTaskToReadyList(pstTask);

  kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
  // --- CRITCAL SECTION END ---

  return pstTask;
}

/*
  set up Task
*/
static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize)
{
  // context init
  kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));

  // set RSP, RBP
  pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
  pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;

  // Return Address (kExitTask)
  *(QWORD *)((QWORD)pvStackAddress + qwStackSize - 8) = (QWORD)kExitTask;

  // Segment Selector
  pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;
  pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
  pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
  pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
  pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
  pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;

  // RIP Register
  pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;

  // set Interrupt Flags (bit 9)
  pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x0200;

  // set TCB member
  pstTCB->pvStackAddress = pvStackAddress;
  pstTCB->qwStackSize = qwStackSize;
  pstTCB->qwFlags = qwFlags;
}


// scheduler

/*
  Init Scheduler 
*/
void kInitializeScheduler(void)
{
  int i;
  TCB* pstTask;

  // Init TCB pool
  kInitializeTCBPOOL();

  // Init Multi level Task List
  for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
    kInitializeList(&(gs_stScheduler.vstReadyList[i]));
    gs_stScheduler.viExecuteCount[i] = 0;
  }
  kInitializeList(&(gs_stScheduler.stWaitList));

  // Init TCB for running task (boot task : shell)
  pstTask = kAllocateTCB();
  gs_stScheduler.pstRunningTask = pstTask;
  pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
  pstTask->qwParentProcessID = pstTask->stLink.qwID;
  pstTask->pvMemoryAddress = (void*)0x100000;
  pstTask->qwMemorySize = 0x500000;
  pstTask->pvStackAddress = (void*)0x600000;
  pstTask->qwStackSize = 0x100000;

  gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
  gs_stScheduler.qwProcessorLoad = 0;
  gs_stScheduler.qwLastFPUUsedTaskID = TASK_INVALIDID;

  kInitializeSpinLock(&(gs_stScheduler.stSpinLock));
}

/*
  Set Running Task
*/
void kSetRunningTask(TCB* pstTask)
{
  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stScheduler.stSpinLock));

  gs_stScheduler.pstRunningTask = pstTask;

  kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
  // --- CRITCAL SECTION END ---
}

/*
  Get Running Task TCB
*/
TCB* kGetRunningTask(void)
{
  TCB* pstRunningTask;

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stScheduler.stSpinLock));

  pstRunningTask = gs_stScheduler.pstRunningTask;

  kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
  // --- CRITCAL SECTION END ---

  return pstRunningTask;
}

/*
  Get Next Task TCB from multi level ready task list
*/
static TCB* kGetNextTaskToRun(void)
{
  TCB* pstTarget = NULL;
  int iTaskCount, i, j;

  for (j = 0; j < 2; j++) {
    for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
      iTaskCount = kGetListCount(&(gs_stScheduler.vstReadyList[i]));
      // return task in current list
      if (gs_stScheduler.viExecuteCount[i] < iTaskCount) {
        pstTarget = (TCB*)kRemoveListFromHeader(&(gs_stScheduler.vstReadyList[i]));
        gs_stScheduler.viExecuteCount[i]++;
        break;
      }
      // if execute all task, reset execute count and move to next prioity list
      else {
        gs_stScheduler.viExecuteCount[i] = 0;
      }
    }

    // find next task to run
    if (pstTarget != NULL)
      break;
  }

  return pstTarget;
}

/*
  Add Task TCB to Multi level ready task list
*/
static BOOL kAddTaskToReadyList(TCB* pstTask)
{
  BYTE bPriority;

  bPriority = GETPRIORITY(pstTask->qwFlags);

  // WAIT
  if (bPriority == TASK_FLAGS_WAIT)
  {
    kAddListToTail(&(gs_stScheduler.stWaitList), pstTask);
    return TRUE;
  }
  // invalid priority
  else if (bPriority >= TASK_MAXREADYLISTCOUNT)
    return FALSE;

  kAddListToTail(&(gs_stScheduler.vstReadyList[bPriority]), pstTask);
  return TRUE;
}

/*
  Remove Task TCB from Multi level ready task list
*/
static TCB* kRemoveTaskFromReadyList(QWORD qwTaskID)
{
  TCB* pstTarget;
  BYTE bPriority;

  // invalid task id
  if (GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT) {
    return NULL;
  }

  pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
  if (pstTarget->stLink.qwID != qwTaskID) {
    return NULL;
  }

  bPriority = GETPRIORITY(pstTarget->qwFlags);
  pstTarget = kRemoveList(&(gs_stScheduler.vstReadyList[bPriority]), qwTaskID);
  
  return pstTarget;
}

/*
  change Task prioiry using task id
*/
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority)
{
  TCB* pstTarget;

  // invalid task priority
  if (bPriority > TASK_MAXREADYLISTCOUNT) {
    return FALSE;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stScheduler.stSpinLock));

  // if target is running task, set prioirty
  pstTarget = gs_stScheduler.pstRunningTask;
  if (pstTarget->stLink.qwID == qwTaskID) {
    SETPRIORITY(pstTarget->qwFlags, bPriority);
  }
  // if target in ready task list
  else {
    // find task TCB
    // if task is not running, set priority
    pstTarget = kRemoveTaskFromReadyList(qwTaskID);
    if (pstTarget == NULL) {
      pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
      if (pstTarget != NULL) {
        SETPRIORITY(pstTarget->qwFlags, bPriority);
      }
    }
    // if task in ready list, pop task, set priority, push task
    else {
      SETPRIORITY(pstTarget->qwFlags, bPriority);
      kAddTaskToReadyList(pstTarget);
    }
  }

  kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
  // --- CRITCAL SECTION END ---

  return TRUE;
}

/*
  Return total count of ready tasks
*/
int kGetReadyTaskCount(void)
{
  int iTotalCount = 0;
  int i;

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stScheduler.stSpinLock));

  for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
    iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
  }

  kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
  // --- CRITCAL SECTION END ---

  return iTotalCount;
}

/*
  Return ready tasks + wait tasks + idle task(1)
*/
int kGetTaskCount(void)
{
  int iTotalCount = 0;

  iTotalCount = kGetReadyTaskCount();

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stScheduler.stSpinLock));

  iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;

  kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
  // --- CRITCAL SECTION END ---

  return iTotalCount;
}

/*
  get TCB using offset
*/
TCB* kGetTCBInTCBPool(int iOffset)
{
  if ((iOffset < -1) || (iOffset > TASK_MAXCOUNT))
    return NULL;

  return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}

/*
  check task exist using Task ID
*/
BOOL kIsTaskExist(QWORD qwID)
{
  TCB* pstTCB;

  pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));

  if ((pstTCB == NULL) || (pstTCB->stLink.qwID != qwID)) {
    return FALSE;
  }
  return TRUE;
}

/*
  Return processor load
*/
QWORD kGetProcessorLoad(void)
{
  return gs_stScheduler.qwProcessorLoad;
}

/*
  switch task 
*/
void kSchedule(void)
{
  TCB* pstRunningTask, *pstNextTask;
  BOOL bPreviousFlag;

  // nothing ready Task
  if (kGetReadyTaskCount() < 1)
    return;

  // --- CRITCAL SECTION BEGIN ---
  bPreviousFlag = kSetInterruptFlag(FALSE);
  kLockForSpinLock(&(gs_stScheduler.stSpinLock));

  // get next task
  pstNextTask = kGetNextTaskToRun();
  if (pstNextTask == NULL) {
    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
    kSetInterruptFlag(bPreviousFlag);
    // --- CRITCAL SECTION END ---
    return;
  }

  // change running task
  pstRunningTask = gs_stScheduler.pstRunningTask;
  gs_stScheduler.pstRunningTask = pstNextTask;

  // if running task is idle task, record spend processor time
  if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
    gs_stScheduler.qwSpendProcessorTimeInIdleTask += (TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime);
  }

  // if Next Task is not last FPU used Task, Set TS bit for reload or init  
  if (gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
    kSetTS();
  }
  // Next Task is last FPU used Task, Clear TS bit for
  else {
    kClearTS();
  }

  // Processor time assign
  gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

  // if running task end, move to wait list
  if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
    kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);

    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
    // --- CRITCAL SECTION END ---

    kSwitchContext(NULL, &(pstNextTask->stContext));
  }
  // not end
  else {
    // running task move to ready task list
    kAddTaskToReadyList(pstRunningTask);
    // context switching

    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
    // --- CRITCAL SECTION END ---

    kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
  }

  kSetInterruptFlag(bPreviousFlag);
}

/*
  switch task in interrupt
*/
BOOL kScheduleInInterrupt(void)
{
  TCB* pstRunningTask, *pstNextTask;
  char* pcContextAddress;

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stScheduler.stSpinLock));

  // nothing ready Task
  pstNextTask = kGetNextTaskToRun();
  if (pstNextTask == NULL) {
    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

  // change running task
  pstRunningTask = gs_stScheduler.pstRunningTask;
  gs_stScheduler.pstRunningTask = pstNextTask;

  // if running task is idle task, record spend processor time
  if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
    gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
  }

  // if running task end, move to wait list
  if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
    kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
  }
  // not end
  else {
    // running task's context copy to TCB and move to ready task list
    kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
    kAddTaskToReadyList(pstRunningTask);
  }

  kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
  // --- CRITCAL SECTION END ---

  // if Next Task is not last FPU used Task, Set TS bit for reload or init  
  if (gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
    kSetTS();
  }
  // Next Task is last FPU used Task, Clear TS bit for
  else {
    kClearTS();
  }

  kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

  return TRUE;
}

/*
  Decrease Processor time
*/
void kDecreaseProcessorTime(void)
{
  if (gs_stScheduler.iProcessorTime > 0)
    gs_stScheduler.iProcessorTime--;
}

/*
  check processor time is 0 to switch task
*/
BOOL kIsProcessorTimeExpired(void)
{
  if (gs_stScheduler.iProcessorTime <= 0)
    return TRUE;
  return FALSE;
}

/*
  set task to wait state
*/
BOOL kEndTask(QWORD qwTaskID)
{
  TCB* pstTarget;
  BYTE bPriority;

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stScheduler.stSpinLock));

  // if task is running task
  pstTarget = gs_stScheduler.pstRunningTask;
  if (pstTarget->stLink.qwID == qwTaskID) {
    pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
    SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
    // --- CRITCAL SECTION END ---

    kSchedule();

    while (1);
  }
  // if target in ready task list
  else {
    // find task TCB
    // if task is not running, set priority
    pstTarget = kRemoveTaskFromReadyList(qwTaskID);
    if (pstTarget == NULL) {
      pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
      if (pstTarget != NULL) {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
      }
      kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
      // --- CRITCAL SECTION END ---
      return TRUE;
    }
    // if task in ready list, pop task, set priority, push wait list
    pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
    SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
    kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
  }

  kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
  // --- CRITCAL SECTION END ---
  return TRUE;
}

/*
  Exit running task
*/
void kExitTask(void)
{
  kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

/*
  Idle task
  clear wait task
*/
void kIdleTask(void)
{
  TCB* pstTask, *pstChildThread, *pstProcess;
  QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
  QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
  int i, iCount;
  QWORD qwTaskID;
  void* pstThreadLink;

  qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
  qwLastMeasureTickCount = kGetTickCount();

  while (1) {
    qwCurrentMeasureTickCount = kGetTickCount();
    qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

    if ((qwCurrentMeasureTickCount - qwLastMeasureTickCount) == 0) {
      gs_stScheduler.qwProcessorLoad = 0;
    }
    else {
      gs_stScheduler.qwProcessorLoad = 100 -
        (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) *
        100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount);
    }

    qwLastMeasureTickCount = qwCurrentMeasureTickCount;
    qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

    // 
    kHaltProcessorByLoad();

    // free wait task 
    if (kGetListCount(&(gs_stScheduler.stWaitList)) >= 0) {
      while (1) {
        // --- CRITCAL SECTION BEGIN ---
        kLockForSpinLock(&(gs_stScheduler.stSpinLock));

        pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
        if (pstTask == NULL) {

          kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
          // --- CRITCAL SECTION END ---

          break;
        }
        // if Process
        if (pstTask->qwFlags & TASK_FLAGS_PROCESS) {
          iCount = kGetListCount(&(pstTask->stChildThreadList));
          // loop child thread list
          for (i = 0; i < iCount; i++) {
            pstThreadLink = (TCB*)kRemoveListFromHeader(&(pstTask->stChildThreadList));
            if (pstThreadLink == NULL) {
              break;
            }

            pstChildThread = GETTCBFROMTHREADLINK(pstThreadLink);

            kAddListToTail(&(pstTask->stChildThreadList), &(pstChildThread->stThreadLink));

            kEndTask(pstChildThread->stLink.qwID);
          }

          if (kGetListCount(&(pstTask->stChildThreadList)) > 0) {
            kAddListToTail(&(gs_stScheduler.stWaitList), pstTask);

            kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
            // --- CRITCAL SECTION END ---
            continue;
          }
          else {
            // TODO: later
          }
        }
        else if (pstTask->qwFlags & TASK_FLAGS_THREAD) {
          pstProcess = kGetProcessByThread(pstTask);
          if (pstProcess != NULL) {
            kRemoveList(&(pstProcess->stChildThreadList), pstTask->stLink.qwID);
          }
        }

        qwTaskID = pstTask->stLink.qwID;
        kFreeTCB(qwTaskID);

        kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
        // --- CRITCAL SECTION END ---
        kPrintf("IDLE: Task ID[0x%q] is completely ended.\n", pstTask->stLink.qwID);
      }
    }

    kSchedule();
  }
}

/*

*/
void kHaltProcessorByLoad(void)
{
  if (gs_stScheduler.qwProcessorLoad < 40) {
    kHlt();
    kHlt();
    kHlt();
  }
  else if (gs_stScheduler.qwProcessorLoad < 80) {
    kHlt();
    kHlt();
  }
  else if (gs_stScheduler.qwProcessorLoad < 95) {
    kHlt();
  }
}

/*
  Return thread parent Process's TCB
*/
static TCB* kGetProcessByThread(TCB* pstThread)
{
  TCB* pstProcess;

  // if process
  if (pstThread->qwFlags & TASK_FLAGS_PROCESS) {
    return pstThread;
  }

  pstProcess = kGetTCBInTCBPool(GETTCBOFFSET(pstThread->qwParentProcessID));
  if ((pstProcess == NULL) || (pstProcess->stLink.qwID != pstThread->qwParentProcessID)) {
    return NULL;
  }
  return pstProcess;
}

/*
  Get Last Used FPU Task ID
*/
QWORD kGetLastFPUUsedTaskID(void)
{
  return gs_stScheduler.qwLastFPUUsedTaskID;
}

/*
  Set Last Used FPU Task ID
*/
void kSetLastFPUUsedTaskID(QWORD qwTaskID)
{
  gs_stScheduler.qwLastFPUUsedTaskID = qwTaskID;
}