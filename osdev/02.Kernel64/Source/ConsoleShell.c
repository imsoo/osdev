#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "PIC.h"
#include "AssemblyUtility.h"

// Command Table
SHELLCOMMANDENTRY gs_vstCommandTable[] =
{
  { "help", "Show Help", kHelp },
  { "cls", "Clear Screen", kCls },
  { "totalram", "Show Total RAM Size", kShowTotalRAMSize },
  { "strtod", "String To Decial/Hex Convert", kStringToDecimalHexTest },
  { "shutdown", "Shutdown And Reboot OS", kShutDown },
  { "settimer", "Set PIT Controller Counter0, ex)settimer 10(ms) 1(periodic)", kSetTimer },
  { "wait", "Wait ms Using PIT, ex)wait 100(ms)", kWaitUsingPIT },
  { "rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter },
  { "cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed },
  { "date", "Show Date And Time", kShowDateAndTime },
  { "createtask", "Create Task", kCreateTestTask }      
};

// TCB
static TCB gs_vstTask[2] = { 0, };
static QWORD gs_vstStack[1024] = { 0, };

/*
  Shell main function
*/
void kStartConsoleShell(void)
{
  char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
  int iCommandBufferIndex = 0;
  BYTE bKey;
  int iCursorX, iCursorY;

  // print PROMPT
  kPrintf(CONSOLESHELL_PROMPTMESSAGE);

  while (1)
  {
    // wait key
    bKey = kGetCh();
    // Backspace 
    if (bKey == KEY_BACKSPACE)
    {
      if (iCommandBufferIndex > 0)
      {
        kGetCursor(&iCursorX, &iCursorY);
        kPrintStringXY(iCursorX - 1, iCursorY, " ");
        kSetCursor(iCursorX - 1, iCursorY);
        iCommandBufferIndex--;
      }
    }
    // Enter
    else if (bKey == KEY_ENTER)
    {
      kPrintf("\n");

      if (iCommandBufferIndex > 0)
      {
        // Execute Command
        vcCommandBuffer[iCommandBufferIndex] = '\0';
        kExecuteCommand(vcCommandBuffer);
      }

      // print PROMPT, Clear Command Buffer
      kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
      kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
      iCommandBufferIndex = 0;
    }
    // Shift, CAPS Lock, NUM Lock, Scroll Lock are ignored
    else if ((bKey == KEY_LSHIFT) || (bKey == KEY_RSHIFT) ||
      (bKey == KEY_CAPSLOCK) || (bKey == KEY_NUMLOCK) ||
      (bKey == KEY_SCROLLLOCK))
    {
      ;
    }
    else
    {
      // Tab to blank
      if (bKey == KEY_TAB)
      {
        bKey = ' ';
      }

      // write input key into command buffer 
      if (iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT)
      {
        vcCommandBuffer[iCommandBufferIndex++] = bKey;
        kPrintf("%c", bKey);
      }
    }
  }
}

/*
  Read command Buffer and execute command
*/
void kExecuteCommand(const char* pcCommandBuffer)
{
  int i, iSpaceIndex;
  int iCommandBufferLength, iCommandLength;
  int iCount;

  // command, parameter split
  iCommandBufferLength = kStrLen(pcCommandBuffer);
  for (iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++)
  {
    if (pcCommandBuffer[iSpaceIndex] == ' ')
    {
      break;
    }
  }

  // compare command and command table
  iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);
  for (i = 0; i < iCount; i++)
  {
    iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);

    // strcmp and strlen compare
    if ((iCommandLength == iSpaceIndex) &&
      (kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0)
      )
    {
      gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1);
      break;
    }
  }
  // unknown command
  if (i >= iCount)
  {
    kPrintf("'%s' is not found.\n", pcCommandBuffer);
  }
}

/*
  Init Param
*/
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter)
{
  pstList->pcBuffer = pcParameter;
  pstList->iLength = kStrLen(pcParameter);
  pstList->iCurrentPosition = 0;
}

/*
  return param 
*/
int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter)
{
  int i;
  int iLength;

  // param list end
  if (pstList->iLength <= pstList->iCurrentPosition)
  {
    return 0;
  }

  // find blank 
  for (i = pstList->iCurrentPosition; i < pstList->iLength; i++)
  {
    if (pstList->pcBuffer[i] == ' ')
    {
      break;
    }
  }

  // copy param
  kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
  iLength = i - pstList->iCurrentPosition;
  pcParameter[iLength] = '\0';

  // update param position in buffer
  pstList->iCurrentPosition += iLength + 1;
  return iLength;
}

// Command function
/*
  Help Command
*/
void kHelp(const char* pcCommandBuffer)
{
  int i;
  int iCount;
  int iCursorX, iCursorY;
  int iLength, iMaxCommandLength = 0;


  kPrintf("=========================================================\n");
  kPrintf("                       Shell Help                        \n");
  kPrintf("=========================================================\n");

  iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

  // Find longest command
  for (i = 0; i < iCount; i++)
  {
    iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
    if (iLength > iMaxCommandLength)
    {
      iMaxCommandLength = iLength;
    }
  }

  // print
  for (i = 0; i < iCount; i++)
  {
    kPrintf("%s", gs_vstCommandTable[i].pcCommand);
    kGetCursor(&iCursorX, &iCursorY);
    kSetCursor(iMaxCommandLength, iCursorY);
    kPrintf("  - %s\n", gs_vstCommandTable[i].pcHelp);
  }
}

/*
  Clear screen command
*/
void kCls(const char* pcParameterBuffer)
{
  kClearScreen();

  // first line use for debug
  kSetCursor(0, 1);
}

