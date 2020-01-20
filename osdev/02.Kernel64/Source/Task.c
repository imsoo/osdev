#include "Task.h"
#include "MultiProcessor.h"
#include "Descriptor.h" // for kernel segment
#include "DynamicMemory.h"

//
static SCHEDULER gs_vstScheduler[MAXPROCESSORCOUNT];
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

  // Init SpinLock
  kInitializeSpinLock(&gs_stTCBPoolManager.stSpinLock);
}

/*
  Allocate TCB from TCB Pool
*/
static TCB* kAllocateTCB(void)
{
  TCB* pstEmptyTCB;
  int i;

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stTCBPoolManager.stSpinLock));

  // all TCB allocated
  if (gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount) {
    kUnlockForSpinLock(&(gs_stTCBPoolManager.stSpinLock));
    // --- CRITCAL SECTION END ---
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

  kUnlockForSpinLock(&(gs_stTCBPoolManager.stSpinLock));
  // --- CRITCAL SECTION END ---

  return pstEmptyTCB;
}

/*
  Free TCB to TCB Pool
*/
static void kFreeTCB(QWORD qwID)
{
  int i;

  i = GETTCBOFFSET(qwID);

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stTCBPoolManager.stSpinLock));

  kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
  // sign TCB[i] is free
  gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
  gs_stTCBPoolManager.iUseCount--;

  kUnlockForSpinLock(&(gs_stTCBPoolManager.stSpinLock));
  // --- CRITCAL SECTION END ---
}

/*
  Allocate TCB and set up Task
*/
TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress, BYTE bAffinity)
{
  TCB* pstTask, *pstProcess;
  void* pvStackAddress;
  BYTE bCurrentAPICID;

  bCurrentAPICID = kGetAPICID();

  pstTask = kAllocateTCB();
  if (pstTask == NULL) {
    return NULL;
  }

  // ALlocate Stack Memory
  pstTask->pvStackAddress = kAllocateMemory(TASK_STACKSIZE);
  if (pstTask->pvStackAddress == NULL) {
    kFreeTCB(pstTask->stLink.qwID);
    return NULL;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

  pstProcess = kGetProcessByThread(kGetRunningTask(bCurrentAPICID));
  if (pstProcess == NULL) {
    kFreeTCB(pstTask->stLink.qwID);
    kFreeMemory(pstTask->pvStackAddress);
    kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
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
    pstTask->pvMemoryAddress = pvMemoryAddress;
    pstTask->qwMemorySize = qwMemorySize;
  }

  // Thread id = task id
  pstTask->stThreadLink.qwID = pstTask->stLink.qwID;

  kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
  // --- CRITCAL SECTION END ---

  kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pstTask->pvStackAddress, TASK_STACKSIZE);

  // init Child Thread List
  kInitializeList(&(pstTask->stChildThreadList));

  // Init FPU
  pstTask->bFPUUsed = FALSE;

  // Set APICID & Affinity
  pstTask->bAPICID = bCurrentAPICID;
  pstTask->bAffinity = bAffinity;

  // Add Task to Scheduler
  kAddTaskToSchedulerWithLoadBalancing(pstTask);

  return pstTask;
}

/*
  Return APICID which has Minimum Task count
*/
static BYTE kFindSchedulerOfMinimumTaskCount(const TCB* pstTask)
{
  BYTE bPriority;
  BYTE i;
  int iCureentTaskCount;
int iMinTaskCount;
BYTE bMinCoreIndex;
int iTempTaskCount;
int iProcessorCount;

iProcessorCount = kGetProcessorCount();

// if Single Processor
if (iProcessorCount == 1) {
  return pstTask->bAPICID;
}

bPriority = GETPRIORITY(pstTask->qwFlags);

iCureentTaskCount = kGetListCount(&(gs_vstScheduler[pstTask->bAPICID].vstReadyList[bPriority]));

iMinTaskCount = TASK_MAXCOUNT;
bMinCoreIndex = pstTask->bAPICID;

// find APICID which has Minimum Task count 
for (i = 0; i < iProcessorCount; i++) {
  if (i == pstTask->bAPICID) {
    continue;
  }

  iTempTaskCount = kGetListCount(&(gs_vstScheduler[i].vstReadyList[bPriority]));

  if ((iTempTaskCount + 2 <= iCureentTaskCount) &&
    (iTempTaskCount < iMinTaskCount)) {
    bMinCoreIndex = i;
    iMinTaskCount = iTempTaskCount;
  }
}

return bMinCoreIndex;
}

