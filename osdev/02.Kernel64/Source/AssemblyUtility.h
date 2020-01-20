#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"
#include "Task.h"

// Read from Port
BYTE kInPortByte(WORD wPort);
WORD kInPortWord(WORD wPort);

// Write to Port
void kOutPortByte(WORD wPort, BYTE bData);
void kOutPortWord(WORD wPort, WORD wData);

void kReadMSR(QWORD qwMSRAddress, QWORD* pqwRDX, QWORD* pqwRAX);
void kWriteMSR(QWORD qwMSRAddress, QWORD qwRDX, QWORD qwRAX);

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
void kPause(void);

// FPU
void kInitializeFPU(void);
void kSaveFPUContext(void* pvFPUContext);
void kLoadFPUContext(void* pvFPUContext);
void kSetTS(void);
void kClearTS(void);

// Local APIC
void kEnableGlobalLocalAPIC(void);

#endif /* __ASSEMBLYUTILITY_H__ */