/*
  Print RAM Size
*/
void kShowTotalRAMSize(const char* pcParameterBuffer)
{
  kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

/*
  strtol and print
*/
void kStringToDecimalHexTest(const char* pcParameterBuffer)
{
  char vcParameter[100];
  int iLength;
  PARAMETERLIST stList;
  int iCount = 0;
  long lValue;

  kInitializeParameter(&stList, pcParameterBuffer);

  while (1)
  {
    iLength = kGetNextParameter(&stList, vcParameter);
    if (iLength == 0)
    {
      break;
    }

    kPrintf("Param %d = '%s', Length = %d, ", iCount + 1,
      vcParameter, iLength);

    
    if (kMemCmp(vcParameter, "0x", 2) == 0)
    {
      lValue = kAToI(vcParameter + 2, 16);
      kPrintf("HEX Value = %q\n", lValue);
    }
    else
    {
      lValue = kAToI(vcParameter, 10);
      kPrintf("Decimal Value = %d\n", lValue);
    }

    iCount++;
  }
}

/*
  reboot 
*/
void kShutDown(const char* pcParamegerBuffer)
{
  kPrintf("System Shutdown Start...\n");

  kPrintf("Press Any Key To Reboot PC...");
  kGetCh();
  kReboot();
}

/*
  set PIT counter
*/
void kSetTimer(const char* pcParameterBuffer)
{
  char vcParameter[100];
  PARAMETERLIST stList;
  long lValue;
  BOOL bPeriodic;

  // Init Param
  kInitializeParameter(&stList, pcParameterBuffer);

  // Param : milisecond
  if (kGetNextParameter(&stList, vcParameter) == 0)
  {
    kPrintf("ex)settimer 10(ms) 1(periodic)\n");
    return;
  }
  lValue = kAToI(vcParameter, 10);

  // Param : Periodic
  if (kGetNextParameter(&stList, vcParameter) == 0)
  {
    kPrintf("ex)settimer 10(ms) 1(periodic)\n");
    return;
  }
  bPeriodic = kAToI(vcParameter, 10);

  // set PIT 
  kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
  kPrintf("Time = %d ms, Periodic = %d Change Complete\n", lValue, bPeriodic);
}

/*
  wait MS using PIT
*/
void kWaitUsingPIT(const char* pcParameterBuffer)
{
  char vcParameter[100];
  int iLength;
  PARAMETERLIST stList;
  long lMillisecond;
  int i;

  // Init Param
  kInitializeParameter(&stList, pcParameterBuffer);
  if (kGetNextParameter(&stList, vcParameter) == 0)
  {
    kPrintf("ex)wait 100(ms)\n");
    return;
  }

  lMillisecond = kAToI(pcParameterBuffer, 10);
  kPrintf("%d ms Sleep Start...\n", lMillisecond);

  // Disable Interrupt & Wait
  kDisableInterrupt();
  for (i = 0; i < lMillisecond / 30; i++)
  {
    kWaitUsingDirectPIT(MSTOCOUNT(30));
  }
  kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));

  // Enable Interrupt
  kEnableInterrupt();
  kPrintf("%d ms Sleep Complete\n", lMillisecond);

  // reset PIC default setting
  kInitializePIT(MSTOCOUNT(1), TRUE);
}

/*
  Read TimeStampCounter
*/
void kReadTimeStampCounter(const char* pcParameterBuffer)
{
  QWORD qwTSC;

  qwTSC = kReadTSC();
  kPrintf("Time Stamp Counter = %q\n", qwTSC);
}

/*
  Check Processor speed
*/
void kMeasureProcessorSpeed(const char* pcParameterBuffer)
{
  int i;
  QWORD qwLastTSC, qwTotalTSC = 0;

  kPrintf("Now Measuring.");

  // record total counter during 10 Sec
  kDisableInterrupt();
  for (i = 0; i < 200; i++)
  {
    qwLastTSC = kReadTSC();
    kWaitUsingDirectPIT(MSTOCOUNT(50));
    qwTotalTSC += kReadTSC() - qwLastTSC;

    kPrintf(".");
  }

  // reset PIC default setting
  kInitializePIT(MSTOCOUNT(1), TRUE);
  kEnableInterrupt();

  kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}

/*
  print RTC Data & Time
*/
void kShowDateAndTime(const char* pcParameterBuffer)
{
  BYTE bSecond, bMinute, bHour;
  BYTE bDayOfWeek, bDayOfMonth, bMonth;
  WORD wYear;

  // read RTC
  kReadRTCTime(&bHour, &bMinute, &bSecond);
  kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

  kPrintf("Date: %d/%d/%d %s, ", wYear, bMonth, bDayOfMonth,
    kConvertDayOfWeekToString(bDayOfWeek));
  kPrintf("Time: %d:%d:%d\n", bHour, bMinute, bSecond);
}

void kTestTask(void)
{
  int i = 0;
  while (1) {
    kPrintf("[%d] This message is from kTestTask. Press any key to switch\n", i++);
    kGetCh();

    kSwitchContext(&gs_vstTask[1].stContext, &gs_vstTask[0].stContext);
  }
}

void kCreateTestTask(const char* pcParameterBuffer)
{
  KEYDATA stData;
  int i = 0;

  kSetUpTask(&(gs_vstTask[1]), 1, 0, (QWORD)kTestTask, &(gs_vstStack), sizeof(gs_vstStack));

  while (1) {
    kPrintf("[%d] This message is from kCreateTestTask. Press any key to switch\n", i++);
    if (kGetCh() == 'q')
      break;
    kSwitchContext(&gs_vstTask[0].stContext, &gs_vstTask[1].stContext);
  }
}