void kAddTaskToSchedulerWithLoadBalancing(TCB* pstTask)
{
  BYTE bCurrentAPICID;
  BYTE bTargetAPICID;

  bCurrentAPICID = pstTask->bAPICID;

  // if Set LoadBalancing, move to minimum task APIC ID
  if ((gs_vstScheduler[bCurrentAPICID].bUseLoadBalancing == TRUE) &&
    (pstTask->bAffinity == TASK_LOADBALANCINGID)) {
    bTargetAPICID = kFindSchedulerOfMinimumTaskCount(pstTask);
  }
  // if Set Affinity, move to Affinity Processor
  else if ((pstTask->bAffinity != bCurrentAPICID) &&
    (pstTask->bAffinity != TASK_LOADBALANCINGID)) {
    bTargetAPICID = pstTask->bAffinity;
  }
  // Don't move
  else {
    bTargetAPICID = bCurrentAPICID;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

  // FPU restore
  if ((bCurrentAPICID != bTargetAPICID) &&
    (pstTask->stLink.qwID == gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID))
  {
    kClearTS();
    kSaveFPUContext(pstTask->vqwFPUContext);
    gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID = TASK_INVALIDID;
  }

  kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
  // --- CRITCAL SECTION END ---

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_vstScheduler[bTargetAPICID].stSpinLock));

  pstTask->bAPICID = bTargetAPICID;
  kAddTaskToReadyList(bTargetAPICID, pstTask);

  kUnlockForSpinLock(&(gs_vstScheduler[bTargetAPICID].stSpinLock));
  // --- CRITCAL SECTION END ---
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

  // if Kernel Task
  if ((qwFlags & TASK_FLAGS_USERLEVEL) == 0) {
    // Set Segment Selector
    pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT | SELECTOR_RPL_0;
    pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
    pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
    pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
    pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
    pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
  }
  // User Task
  else {
    // Set Segment Selector
    pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_USERCODESEGMENT | SELECTOR_RPL_3;
    pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
    pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
    pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
    pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
    pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
  }

  // RIP Register
  pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;

  // set Interrupt Flags (bit 9), IOPL(bit 12 ~ 13) 
  // User Task can access I/O Port
  pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x3200;

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
  int i, j;
  BYTE bCurrentAPICID;
  TCB* pstTask;

  // Current Processor's APICID
  bCurrentAPICID = kGetAPICID();

  // if BSP
  if (bCurrentAPICID == 0) {
    // Init TCB pool
    kInitializeTCBPOOL();

    for (j = 0; j < MAXPROCESSORCOUNT; j++) {
      // Init Scheduler Multi level Task  Ready List
      for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        kInitializeList(&(gs_vstScheduler[j].vstReadyList[i]));
        gs_vstScheduler[j].viExecuteCount[i] = 0;
      }

      // Init Scheduler Wait List
      kInitializeList(&(gs_vstScheduler[j].stWaitList));

      // Init Scheduler SpinLock
      kInitializeSpinLock(&(gs_vstScheduler[j].stSpinLock));
    }
  }

  // Init TCB for running task (boot task : shell OR IDLE)
  pstTask = kAllocateTCB();
  gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstTask;

  pstTask->bAPICID = bCurrentAPICID;
  pstTask->bAffinity = bCurrentAPICID;

  // if BP : Run Console Shell
  if (bCurrentAPICID == 0) {
    pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
  }
  // else AP : Run IDLE Task
  else {
    pstTask->qwFlags = TASK_FLAGS_LOWEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE;
  }

  pstTask->qwParentProcessID = pstTask->stLink.qwID;
  pstTask->pvMemoryAddress = (void*)0x100000;
  pstTask->qwMemorySize = 0x500000;
  pstTask->pvStackAddress = (void*)0x600000;
  pstTask->qwStackSize = 0x100000;

  gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask = 0;
  gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 0;
  gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID = TASK_INVALIDID;
}

