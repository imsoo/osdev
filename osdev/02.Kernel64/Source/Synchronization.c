#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"

/*
  System Data Lock Using by Interrupt control
*/
BOOL kLockForSystemData(void)
{
  return kSetInterruptFlag(FALSE);
}

/*
  System Data UnLock Using by Interrupt control
*/
void kUnlockForSystemData(BOOL bInterruptFlag)
{
  kSetInterruptFlag(bInterruptFlag);
}

/*
  Init Mutex
*/
void kInitializeMutex(MUTEX* pstMutex)
{
  pstMutex->bLockFlag = FALSE;
  pstMutex->dwLockCount = 0;
  pstMutex->qwTaskID = TASK_INVALIDID;
}

/*
  Lock Mutex
*/
void kLock(MUTEX* pstMutex)
{
  // if Mutex lock
  if (kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE) {
    // if Mutex Owner is running task, increase lockCount
    if (pstMutex->qwTaskID == kGetRunningTask()->stLink.qwID) {
      pstMutex->dwLockCount++;
      return;
    }
    // Mutex is not mine, wait until unlock
    else {
      while (kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE) {
        kSchedule();
      }
    }
  }

  // Set Mutex
  pstMutex->dwLockCount = 1;
  pstMutex->qwTaskID = kGetRunningTask()->stLink.qwID;
}

/*
  Unlock Mutex
*/
void kUnlock(MUTEX* pstMutex)
{
  // if mutex is not lock OR mutex is lock but not mine, then return
  if ((pstMutex->bLockFlag == FALSE) || (pstMutex->qwTaskID != kGetRunningTask()->stLink.qwID))
    return;

  // mutex is mine
  if (pstMutex->dwLockCount > 1) {
    pstMutex->dwLockCount--;
    return;
  }

  // clear Mutex
  pstMutex->qwTaskID = TASK_INVALIDID;
  pstMutex->dwLockCount = 0;
  pstMutex->bLockFlag = FALSE;
}