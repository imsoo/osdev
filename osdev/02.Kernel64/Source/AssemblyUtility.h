#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"
#include "Task.h"

BYTE kInPortByte(WORD wPort);
void kOutPortByte(WORD wPort, BYTE bData);

void kLoadGDTR(QWORD qwGDTRAddress);
void kLoadTR(WORD wTSSSegmentOffset);
void kLoadIDTR(QWORD qwIDTRAddress);

void kEnableInterrupt(void);
void kDisableInterrupt(void);
QWORD kReadRFLAGS(void);

// TSC
QWORD kReadTSC(void);

// TASK
void kSwitchContext(CONTEXT* pstCurrentContext, CONTEXT* pstNextContext);
void kHlt(void);
BOOL kTestAndSet(volatile BYTE* pbDestination, BYTE bCompare, BYTE bSource);

// FPU
void kInitializeFPU(void);
void kSaveFPUContext(void* pvFPUContext);
void kLoadFPUContext(void* pvFPUContext);
void kSetTS(void);
void kClearTS(void);

#endif /* __ASSEMBLYUTILITY_H__ */