/*
  Set Running Task
*/
void kSetRunningTask(BYTE bAPICID, TCB* pstTask)
{
  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

  gs_vstScheduler[bAPICID].pstRunningTask = pstTask;

  kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
  // --- CRITCAL SECTION END ---
}

/*
  Get Running Task TCB
*/
TCB* kGetRunningTask(BYTE bAPICID)
{
  TCB* pstRunningTask;

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

  pstRunningTask = gs_vstScheduler[bAPICID].pstRunningTask;

  kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
  // --- CRITCAL SECTION END ---

  return pstRunningTask;
}

/*
  Get Next Task TCB from multi level ready task list
*/
static TCB* kGetNextTaskToRun(BYTE bAPICID)
{
  TCB* pstTarget = NULL;
  int iTaskCount, i, j;

  for (j = 0; j < 2; j++) {
    for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
      iTaskCount = kGetListCount(&(gs_vstScheduler[bAPICID].vstReadyList[i]));
      // return task in current list
      if (gs_vstScheduler[bAPICID].viExecuteCount[i] < iTaskCount) {
        pstTarget = (TCB*)kRemoveListFromHeader(&(gs_vstScheduler[bAPICID].vstReadyList[i]));
        gs_vstScheduler[bAPICID].viExecuteCount[i]++;
        break;
      }
      // if execute all task, reset execute count and move to next prioity list
      else {
        gs_vstScheduler[bAPICID].viExecuteCount[i] = 0;
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
static BOOL kAddTaskToReadyList(BYTE bAPICID, TCB* pstTask)
{
  BYTE bPriority;

  bPriority = GETPRIORITY(pstTask->qwFlags);

  // WAIT
  if (bPriority == TASK_FLAGS_WAIT)
  {
    kAddListToTail(&(gs_vstScheduler[bAPICID].stWaitList), pstTask);
    return TRUE;
  }
  // invalid priority
  else if (bPriority >= TASK_MAXREADYLISTCOUNT)
    return FALSE;

  kAddListToTail(&(gs_vstScheduler[bAPICID].vstReadyList[bPriority]), pstTask);
  return TRUE;
}

/*
  Remove Task TCB from Multi level ready task list
*/
static TCB* kRemoveTaskFromReadyList(BYTE bAPICID, QWORD qwTaskID)
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
  if (bPriority >= TASK_MAXREADYLISTCOUNT)
  {
    return NULL;
  }

  pstTarget = kRemoveList(&(gs_vstScheduler[bAPICID].vstReadyList[bPriority]), qwTaskID);

  return pstTarget;
}

/*
  change Task prioiry using task id
*/
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority)
{
  TCB* pstTarget;
  BYTE bAPICID;

  // invalid task priority
  if (bPriority > TASK_MAXREADYLISTCOUNT) {
    return FALSE;
  }

  // --- CRITCAL SECTION BEGIN ---
  if (kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE) {
    return FALSE;
  }

  // if target is running task, set prioirty
  pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
  if (pstTarget->stLink.qwID == qwTaskID) {
    SETPRIORITY(pstTarget->qwFlags, bPriority);
  }
  // if target in ready task list
  else {
    // find task TCB
    // if task is not running, set priority
    pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);
    if (pstTarget == NULL) {
      pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
      if (pstTarget != NULL) {
        SETPRIORITY(pstTarget->qwFlags, bPriority);
      }
    }
    // if task in ready list, pop task, set priority, push task
    else {
      SETPRIORITY(pstTarget->qwFlags, bPriority);
      kAddTaskToReadyList(bAPICID, pstTarget);
    }
  }

  kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
  // --- CRITCAL SECTION END ---

  return TRUE;
}

