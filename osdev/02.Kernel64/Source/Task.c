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
TCB* kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress)
{
  TCB* pstTask;
  void* pvStackAddress;
  BOOL bPreviousFlag;

  // --- CRITCAL SECTION BEGIN ---
  bPreviousFlag = kLockForSystemData();

  pstTask = kAllocateTCB();
  if (pstTask == NULL) {
    kUnlockForSystemData(bPreviousFlag);
    // --- CRITCAL SECTION END ---
    return NULL;
  }

  kUnlockForSystemData(bPreviousFlag);
  // --- CRITCAL SECTION END ---

  pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * (GETTCBOFFSET(pstTask->stLink.qwID))));
  kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

  // --- CRITCAL SECTION BEGIN ---
  bPreviousFlag = kLockForSystemData();

  kAddTaskToReadyList(pstTask);

  kUnlockForSystemData(bPreviousFlag);
  // --- CRITCAL SECTION END ---

  return pstTask;
}

/*
  set up Task
*/
static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize)
{
  kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));

  // set RSP, RBP
  pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress + qwStackSize;
  pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress + qwStackSize;

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

  // Init TCB pool
  kInitializeTCBPOOL();

  // Init Multi level Task List
  for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
    kInitalizeList(&(gs_stScheduler.vstReadyList[i]));
    gs_stScheduler.viExecuteCount[i] = 0;
  }
  kInitalizeList(&(gs_stScheduler.stWaitList));

  // Init TCB for running task (boot task : shell)
  gs_stScheduler.pstRunningTask = kAllocateTCB();
  gs_stScheduler.pstRunningTask->qwFlags = TASK_FLAGS_HIGHEST;

  gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
  gs_stScheduler.qwProcessorLoad = 0;
}

/*
  Set Running Task
*/
void kSetRunningTask(TCB* pstTask)
{
  BOOL bPreviousFlag;

  // --- CRITCAL SECTION BEGIN ---
  bPreviousFlag = kLockForSystemData();

  gs_stScheduler.pstRunningTask = pstTask;

  kUnlockForSystemData(bPreviousFlag);
  // --- CRITCAL SECTION END ---
}

/*
  Get Running Task TCB
*/
TCB* kGetRunningTask(void)
{
  BOOL bPreviousFlag;
  TCB* pstRunningTask;

  // --- CRITCAL SECTION BEGIN ---
  bPreviousFlag = kLockForSystemData();

  pstRunningTask = gs_stScheduler.pstRunningTask;

  kUnlockForSystemData(bPreviousFlag);
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

  // invalid priority
  if (bPriority >= TASK_MAXREADYLISTCOUNT)
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
  BOOL bPreviousFlag;

  // invalid task priority
  if (bPriority > TASK_MAXREADYLISTCOUNT) {
    return FALSE;
  }

  // --- CRITCAL SECTION BEGIN ---
  bPreviousFlag = kLockForSystemData();

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
      else {
        return FALSE;
      }
    }
    // if task in ready list, pop task, set priority, push task
    else {
      SETPRIORITY(pstTarget->qwFlags, bPriority);
      kAddTaskToReadyList(pstTarget);
    }
  }

  kUnlockForSystemData(bPreviousFlag);
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
  BOOL bPreviousFlag;

  // --- CRITCAL SECTION BEGIN ---
  bPreviousFlag = kLockForSystemData();

  for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
    iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
  }

  kUnlockForSystemData(bPreviousFlag);
  // --- CRITCAL SECTION END ---

  return iTotalCount;
}

/*
  Return ready tasks + wait tasks + idle task(1)
*/
int kGetTaskCount(void)
{
  int iTotalCount = 0;
  BOOL bPreviousFlag;

  iTotalCount = kGetReadyTaskCount();

  // --- CRITCAL SECTION BEGIN ---
  bPreviousFlag = kLockForSystemData();

  iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;

  kUnlockForSystemData(bPreviousFlag);
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
  bPreviousFlag = kLockForSystemData();

  // get next task
  pstNextTask = kGetNextTaskToRun();
  if (pstNextTask == NULL) {
    kUnlockForSystemData(bPreviousFlag);
    // --- CRITCAL SECTION END ---
    return;
  }

  // change running task
  pstRunningTask = gs_stScheduler.pstRunningTask;
  gs_stScheduler.pstRunningTask = pstNextTask;

  // if running task is idle task, record spend processor time
  if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
    gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
  }

  // Processor time assign
  gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

  // if running task end, move to wait list
  if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
    kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
    kSwitchContext(NULL, &(pstNextTask->stContext));
  }
  // not end
  else {
    // running task move to ready task list
    kAddTaskToReadyList(pstRunningTask);
    // context switching
    kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
  }

  kUnlockForSystemData(bPreviousFlag);
  // --- CRITCAL SECTION END ---
}

/*
  switch task in interrupt
*/
BOOL kScheduleInInterrupt(void)
{
  TCB* pstRunningTask, *pstNextTask;
  char* pcContextAddress;
  BOOL bPreviousFlag;

  // --- CRITCAL SECTION BEGIN ---
  bPreviousFlag = kLockForSystemData();

  // nothing ready Task
  pstNextTask = kGetNextTaskToRun();
  if (pstNextTask == NULL) {
    kUnlockForSystemData(bPreviousFlag);
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

  kUnlockForSystemData(bPreviousFlag);
  // --- CRITCAL SECTION END ---

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
  BOOL bPreviousFlag;

  // --- CRITCAL SECTION BEGIN ---
  bPreviousFlag = kLockForSystemData();

  // if task is running task
  pstTarget = gs_stScheduler.pstRunningTask;
  if (pstTarget->stLink.qwID == qwTaskID) {
    pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
    SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

    kUnlockForSystemData(bPreviousFlag);
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
      kUnlockForSystemData(bPreviousFlag);
      // --- CRITCAL SECTION END ---
      return TRUE;
    }
    // if task in ready list, pop task, set priority, push wait list
    else {
      pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
      SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
      kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
    }
  }

  kUnlockForSystemData(bPreviousFlag);
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
  TCB* pstTask;
  QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
  QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
  BOOL bPreviousFlag;
  QWORD qwTaskID;

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
        bPreviousFlag = kLockForSystemData();

        pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
        if (pstTask == NULL) {

          kUnlockForSystemData(bPreviousFlag);
          // --- CRITCAL SECTION END ---

          break;
        }
        qwTaskID = pstTask->stLink.qwID;
        kFreeTCB(qwTaskID);

        kUnlockForSystemData(bPreviousFlag);
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