#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"
#include "Synchronization.h"

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
#define TASK_PROCESSORTIME 5

// Multi Level Queue Scheduler
#define TASK_MAXREADYLISTCOUNT 5

// Task Priority
#define TASK_FLAGS_HIGHEST 0
#define TASK_FLAGS_HIGH 1
#define TASK_FLAGS_MEDIUM 2
#define TASK_FLAGS_LOW 3
#define TASK_FLAGS_LOWEST 4

#define TASK_FLAGS_WAIT 0xFF

#define TASK_FLAGS_ENDTASK  0x8000000000000000
#define TASK_FLAGS_SYSTEM   0x4000000000000000
#define TASK_FLAGS_PROCESS  0x2000000000000000
#define TASK_FLAGS_THREAD   0x1000000000000000
#define TASK_FLAGS_IDLE     0x0800000000000000

// Task Affinity 
#define TASK_LOADBALANCINGID  0xFF

// using flags's lower 8 bit 
#define GETPRIORITY(X) ((X) & 0xFF)
#define SETPRIORITY(X, PRIOR) ((X) = ((X) & 0xFFFFFFFFFFFFFF00) | (PRIOR))
#define GETTCBOFFSET(X) ((X) & 0xFFFFFFFF)

#define GETTCBFROMTHREADLINK(X) (TCB*)((QWORD)(X) - offsetof(TCB, stThreadLink))

#pragma pack(push, 1)

typedef struct kContextStruct {
  QWORD vqRegister[TASK_REGISTERCOUNT];
} CONTEXT;

typedef struct kTaskControlBlockStruct {
  // Using List
  LISTLINK stLink;

  // flags
  QWORD qwFlags;

  // Process Memory
  void* pvMemoryAddress;
  QWORD qwMemorySize;

  // --- Next THREAD Link
  LISTLINK stThreadLink;
  
  // Parent Process ID
  QWORD qwParentProcessID;

  // FPU Context
  QWORD vqwFPUContext[512 / 8];

  // --- Child THREAD List
  LIST stChildThreadList;

  // Context
  CONTEXT stContext;

  // stack
  void* pvStackAddress;
  QWORD qwStackSize;

  // FPU Use
  BOOL bFPUUsed;

  // Processor Affinity 
  BYTE bAffinity;

  // Local APIC ID
  BYTE bAPICID;

  char vcPadding[9];
} TCB;

typedef struct kTCBPoolManagerStruct
{
  // SpinLock
  SPINLOCK stSpinLock;

  // Task Pool
  TCB* pstStartAddress;
  int iMaxCount;
  int iUseCount;

  int iAllocatedCount;
} TCBPOOLMANAGER;

typedef struct kSchedulerStruct
{
  // SpinLock
  SPINLOCK stSpinLock;

  // running task
  TCB* pstRunningTask;

  // remain tile
  int iProcessorTime;

  // Multi level Task List
  LIST vstReadyList[TASK_MAXREADYLISTCOUNT];

  // ready exit task list
  LIST stWaitList;

  // Each level execute count
  int viExecuteCount[TASK_MAXREADYLISTCOUNT];

  // processor spend time
  QWORD qwProcessorLoad;

  // Idle Task spend time
  QWORD qwSpendProcessorTimeInIdleTask;

  // Last Used FPU Task ID 
  QWORD qwLastFPUUsedTaskID;

  // LoadBalancing Flag
  BOOL bUseLoadBalancing;

  char vcPadding[10];
} SCHEDULER;
#pragma pack(pop)

// function (static function : internal use)
// Task
static void kInitializeTCBPOOL(void);
static TCB* kAllocateTCB(void);
static void kFreeTCB(QWORD qwID);
static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize);
TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress, BYTE bAffinity);

// Schedule
void kInitializeScheduler(void);

void kSetRunningTask(BYTE bAPICID, TCB* pstTask);
TCB* kGetRunningTask(BYTE bAPICID);

static TCB* kGetNextTaskToRun(BYTE bAPICID);

int kGetReadyTaskCount(BYTE bAPICID);
int kGetTaskCount(BYTE bAPICID);
TCB* kGetTCBInTCBPool(int iOffset);
static BOOL kAddTaskToReadyList(BYTE bAPICID, TCB* pstTask);
static TCB* kRemoveTaskFromReadyList(BYTE bAPICID, QWORD qwTaskID);
BOOL kSchedule(void);
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority);
BOOL kScheduleInInterrupt(void);
void kDecreaseProcessorTime(BYTE bAPICID);
BOOL kIsProcessorTimeExpired(BYTE bAPICID);
BOOL kEndTask(QWORD qwTaskID);
void kExitTask(void);
BOOL kIsTaskExist(QWORD qwID);
QWORD kGetProcessorLoad(BYTE bAPICID);
void kAddTaskToScheduleWithLoadBalancing(TCB* pstTask);
static BYTE kFindSchedulerOfMinimumTaskCount(const TCB* pstTask);
BYTE kSetTaskLoadBalancing(BYTE bAPICID, BOOL bUseLoadBalancing);
static BOOL kFindSchedulerOfTaskAndLock(QWORD qwTaskID, BYTE* pbAPICID);
BOOL kChangeProcessorAffinity(QWORD qwTaskID, BYTE bAffinity);

// idle task
void kIdleTask(void);
void kHaltProcessorByLoad(BYTE bAPICID);

// Thread
static TCB* kGetProcessByThread(TCB* pstThread);

// FPU
QWORD kGetLastFPUUsedTaskID(BYTE bAPICID);
void kSetLastFPUUsedTaskID(BYTE bAPICID, QWORD qwTaskID);

#endif // !__TASK_H__