/*
  Return total count of ready tasks
*/
int kGetReadyTaskCount(BYTE bAPICID)
{
  int iTotalCount = 0;
  int i;

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

  for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
    iTotalCount += kGetListCount(&(gs_vstScheduler[bAPICID].vstReadyList[i]));
  }

  kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
  // --- CRITCAL SECTION END ---

  return iTotalCount;
}

/*
  Return ready tasks + wait tasks + idle task(1)
*/
int kGetTaskCount(BYTE bAPICID)
{
  int iTotalCount = 0;

  iTotalCount = kGetReadyTaskCount(bAPICID);

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

  iTotalCount += kGetListCount(&(gs_vstScheduler[bAPICID].stWaitList)) + 1;

  kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
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
QWORD kGetProcessorLoad(BYTE bAPICID)
{
  return gs_vstScheduler[bAPICID].qwProcessorLoad;
}

/*
  switch task
*/
BOOL kSchedule(void)
{
  TCB* pstRunningTask, *pstNextTask;
  BOOL bPreviousFlag;
  BYTE bCurrentAPICID;

  bPreviousFlag = kSetInterruptFlag(FALSE);
  bCurrentAPICID = kGetAPICID();

  // nothing ready Task
  if (kGetReadyTaskCount(bCurrentAPICID) < 1) {
    kSetInterruptFlag(bPreviousFlag);
    return FALSE;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

  // get next task
  pstNextTask = kGetNextTaskToRun(bCurrentAPICID);
  if (pstNextTask == NULL) {
    kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
    kSetInterruptFlag(bPreviousFlag);
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // change running task
  pstRunningTask = gs_vstScheduler[bCurrentAPICID].pstRunningTask;
  gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstNextTask;

  // if running task is idle task, record spend processor time
  if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
    gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask += (TASK_PROCESSORTIME - gs_vstScheduler[bCurrentAPICID].iProcessorTime);
  }

  // if Next Task is not last FPU used Task, Set TS bit for reload or init  
  if (gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
    kSetTS();
  }
  // Next Task is last FPU used Task, Clear TS bit for
  else {
    kClearTS();
  }

  // Processor time assign
  gs_vstScheduler[bCurrentAPICID].iProcessorTime = TASK_PROCESSORTIME;

  // if running task end, move to wait list
  if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
    kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstRunningTask);

    kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
    // --- CRITCAL SECTION END ---

    kSwitchContext(NULL, &(pstNextTask->stContext));
  }
  // not end
  else {
    // running task move to ready task list
    kAddTaskToReadyList(bCurrentAPICID, pstRunningTask);
    // context switching

    kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
    // --- CRITCAL SECTION END ---

    kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
  }

  kSetInterruptFlag(bPreviousFlag);
  return FALSE;
}

/*
  switch task in interrupt
*/
BOOL kScheduleInInterrupt(void)
{
  TCB* pstRunningTask, *pstNextTask;
  char* pcContextAddress;
  BYTE bCurrentAPICID;
  QWORD qwISTStartAddress;

  bCurrentAPICID = kGetAPICID();

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

  // nothing ready Task
  pstNextTask = kGetNextTaskToRun(bCurrentAPICID);
  if (pstNextTask == NULL) {
    kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  qwISTStartAddress = IST_STARTADDRESS + IST_SIZE - (IST_SIZE / MAXPROCESSORCOUNT * bCurrentAPICID);
  pcContextAddress = (char*)qwISTStartAddress - sizeof(CONTEXT);

  // change running task
  pstRunningTask = gs_vstScheduler[bCurrentAPICID].pstRunningTask;
  gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstNextTask;

  // if running task is idle task, record spend processor time
  if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
    gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
  }

  // if running task end, move to wait list
  if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
    kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstRunningTask);
  }
  // not end
  else {
    // running task's context copy to TCB and move to ready task list
    kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
  }

  // if Next Task is not last FPU used Task, Set TS bit for reload or init  
  if (gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
    kSetTS();
  }
  // Next Task is last FPU used Task, Clear TS bit for
  else {
    kClearTS();
  }

  kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
  // --- CRITCAL SECTION END ---

  kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

  if ((pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) != TASK_FLAGS_ENDTASK) {
    kAddTaskToSchedulerWithLoadBalancing(pstRunningTask);
  }

  gs_vstScheduler[bCurrentAPICID].iProcessorTime = TASK_PROCESSORTIME;

  return TRUE;
}

