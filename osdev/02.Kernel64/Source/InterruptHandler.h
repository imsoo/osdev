#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "Types.h"
#include "MultiProcessor.h"

#define INTERRUPT_MAXVECTORCOUNT  16
#define INTERRUPT_LOADBALANCINGDIVIDOR  10

typedef struct kInterruptManagerStruct
{
  // Each Core Interrupt Count 
  QWORD vvqwCoreInterruptCount[MAXPROCESSORCOUNT][INTERRUPT_MAXVECTORCOUNT];

  // Load Balacing Flag
  BOOL bUseLoadBalancing;

  // Symmetric IO Flag
  BOOL bSymmetricIOMode;
} INTERRUPTMANAGER;

// function
void kSetSymmetricIOMode(BOOL bSymmetricIOMode);
void kSetInterruptLoadBalancing(BOOL bUseLoadBalancing);
void kIncreaseInterruptCount(int iIRQ);
void kSendEOI(int iIRQ);
INTERRUPTMANAGER* kGetInterruptManager(void);
void kProcessLoadBalancing(int iIRQ);

// handler
void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode);
void kCommonInterruptHandler(int iVectorNumber);
void kKeyboardHandler(int iVectorNumber);
void kMouseHandler(int iVectorNumber);
void kTimerHandler(int iVectorNumber);
void kDeviceNotAvailableHandler(int iVectorNumber);
void kHDDHandler(int iVectorNumber);
void kEthernetHandler(int iVectorNumber);


#endif // !__INTERRUPTHANDLER_H__
