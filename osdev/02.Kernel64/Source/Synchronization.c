#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"
#include "AssemblyUtility.h"
#include "MultiProcessor.h"

#if 0
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
#endif

/*
  Init SpinLock
*/
void kInitializeSpinLock(SPINLOCK* pstSpinLock)
{
  pstSpinLock->bLockFlag = FALSE;
  pstSpinLock->dwLockCount = 0;
  pstSpinLock->bAPICID = 0xFF;
  pstSpinLock->bInterruptFlag = FALSE;
}

/*
  Lock SpinLock
*/
void kLockForSpinLock(SPINLOCK* pstSpinLock)
{
  BOOL bInterruptFlag;

  bInterruptFlag = kSetInterruptFlag(FALSE);

  if (kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE) {

    // if SpinLock is mine, increase lock Count
    if (pstSpinLock->bAPICID == kGetAPICID()) {
      pstSpinLock->dwLockCount++;
      return;
    }

    // wait until release
    while (kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE) {
      while (pstSpinLock->bLockFlag == TRUE) {
        kPause();
      }
    }
  }

  pstSpinLock->dwLockCount = 1;
  pstSpinLock->bAPICID = kGetAPICID();

  pstSpinLock->bInterruptFlag = bInterruptFlag;
}

/*
  Unlock SpinLock
*/
void kUnlockForSpinLock(SPINLOCK* pstSpinLock)
{
  BOOL bInterruptFlag;

  bInterruptFlag = kSetInterruptFlag(FALSE);

  // if SpinLock is not mine
  if ((pstSpinLock->bLockFlag == FALSE) ||
    (pstSpinLock->bAPICID != kGetAPICID())) {
    kSetInterruptFlag(bInterruptFlag);
    return;
  }

  if (pstSpinLock->dwLockCount > 1) {
    pstSpinLock->dwLockCount--;
    return;
  }

  bInterruptFlag = pstSpinLock->bInterruptFlag;
  pstSpinLock->bAPICID = 0xFF;
  pstSpinLock->dwLockCount = 0;
  pstSpinLock->bInterruptFlag = FALSE;

  // Lock Flag Clear has to be processed at last position
  pstSpinLock->bLockFlag = FALSE;

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