/*
  Decrease Processor time
*/
void kDecreaseProcessorTime(BYTE bAPICID)
{
  gs_vstScheduler[bAPICID].iProcessorTime--;
}

/*
  check processor time is 0 to switch task
*/
BOOL kIsProcessorTimeExpired(BYTE bAPICID)
{
  if (gs_vstScheduler[bAPICID].iProcessorTime <= 0)
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
  BYTE bAPICID;

  // --- CRITCAL SECTION BEGIN ---
  if (kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE) {
    return FALSE;
  }

  // if task is running task
  pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
  if (pstTarget->stLink.qwID == qwTaskID) {
    pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
    SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

    kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
    // --- CRITCAL SECTION END ---

    if (kGetAPICID() == bAPICID) {
      kSchedule();

      while (1);
    }

    return TRUE;
  }

  // if target in ready task list
  pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);
  if (pstTarget == NULL) {
    pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
    if (pstTarget != NULL) {
      pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
      SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
    }
    kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
    // --- CRITCAL SECTION END ---
    return TRUE;
  }

  // if task in ready list, pop task, set priority, push wait list
  pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
  SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
  kAddListToTail(&(gs_vstScheduler[bAPICID].stWaitList), pstTarget);

  kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
  // --- CRITCAL SECTION END ---
  return TRUE;
}

/*
  Exit running task
*/
void kExitTask(void)
{
  kEndTask(gs_vstScheduler[kGetAPICID()].pstRunningTask->stLink.qwID);
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
  QWORD qwTaskID, qwChildThreadID;
  int i, iCount;
  void* pstThreadLink;
  BYTE bCurrentAPICD, bProcessAPICD;

  bCurrentAPICD = kGetAPICID();

  qwLastSpendTickInIdleTask = gs_vstScheduler[bCurrentAPICD].qwSpendProcessorTimeInIdleTask;
  qwLastMeasureTickCount = kGetTickCount();

  while (1) {
    qwCurrentMeasureTickCount = kGetTickCount();
    qwCurrentSpendTickInIdleTask = gs_vstScheduler[bCurrentAPICD].qwSpendProcessorTimeInIdleTask;

    if ((qwCurrentMeasureTickCount - qwLastMeasureTickCount) == 0) {
      gs_vstScheduler[bCurrentAPICD].qwProcessorLoad = 0;
    }
    else {
      gs_vstScheduler[bCurrentAPICD].qwProcessorLoad = 100 -
        (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) *
        100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount);
    }

    qwLastMeasureTickCount = qwCurrentMeasureTickCount;
    qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

    // 
    kHaltProcessorByLoad(bCurrentAPICD);

    // free wait task 
    if (kGetListCount(&(gs_vstScheduler[bCurrentAPICD].stWaitList)) > 0) {
      while (1) {
        // --- CRITCAL SECTION BEGIN ---
        kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICD].stSpinLock));

        pstTask = kRemoveListFromHeader(&(gs_vstScheduler[bCurrentAPICD].stWaitList));

        kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICD].stSpinLock));
        // --- CRITCAL SECTION END ---

        if (pstTask == NULL) {
          break;
        }

        // if Process
        if (pstTask->qwFlags & TASK_FLAGS_PROCESS) {
          iCount = kGetListCount(&(pstTask->stChildThreadList));
          // loop child thread list
          for (i = 0; i < iCount; i++) {

            // --- CRITCAL SECTION BEGIN ---
            kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICD].stSpinLock));

            pstThreadLink = (TCB*)kRemoveListFromHeader(&(pstTask->stChildThreadList));
            if (pstThreadLink == NULL) {

              kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICD].stSpinLock));
              // --- CRITCAL SECTION END ---

              break;
            }

            pstChildThread = GETTCBFROMTHREADLINK(pstThreadLink);
            kAddListToTail(&(pstTask->stChildThreadList), &(pstChildThread->stThreadLink));
            qwChildThreadID = pstChildThread->stLink.qwID;

            kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICD].stSpinLock));
            // --- CRITCAL SECTION END ---

            kEndTask(qwChildThreadID);
          }

          if (kGetListCount(&(pstTask->stChildThreadList)) > 0) {
            // --- CRITCAL SECTION BEGIN ---
            kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICD].stSpinLock));

            kAddListToTail(&(gs_vstScheduler[bCurrentAPICD].stWaitList), pstTask);

            kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICD].stSpinLock));
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

            if (kFindSchedulerOfTaskAndLock(pstProcess->stLink.qwID, &bProcessAPICD) == TRUE) {
              kRemoveList(&(pstProcess->stChildThreadList), pstTask->stLink.qwID);

              kUnlockForSpinLock(&(gs_vstScheduler[bProcessAPICD].stSpinLock));
              // --- CRITCAL SECTION END ---
            }
          }
        }

        qwTaskID = pstTask->stLink.qwID;

        kFreeMemory(pstTask->pvStackAddress);
        kFreeTCB(qwTaskID);
        kPrintf("IDLE: Task ID[0x%q] is completely ended.\n", qwTaskID);
      }
    }

    kSchedule();
  }
}

