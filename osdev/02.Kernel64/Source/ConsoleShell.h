#ifndef __CONSOLESHELL_H__
#define __CONSOLESHELL_H__

#include "Types.h"

#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT 300
#define CONSOLESHELL_PROMPTMESSAGE  ">"

typedef void(*CommandFunction)(const char* pcParameter);

#pragma pack(push, 1)

// for shell command
typedef struct kShellCommandEntryStruct
{
  // Command Name
  char* pcCommand;
  // Command Help String
  char* pcHelp;
  // Command function pointer
  CommandFunction pfFunction;
} SHELLCOMMANDENTRY;

// for parameter 
typedef struct kParameterListStruct
{
  // Parameter Buffer
  const char* pcBuffer;
  // Parameter Length
  int iLength;
  // Current Parameter begin Position in Buffer
  int iCurrentPosition;
} PARAMETERLIST;

#pragma pack(pop)

// function
void kStartConsoleShell(void);
void kExecuteCommand(const char* pcCommandBuffer);
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter);
int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter);

// command function
static void kHelp(const char* pcParameterBuffer);
static void kCls(const char* pcParameterBuffer);
static void kShowTotalRAMSize(const char* pcParameterBuffer);
static void kStringToDecimalHexTest(const char* pcParameterBuffer);
static void kShutDown(const char* pcParameterBuffer);

// command (PIT, RCT)
static void kSetTimer(const char* pcParameterBuffer);
static void kWaitUsingPIT(const char* pcParameterBuffer);
static void kReadTimeStampCounter(const char* pcParameterBuffer);
static void kMeasureProcessorSpeed(const char* pcParameterBuffer);
static void kShowDateAndTime(const char* pcParameterBuffer);

// Task
static void kCreateTestTask(const char* pcParameterBuffer);
static void kChangeTaskPriority(const char* pcParameterBuffer);
static void kShowTaskList(const char* pcParameterBuffer);
static void kKillTask(const char* pcParameterBuffer);
static void kCPULoad(const char* pcParameterBuffer);

#endif // __CONSOLESHELL_H__
