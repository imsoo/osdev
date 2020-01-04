#include "Task.h"
#include "Descriptor.h" // for kernel segment

//
static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

/*
  Init TCB Pool
*/
void kInitializeTCBPOOL(void)
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
TCB* kAllocateTCB(void)
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
void kFreeTCB(QWORD qwID)
{
  int i;

  i = qwID & 0xFFFFFFFF;

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

  pstTask = kAllocateTCB();
  if (pstTask == NULL) {
    return NULL;
  }

  pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * (pstTask->stLink.qwID & 0xFFFFFFFF)));
  kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);
  kAddTaskToReadyList(pstTask);

  return pstTask;
}

/*
  set up Task
*/
void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize)
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
  // Init TCB pool
  kInitializeTCBPOOL();

  // Init List
  kInitalizeList(&(gs_stScheduler.stReadyList));

  // Init TCB for running task (boot task)
  gs_stScheduler.pstRunningTask = kAllocateTCB();
}

/*
  Set Running Task
*/
void kSetRunningTask(TCB* pstTask)
{
  gs_stScheduler.pstRunningTask = pstTask;
}

/*
  Get Running Task TCB
*/
TCB* kGetRunningTask(void)
{
  return gs_stScheduler.pstRunningTask;
}

/*
  Get Next Task TCB from waiting task list
*/
TCB* kGetNextTaskToRun(void)
{
  // nothing waiting Task
  if (kGetListCount(&(gs_stScheduler.stReadyList)) == 0) {
    return NULL;
  }

  return (TCB*)kRemoveListFromHeader(&(gs_stScheduler.stReadyList));
}

/*
  Add Task TCB to waiting task list
*/
void kAddTaskToReadyList(TCB* pstTask)
{
  kAddListToTail(&(gs_stScheduler.stReadyList), pstTask);
}

/*
  switch task 
*/
void kSchedule(void)
{
  TCB* pstRunningTask, *pstNextTask;
  BOOL bPreviousFlag;

  // nothing waiting Task
  if (kGetListCount(&(gs_stScheduler.stReadyList)) == 0)
    return;

  bPreviousFlag = kSetInterruptFlag(FALSE);
  pstNextTask = kGetNextTaskToRun();
  if (pstNextTask == NULL) {
    kSetInterruptFlag(bPreviousFlag);
    return;
  }

  // running task move to waiting task list
  pstRunningTask = gs_stScheduler.pstRunningTask;
  kAddTaskToReadyList(pstRunningTask);

  // Processor time assign
  gs_stScheduler.iProcessorTime = TASK_RPOCESSORTIME;

  // get task from list & context switching
  gs_stScheduler.pstRunningTask = pstNextTask;
  kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
  

  kSetInterruptFlag(bPreviousFlag);
}

/*
  switch task in interrupt
*/
BOOL kScheduleInInterrupt(void)
{
  TCB* pstRunningTask, *pstNextTask;
  char* pcContextAddress;

  // nothing waiting Task
  pstNextTask = kGetNextTaskToRun();
  if (pstNextTask == NULL)
    return FALSE;

  pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

  // running task's context copy to TCB and move to waiting task list
  pstRunningTask = gs_stScheduler.pstRunningTask;
  kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
  kAddTaskToReadyList(pstRunningTask);

  // Processor time assign
  gs_stScheduler.iProcessorTime = TASK_RPOCESSORTIME;

  // get next task and next task's context copy to IST
  gs_stScheduler.pstRunningTask = pstNextTask;
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