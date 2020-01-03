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

// command function
void kHelp(const char* pcParameterBuffer);
void kCls(const char* pcParameterBuffer);
void kShowTotalRAMSize(const char* pcParameterBuffer);
void kStringToDecimalHexTest(const char* pcParameterBuffer);
void kShutDown(const char* pcParameterBuffer);

#endif // __CONSOLESHELL_H__
