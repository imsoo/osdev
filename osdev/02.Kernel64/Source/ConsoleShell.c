#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "PIC.h"
#include "AssemblyUtility.h"
#include "Task.h"
#include "Synchronization.h"

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
  { "createtask", "Create Task ex)createtask 1(type) 10(count)", kCreateTestTask },
  { "changepriority", "Change Task Priority, ex)changepriority 1(ID) 2(Priorty)", kChangeTaskPriority },
  { "tasklist", "Show Task List", kShowTaskList },
  { "killtask", "End Task, ex)killtask 1(ID) or 0xffffffff(All Task)", kKillTask },
  { "cpuload", "Show processor Load", kCPULoad },
  { "testmutex", "Test Mutex Function", kTestMutex },
};

// TCB
static TCB gs_vstTask[2] = { 0, };
static QWORD gs_vstStack[1024] = { 0, };

// Mutex
static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

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
static void kHelp(const char* pcCommandBuffer)
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
static void kCls(const char* pcParameterBuffer)
{
  kClearScreen();

  // first line use for debug
  kSetCursor(0, 1);
}

/*
  Print RAM Size
*/
static void kShowTotalRAMSize(const char* pcParameterBuffer)
{
  kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

/*
  strtol and print
*/
static void kStringToDecimalHexTest(const char* pcParameterBuffer)
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
static void kShutDown(const char* pcParamegerBuffer)
{
  kPrintf("System Shutdown Start...\n");

  kPrintf("Press Any Key To Reboot PC...");
  kGetCh();
  kReboot();
}

/*
  set PIT counter
*/
static void kSetTimer(const char* pcParameterBuffer)
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
    kPrintf("ex)settimer 10(ms) 0(periodic)\n");
    return;
  }
  lValue = kAToI(vcParameter, 10);

  // Param : Periodic
  if (kGetNextParameter(&stList, vcParameter) == 0)
  {
    kPrintf("ex)settimer 10(ms) 0(periodic)\n");
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
static void kWaitUsingPIT(const char* pcParameterBuffer)
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
static void kReadTimeStampCounter(const char* pcParameterBuffer)
{
  QWORD qwTSC;

  qwTSC = kReadTSC();
  kPrintf("Time Stamp Counter = %q\n", qwTSC);
}

/*
  Check Processor speed
*/
static void kMeasureProcessorSpeed(const char* pcParameterBuffer)
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
static void kShowDateAndTime(const char* pcParameterBuffer)
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

static void kTestTask1(void)
{
  BYTE bData;
  int i = 0, iX = 0, iY = 0, iMargin, j;
  CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
  TCB* pstRunningTask;

  pstRunningTask = kGetRunningTask();
  iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

  for (j = 0; j < 20000; j++)
  {
    switch (i)
    {
    case 0:
      iX++;
      if (iX >= (CONSOLE_WIDTH - iMargin))
      {
        i = 1;
      }
      break;

    case 1:
      iY++;
      if (iY >= (CONSOLE_HEIGHT - iMargin))
      {
        i = 2;
      }
      break;

    case 2:
      iX--;
      if (iX < iMargin)
      {
        i = 3;
      }
      break;

    case 3:
      iY--;
      if (iY < iMargin)
      {
        i = 0;
      }
      break;
    }

    pstScreen[iY * CONSOLE_WIDTH + iX].bCharactor = bData;
    pstScreen[iY * CONSOLE_WIDTH + iX].bAttribute = bData & 0x0F;
    bData++;

    // kSchedule();
  }
  kExitTask();
}

static void kTestTask2(void)
{
  int i = 0, iOffset;
  CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
  TCB* pstRunningTask;
  char vcData[4] = { '-', '\\', '|', '/' };

  pstRunningTask = kGetRunningTask();
  iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
  iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT -
    (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

  while (1)
  {
    pstScreen[iOffset].bCharactor = vcData[i % 4];
    pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
    i++;

    // kSchedule();
  }
}

void kCreateTestTask(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcType[30];
  char vcCount[30];
  int i;

  kInitializeParameter(&stList, pcParameterBuffer);
  kGetNextParameter(&stList, vcType);
  kGetNextParameter(&stList, vcCount);

  switch (kAToI(vcType, 10))
  {
  case 1:
    for (i = 0; i < kAToI(vcCount, 10); i++)
    {
      if (kCreateTask(TASK_FLAGS_LOW, (QWORD)kTestTask1) == NULL)
      {
        break;
      }
    }

    kPrintf("Task1 %d Created\n", i);
    break;

  case 2:
  default:
    for (i = 0; i < kAToI(vcCount, 10); i++)
    {
      if (kCreateTask(TASK_FLAGS_LOW, (QWORD)kTestTask2) == NULL)
      {
        break;
      }
    }

    kPrintf("Task2 %d Created\n", i);
    break;
  }
}

/*
  change task prior
*/
static void kChangeTaskPriority(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcID[30];
  char vcPriority[30];
  QWORD qwID;
  BYTE bPriority;

  // param
  kInitializeParameter(&stList, pcParameterBuffer);
  kGetNextParameter(&stList, vcID);
  kGetNextParameter(&stList, vcPriority);

  // change prior
  if (kMemCmp(vcID, "0x", 2) == 0)
  {
    qwID = kAToI(vcID + 2, 16);
  }
  else
  {
    qwID = kAToI(vcID, 10);
  }

  bPriority = kAToI(vcPriority, 10);

  kPrintf("Change Task Priority ID [0x%q] Priority[%d] ", qwID, bPriority);
  if (kChangePriority(qwID, bPriority) == TRUE)
  {
    kPrintf("Success\n");
  }
  else
  {
    kPrintf("Fail\n");
  }
}

/*
  Show all Task 
*/
static void kShowTaskList(const char* pcParameterBuffer)
{
  int i;
  TCB* pstTCB;
  int iCount = 0;

  kPrintf("=========== Task Total Count [%d] ===========\n", kGetTaskCount());
  for (i = 0; i < TASK_MAXCOUNT; i++)
  {
    // Check TCB is using
    pstTCB = kGetTCBInTCBPool(i);
    if ((pstTCB->stLink.qwID >> 32) != 0)
    {
      // print more
      if ((iCount != 0) && ((iCount % 10) == 0))
      {
        kPrintf("Press any key to continue... ('q' is exit) : ");
        if (kGetCh() == 'q')
        {
          kPrintf("\n");
          break;
        }
        kPrintf("\n");
      }

      kPrintf("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q]\n", 1 + iCount++,
        pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags),
        pstTCB->qwFlags);
    }
  }
}

/*
  Kill task using Task ID
*/
static void kKillTask(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcID[30];
  QWORD qwID;
  TCB* pstTCB;
  int i;

  // param
  kInitializeParameter(&stList, pcParameterBuffer);
  kGetNextParameter(&stList, vcID);

  if (kMemCmp(vcID, "0x", 2) == 0)
  {
    qwID = kAToI(vcID + 2, 16);
  }
  else
  {
    qwID = kAToI(vcID, 10);
  }

  // kill task using id
  if (qwID != 0xFFFFFFFF) {
    kPrintf("Kill Task ID [0x%q] ", qwID);
    if (kEndTask(qwID) == TRUE)
    {
      kPrintf("Success\n");
    }
    else
    {
      kPrintf("Fail\n");
    }
  }
  // kill all tesk
  else {
    for (i = 2; i < TASK_MAXCOUNT; i++) {
      pstTCB = kGetTCBInTCBPool(i);
      qwID = pstTCB->stLink.qwID;
      if ((qwID >> 32) != 0) {
        kPrintf("Kill Task ID [0x%q] ", qwID);
        if (kEndTask(qwID) == TRUE)
        {
          kPrintf("Success\n");
        }
        else
        {
          kPrintf("Fail\n");
        }
      }
    }
  }
}

/*
  Print Processor Usage
*/
static void kCPULoad(const char* pcParameterBuffer)
{
  kPrintf("Processor Load : %d%%\n", kGetProcessorLoad());
}


static void kPrintNumberTask(void)
{
  int i;
  int j;
  QWORD qwTickCount;

  qwTickCount = kGetTickCount();
  while ((kGetTickCount() - qwTickCount) < 50) {
    kSchedule();
  }

  for (i = 0; i < 5; i++) {
    kLock(&(gs_stMutex));
    kPrintf("Task ID [0x%Q] Value[%d]\n", kGetRunningTask()->stLink.qwID, gs_qwAdder);
    gs_qwAdder += 1;
    kUnlock(&(gs_stMutex));

    for (j = 0; j < 30000; j++);
  }

  qwTickCount = kGetTickCount();
  while ((kGetTickCount() - qwTickCount) < 1000) {
    kSchedule();
  }

  kExitTask();
}

static void kTestMutex(const char* pcParameterBuffer)
{
  int i;
  gs_qwAdder = 1;

  kInitializeMutex(&gs_stMutex);

  for (i = 0; i < 3; i++) {
    kCreateTask(TASK_FLAGS_LOW, (QWORD)kPrintNumberTask);
  }
  kPrintf("Wait Until %d Task End...\n", i);
  kGetCh();
}