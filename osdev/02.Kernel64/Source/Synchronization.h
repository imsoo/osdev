#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include "Types.h"

#pragma pack(push, 1)

typedef struct kMutexStruct {
  volatile QWORD qwTaskID;
  volatile DWORD dwLockCount;
  volatile BOOL bLockFlag;

  BYTE vbPadding[3];
} MUTEX ;

#pragma pack(pop)

// function
BOOL kLockForSystemData(void);
void kUnlockForSystemData(BOOL bInterruptFlag);

void kInitializeMutex(MUTEX* pstMutex);
void kLock(MUTEX* pstMutex);
void kUnlock(MUTEX* pstMutex);

#endif // !__SYNCHRONIZATION_H__