/*

*/
void kHaltProcessorByLoad(BYTE bAPICID)
{
  if (gs_vstScheduler[bAPICID].qwProcessorLoad < 40) {
    kHlt();
    kHlt();
    kHlt();
  }
  else if (gs_vstScheduler[bAPICID].qwProcessorLoad < 80) {
    kHlt();
    kHlt();
  }
  else if (gs_vstScheduler[bAPICID].qwProcessorLoad < 95) {
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
QWORD kGetLastFPUUsedTaskID(BYTE bAPICID)
{
  return gs_vstScheduler[bAPICID].qwLastFPUUsedTaskID;
}

/*
  Set Last Used FPU Task ID
*/
void kSetLastFPUUsedTaskID(BYTE bAPICID, QWORD qwTaskID)
{
  gs_vstScheduler[bAPICID].qwLastFPUUsedTaskID = qwTaskID;
}

/*
  Find Scheduler and Lock SpinLock
*/
static BOOL kFindSchedulerOfTaskAndLock(QWORD qwTaskID, BYTE* pbAPICID)
{
  TCB* pstTarget;
  BYTE bAPICID;

  while (1)
  {
    // Find Scheduler using Task ID
    pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
    if ((pstTarget == NULL) || (pstTarget->stLink.qwID != qwTaskID))
    {
      return FALSE;
    }

    // Get Current Processor APICID
    bAPICID = pstTarget->bAPICID;

    // --- CRITCAL SECTION BEGIN ---
    kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

    // double check 
    pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
    if (pstTarget->bAPICID == bAPICID)
    {
      break;
    }

    kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
    // --- CRITCAL SECTION END ---
  }

  *pbAPICID = bAPICID;
  return TRUE;
}

BOOL kChangeProcessorAffinity(QWORD qwTaskID, BYTE bAffinity)
{
  TCB* pstTarget;
  BYTE bAPICID;

  if (kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE) {
    return FALSE;
  }

  pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
  // if running task change affinity
  if (pstTarget->stLink.qwID == qwTaskID) {
    pstTarget->bAffinity = bAffinity;

    kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
    // --- CRITCAL SECTION END ---
  }
  // else remove -> change affinity -> add
  else {
    pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);
    if (pstTarget == NULL) {
      pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
      if (pstTarget != NULL)
      {
        pstTarget->bAffinity = bAffinity;
      }
    }
    else {
      pstTarget->bAffinity = bAffinity;
    }

    kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
    // --- CRITCAL SECTION END ---

    kAddTaskToSchedulerWithLoadBalancing(pstTarget);
  }

  return TRUE;
}

/*
  Set Scheduler LoadBalancing flag
*/
BYTE kSetTaskLoadBalancing(BYTE bAPICID, BOOL bUseLoadBalancing)
{
  gs_vstScheduler[bAPICID].bUseLoadBalancing = bUseLoadBalancing;
}