#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"

// macro
// SS, RSP, RFLAGS, CS, RIP + 19 Register
#define TASK_REGISTERCOUNT (5 + 19)
#define TASK_REGISTERSIZE 8

// Context Register Offset
#define TASK_GSOFFSET           0
#define TASK_FSOFFSET           1
#define TASK_ESOFFSET           2
#define TASK_DSOFFSET           3
#define TASK_R15OFFSET          4
#define TASK_R14OFFSET          5
#define TASK_R13OFFSET          6
#define TASK_R12OFFSET          7
#define TASK_R11OFFSET          8
#define TASK_R10OFFSET          9
#define TASK_R9OFFSET           10
#define TASK_R8OFFSET           11
#define TASK_RSIOFFSET          12
#define TASK_RDIOFFSET          13
#define TASK_RDXOFFSET          14
#define TASK_RCXOFFSET          15
#define TASK_RBXOFFSET          16
#define TASK_RAXOFFSET          17
#define TASK_RBPOFFSET          18
#define TASK_RIPOFFSET          19
#define TASK_CSOFFSET           20
#define TASK_RFLAGSOFFSET       21
#define TASK_RSPOFFSET          22
#define TASK_SSOFFSET           23

// Taks Pool
#define TASK_TCBPOOLADDRESS 0x800000
#define TASK_MAXCOUNT 1024

// Stack Pool
#define TASK_STACKPOOLADDRESS (TASK_TCBPOOLADDRESS + sizeof(TCB) * TASK_MAXCOUNT)
#define TASK_STACKSIZE 8192

// invalid task ID
#define TASK_INVALIDID 0xFFFFFFFFFFFFFFFF

// RR each task time (5ms)
#define TASK_RPOCESSORTIME 5

#pragma pack(push, 1)

typedef struct kContextStruct {
  QWORD vqRegister[TASK_REGISTERCOUNT];
} CONTEXT;

typedef struct kTaskControlBlockStruct {
  // Using List
  LISTLINK stLink;

  // flags
  QWORD qwFlags;

  // Context
  CONTEXT stContext;

  // stack
  void* pvStackAddress;
  QWORD qwStackSize
} TCB;

typedef struct kTCBPoolManagerStruct
{
  // Task Pool
  TCB* pstStartAddress;
  int iMaxCount;
  int iUseCount;

  int iAllocatedCount;
} TCBPOOLMANAGER;

typedef struct kSchedulerStruct
{
  // running task
  TCB* pstRunningTask;

  // remain tile
  int iProcessorTime;

  // Task List
  LIST stReadyList;
} SCHEDULER;
#pragma pack(pop)

// function
// Task
void kInitializeTCBPOOL(void);
TCB* kAllocateTCB(void);
void kFreeTCB(QWORD qwID);
TCB* kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress);
void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize);

// Schedule
void kInitializeScheduler(void);
void kSetRunningTask(TCB* pstTask);
TCB* kGetRunningTask(void);
TCB* kGetNextTaskToRun(void);
void kAddTaskToReadyList(TCB* pstTask);
void kSchedule(void);
BOOL kScheduleInInterrupt(void);
void kDecreaseProcessorTime(void);
BOOL kIsProcessorTimeExpired(void);

#endif // !__TASK_H__
