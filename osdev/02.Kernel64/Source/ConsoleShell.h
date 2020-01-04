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
void kHelp(const char* pcParameterBuffer);
void kCls(const char* pcParameterBuffer);
void kShowTotalRAMSize(const char* pcParameterBuffer);
void kStringToDecimalHexTest(const char* pcParameterBuffer);
void kShutDown(const char* pcParameterBuffer);

// command (PIT, RCT)
void kSetTimer(const char* pcParameterBuffer);
void kWaitUsingPIT(const char* pcParameterBuffer);
void kReadTimeStampCounter(const char* pcParameterBuffer);
void kMeasureProcessorSpeed(const char* pcParameterBuffer);
void kShowDateAndTime(const char* pcParameterBuffer);

// Task
void kCreateTestTask(const char* pcParameterBuffer);

#endif // __CONSOLESHELL_H__
