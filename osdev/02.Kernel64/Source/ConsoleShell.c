#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "PIC.h"
#include "AssemblyUtility.h"
#include "Task.h"
#include "Synchronization.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"
#include "MPConfigurationTable.h"
#include "LocalAPIC.h"
#include "MultiProcessor.h"
#include "InterruptHandler.h"
#include "VBE.h"
#include "SystemCall.h"
#include "Loader.h"
#include "Ethernet.h"
#include "IP.h"
#include "ARP.h"
#include "ICMP.h"
#include "TCP.h"
#include "UDP.h"
#include "DHCP.h"
#include "DNS.h"
#include "Telnet.h"

// Command Table
SHELLCOMMANDENTRY gs_vstCommandTable[] =
{
  { "help", "Show Help", kHelp },
  { "cls", "Clear Screen", kCls },
  { "totalram", "Show Total RAM Size", kShowTotalRAMSize },
  { "shutdown", "Shutdown And Reboot OS", kShutDown },

  // PIT, RCT
  { "cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed },
  { "date", "Show Date And Time", kShowDateAndTime },

  // Task
  { "changepriority", "Change Task Priority, ex)changepriority 1(ID) 2(Priorty)", kChangeTaskPriority },
  { "tasklist", "Show Task List", kShowTaskList },
  { "killtask", "End Task, ex)killtask 1(ID) or 0xffffffff(All Task)", kKillTask },
  { "cpuload", "Show processor Load", kCPULoad },

  // Thread
  { "showmatrix", "Show Matrix Screen", kShowMatrix },

  // Hard
  { "dynamicmeminfo", "Show Dyanmic Memory Information", kShowDyanmicMemoryInformation },
  { "hddinfo", "Show HDD Information", kShowHDDInformation },
  { "readsector", "Read HDD Sector, ex)readsector 0(LBA) 10(count)", kReadSector },
  { "writesector", "Write HDD Sector, ex)writesector 0(LBA) 10(count)", kWriteSector },

  // FileSystem
  { "mounthdd", "Mount HDD", kMountHDD },
  { "formathdd", "Format HDD", kFormatHDD },
  { "filesysteminfo", "Show File System Information", kShowFileSystemInformation },
  { "createfile", "Create File, ex)createfile a.txt", kCreateFileInRootDirectory },
  { "deletefile", "Delete File, ex)deletefile a.txt", kDeleteFileInRootDirectory },
  { "dir", "Show Directory", kShowRootDirectory },
  { "writefile", "Write Data To File, ex) writefile a.txt", kWriteDataToFile },
  { "readfile", "Read Data From File, ex) readfile a.txt", kReadDataFromFile },

  // Cache
  { "flush", "Flush File System Cache", kFlushCache },

  // Serial Port
  { "download", "Download Data From Serial, ex) download a.txt", kDownloadFile },

  // MP
  { "showmpinfo", "Show MP Configuration Table Information", kShowMPConfigurationTable },
  { "showirqintinmap", "Show IRQ->INITIN Mapping Table", kShowIRQINTINMappingTable },
  { "showintproccount", "Show Interrupt Processing Count", kShowInterruptProcessingCount },
  { "changeaffinity", "Change Task Affinity, ex)changeaffinity 1(ID) 0xFF(Affinity)", kChangeTaskAffinity },

  // GUI
  { "vbemodeinfo", "Show VBE Mode Information", kShowVBEModeInfo },

  // Loader
  { "exec", "Execute Application Program, ex)exec a.elf argument", kExecuteApplicationProgram },

  // Network
  { "initn", "InitNetwork" , kInitNetwork },
  { "telnet", "telnet <ip> <port>", kTelnetC },
  { "dns", "Print DNS" , kShowDNSState },
  { "arp", "Print ARP" , kShowARPState },
  { "ipconfig", "Print IP Configuration", kShowDHCPState },
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
  CONSOLEMANAGER* pstConsoleManager;

  pstConsoleManager = kGetConsoleManager();

  // print PROMPT
  kPrintf(CONSOLESHELL_PROMPTMESSAGE);

  while (pstConsoleManager->bExit == FALSE)
  {
    // wait key
    bKey = kGetCh();

    if (pstConsoleManager->bExit == TRUE) {
      break;
    }

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
    else if (bKey < 128)
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

    if ((i != 0) && ((i % 20) == 0)) {
      kPrintf("Press any key to continue... ('q' is exit) : ");
      if (kGetCh() == 'q') {
        kPrintf("\n");
        break;
      }
      kPrintf("\n");
    }
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
  reboot 
*/
static void kShutDown(const char* pcParamegerBuffer)
{
  kPrintf("System Shutdown Start...\n");

  kPrintf("Cache Flush... ");
  if (kFlushFileSystemCache() == TRUE)
  {
    kPrintf("Pass\n");
  }
  else
  {
    kPrintf("Fail\n");
  }

  kPrintf("Press Any Key To Reboot PC...");
  kGetCh();
  kReboot();
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
  int iTotalTaskCount = 0;
  char vcBuffer[20];
  int iRemainLength;
  int iProcessorCount;

  iProcessorCount = kGetProcessorCount();

  for (i = 0; i < iProcessorCount; i++)
  {
    iTotalTaskCount += kGetTaskCount(i);
  }

  kPrintf("================= Task Total Count [%d] =================\n",
    iTotalTaskCount);

  if (iProcessorCount > 1)
  {
    for (i = 0; i < iProcessorCount; i++)
    {
      if ((i != 0) && ((i % 4) == 0))
      {
        kPrintf("\n");
      }

      kSPrintf(vcBuffer, "Core %d : %d", i, kGetTaskCount(i));
      kPrintf(vcBuffer);

      iRemainLength = 19 - kStrLen(vcBuffer);
      kMemSet(vcBuffer, ' ', iRemainLength);
      vcBuffer[iRemainLength] = '\0';
      kPrintf(vcBuffer);
    }

    kPrintf("\nPress any key to continue... ('q' is exit) : ");
    if (kGetCh() == 'q')
    {
      kPrintf("\n");
      return;
    }
    kPrintf("\n\n");
  }

  for (i = 0; i < TASK_MAXCOUNT; i++)
  {
    pstTCB = kGetTCBInTCBPool(i);
    if ((pstTCB->stLink.qwID >> 32) != 0)
    {
      if ((iCount != 0) && ((iCount % 6) == 0))
      {
        kPrintf("Press any key to continue... ('q' is exit) : ");
        if (kGetCh() == 'q')
        {
          kPrintf("\n");
          break;
        }
        kPrintf("\n");
      }

      kPrintf("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n", 1 + iCount++,
        pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags),
        pstTCB->qwFlags, kGetListCount(&(pstTCB->stChildThreadList)));
      kPrintf("    Core ID[0x%X] CPU Affinity[0x%X]\n", pstTCB->bAPICID,
        pstTCB->bAffinity);
      kPrintf("    Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n",
        pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize);
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
    pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
    qwID = pstTCB->stLink.qwID;
    
    if (((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00)) {
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
    else {
      kPrintf("Task does not exist or task is system task\n");
    }
  }
  // kill all tesk
  else {
    for (i = 0; i < TASK_MAXCOUNT; i++) {
      pstTCB = kGetTCBInTCBPool(i);
      qwID = pstTCB->stLink.qwID;
      if (((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00)) {
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
  int i;
  char vcBuffer[50];
  int iRemainLength;

  kPrintf("================= Processor Load =================\n");

  for (i = 0; i < kGetProcessorCount(); i++)
  {
    if ((i != 0) && ((i % 4) == 0))
    {
      kPrintf("\n");
    }

    kSPrintf(vcBuffer, "Core %d : %d%%", i, kGetProcessorLoad(i));
    kPrintf("%s", vcBuffer);

    iRemainLength = 19 - kStrLen(vcBuffer);
    kMemSet(vcBuffer, ' ', iRemainLength);
    vcBuffer[iRemainLength] = '\0';
    kPrintf(vcBuffer);
  }
  kPrintf("\n");
}

static void kDropCharactorThread(void)
{
  int iX, iY;
  int i;
  char vcText[2] = { 0, };

  iX = kRandom() % CONSOLE_WIDTH;

  while (1)
  {
    kSleep(kRandom() % 20);

    if ((kRandom() % 20) < 16)
    {
      vcText[0] = ' ';
      for (i = 0; i < CONSOLE_HEIGHT - 1; i++)
      {
        kPrintStringXY(iX, i, vcText);
        kSleep(50);
      }
    }
    else
    {
      for (i = 0; i < CONSOLE_HEIGHT - 1; i++)
      {
        vcText[0] = (i + kRandom()) % 128;
        kPrintStringXY(iX, i, vcText);
        kSleep(50);
      }
    }
  }
}

static void kMatrixProcess(void)
{
  int i;

  for (i = 0; i < 300; i++)
  {
    if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0,
      (QWORD)kDropCharactorThread, TASK_LOADBALANCINGID) == NULL)
    {
      break;
    }

    kSleep(kRandom() % 5 + 5);
  }

  kPrintf("%d Thread is created\n", i);

  kGetCh();
}


static void kShowMatrix(const char* pcParameterBuffer)
{
  TCB* pstProcess;

  pstProcess = kCreateTask(TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, (void*)0xE00000, 0xE00000,
    (QWORD)kMatrixProcess, TASK_LOADBALANCINGID);
  if (pstProcess != NULL)
  {
    kPrintf("Matrix Process [0x%Q] Create Success\n");

    while ((pstProcess->stLink.qwID >> 32) != 0)
    {
      kSleep(100);
    }
  }
  else
  {
    kPrintf("Matrix Process Create Fail\n");
  }
}

/*
  Print Dynamic Memory info
*/
static void kShowDyanmicMemoryInformation(const char* pcParameterBuffer)
{
  QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;

  kGetDynamicMemoryInformation(&qwStartAddress, &qwTotalSize, &qwMetaSize,
    &qwUsedSize);

  kPrintf("============ Dynamic Memory Information ============\n");
  kPrintf("Start Address: [0x%Q]\n", qwStartAddress);
  kPrintf("Total Size:    [0x%Q]byte, [%d]MB\n", qwTotalSize,
    qwTotalSize / 1024 / 1024);
  kPrintf("Meta Size:     [0x%Q]byte, [%d]KB\n", qwMetaSize,
    qwMetaSize / 1024);
  kPrintf("Used Size:     [0x%Q]byte, [%d]KB\n", qwUsedSize, qwUsedSize / 1024);
}

/*
  Print HDD Infomation
*/
static void kShowHDDInformation(const char* pcParameterBuffer)
{
  HDDINFORMATION stHDD;
  char vcBuffer[100];

  if (kReadHDDInformation(TRUE, TRUE, &stHDD) == FALSE)
  {
    kPrintf("HDD Information Read Fail\n");
    return;
  }

  kPrintf("============ Primary Master HDD Information ============\n");

  kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
  vcBuffer[sizeof(stHDD.vwModelNumber) - 1] = '\0';
  kPrintf("Model Number:\t %s\n", vcBuffer);

  kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
  vcBuffer[sizeof(stHDD.vwSerialNumber) - 1] = '\0';
  kPrintf("Serial Number:\t %s\n", vcBuffer);

  kPrintf("Head Count:\t %d\n", stHDD.wNumberOfHead);
  kPrintf("Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder);
  kPrintf("Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder);

  kPrintf("Total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors,
    stHDD.dwTotalSectors / 2 / 1024);
}

/*
  Read HDD Sector Test
*/
static void kReadSector(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcLBA[50], vcSectorCount[50];
  DWORD dwLBA;
  int iSectorCount;
  char* pcBuffer;
  int i, j;
  BYTE bData;
  BOOL bExit = FALSE;

  kInitializeParameter(&stList, pcParameterBuffer);
  if ((kGetNextParameter(&stList, vcLBA) == 0) ||
    (kGetNextParameter(&stList, vcSectorCount) == 0))
  {
    kPrintf("ex) readsector 0(LBA) 10(count)\n");
    return;
  }
  dwLBA = kAToI(vcLBA, 10);
  iSectorCount = kAToI(vcSectorCount, 10);

  pcBuffer = kAllocateMemory(iSectorCount * 512);
  if (kReadHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) == iSectorCount)
  {
    kPrintf("LBA [%d], [%d] Sector Read Success~!!", dwLBA, iSectorCount);
    for (j = 0; j < iSectorCount; j++)
    {
      for (i = 0; i < 512; i++)
      {
        if (!((j == 0) && (i == 0)) && ((i % 256) == 0))
        {
          kPrintf("\nPress any key to continue... ('q' is exit) : ");
          if (kGetCh() == 'q')
          {
            bExit = TRUE;
            break;
          }
        }

        if ((i % 16) == 0)
        {
          kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i);
        }

        bData = pcBuffer[j * 512 + i] & 0xFF;
        if (bData < 16)
        {
          kPrintf("0");
        }
        kPrintf("%X ", bData);
      }

      if (bExit == TRUE)
      {
        break;
      }
    }
    kPrintf("\n");
  }
  else
  {
    kPrintf("Read Fail\n");
  }

  kFreeMemory(pcBuffer);
}

/*
  Write HDD Sector Test
*/
static void kWriteSector(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcLBA[50], vcSectorCount[50];
  DWORD dwLBA;
  int iSectorCount;
  char* pcBuffer;
  int i, j;
  BOOL bExit = FALSE;
  BYTE bData;
  static DWORD s_dwWriteCount = 0;

  kInitializeParameter(&stList, pcParameterBuffer);
  if ((kGetNextParameter(&stList, vcLBA) == 0) ||
    (kGetNextParameter(&stList, vcSectorCount) == 0))
  {
    kPrintf("ex) writesector 0(LBA) 10(count)\n");
    return;
  }
  dwLBA = kAToI(vcLBA, 10);
  iSectorCount = kAToI(vcSectorCount, 10);

  s_dwWriteCount++;

  pcBuffer = kAllocateMemory(iSectorCount * 512);
  for (j = 0; j < iSectorCount; j++)
  {
    for (i = 0; i < 512; i += 8)
    {
      *(DWORD*) &(pcBuffer[j * 512 + i]) = dwLBA + j;
      *(DWORD*) &(pcBuffer[j * 512 + i + 4]) = s_dwWriteCount;
    }
  }

  if (kWriteHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) != iSectorCount)
  {
    kPrintf("Write Fail\n");
    return;
  }
  kPrintf("LBA [%d], [%d] Sector Write Success~!!", dwLBA, iSectorCount);

  for (j = 0; j < iSectorCount; j++)
  {
    for (i = 0; i < 512; i++)
    {
      if (!((j == 0) && (i == 0)) && ((i % 256) == 0))
      {
        kPrintf("\nPress any key to continue... ('q' is exit) : ");
        if (kGetCh() == 'q')
        {
          bExit = TRUE;
          break;
        }
      }

      if ((i % 16) == 0)
      {
        kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i);
      }

      bData = pcBuffer[j * 512 + i] & 0xFF;
      if (bData < 16)
      {
        kPrintf("0");
      }
      kPrintf("%X ", bData);
    }

    if (bExit == TRUE)
    {
      break;
    }
  }
  kPrintf("\n");
  kFreeMemory(pcBuffer);
}

/*
  Mount HDD
*/
static void kMountHDD(const char* pcParameterBuffer)
{
  if (kMount() == FALSE)
  {
    kPrintf("HDD Mount Fail\n");
    return;
  }
  kPrintf("HDD Mount Success\n");
}

/*
  Format HDD
*/
static void kFormatHDD(const char* pcParameterBuffer)
{
  if (kFormat() == FALSE)
  {
    kPrintf("HDD Format Fail\n");
    return;
  }
  kPrintf("HDD Format Success\n");
}

/*
  Print FileSystem information
*/
static void kShowFileSystemInformation(const char* pcParameterBuffer)
{
  FILESYSTEMMANAGER stManager;

  kGetFileSystemInformation(&stManager);

  kPrintf("================== File System Information ==================\n");
  kPrintf("Mouted:\t\t\t\t\t %d\n", stManager.bMounted);
  kPrintf("Reserved Sector Count:\t\t\t %d Sector\n", stManager.dwReservedSectorCount);
  kPrintf("Cluster Link Table Start Address:\t %d Sector\n",
    stManager.dwClusterLinkAreaStartAddress);
  kPrintf("Cluster Link Table Size:\t\t %d Sector\n", stManager.dwClusterLinkAreaSize);
  kPrintf("Data Area Start Address:\t\t %d Sector\n", stManager.dwDataAreaStartAddress);
  kPrintf("Total Cluster Count:\t\t\t %d Cluster\n", stManager.dwTotalClusterCount);
}

/*
  Create File in Root Dir for test
*/
static void kCreateFileInRootDirectory(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcFileName[50];
  int iLength;
  DWORD dwCluster;
  int i;
  FILE* pstFile;

  kInitializeParameter(&stList, pcParameterBuffer);
  iLength = kGetNextParameter(&stList, vcFileName);
  vcFileName[iLength] = '\0';
  if ((iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0))
  {
    kPrintf("Too Long or Too Short File Name\n");
    return;
  }

  pstFile = fopen(vcFileName, "w");
  if (pstFile == NULL)
  {
    kPrintf("File Create Fail\n");
    return;
  }
  fclose(pstFile);
  kPrintf("File Create Success\n");
}

/*
  Delete File in Root Dir for test
*/
static void kDeleteFileInRootDirectory(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcFileName[50];
  int iLength;

  kInitializeParameter(&stList, pcParameterBuffer);
  iLength = kGetNextParameter(&stList, vcFileName);
  vcFileName[iLength] = '\0';
  if ((iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0))
  {
    kPrintf("Too Long or Too Short File Name\n");
    return;
  }

  if (remove(vcFileName) != 0)
  {
    kPrintf("File Not Found or File Opened\n");
    return;
  }

  kPrintf("File Delete Success\n");
}

/*
  Show FileSystem root Directory
*/
static void kShowRootDirectory(const char* pcParameterBuffer)
{
  DIR* pstDirectory;
  int i, iCount, iTotalCount;
  struct dirent* pstEntry;
  char vcBuffer[400];
  char vcTempValue[50];
  DWORD dwTotalByte;
  DWORD dwUsedClusterCount;
  FILESYSTEMMANAGER stManager;

  kGetFileSystemInformation(&stManager);

  pstDirectory = opendir("/");
  if (pstDirectory == NULL)
  {
    kPrintf("Root Directory Open Fail\n");
    return;
  }

  iTotalCount = 0;
  dwTotalByte = 0;
  dwUsedClusterCount = 0;
  while (1)
  {
    pstEntry = readdir(pstDirectory);
    if (pstEntry == NULL)
    {
      break;
    }
    iTotalCount++;
    dwTotalByte += pstEntry->dwFileSize;

    if (pstEntry->dwFileSize == 0)
    {
      dwUsedClusterCount++;
    }
    else
    {
      dwUsedClusterCount += (pstEntry->dwFileSize +
        (FILESYSTEM_CLUSTERSIZE - 1)) / FILESYSTEM_CLUSTERSIZE;
    }
  }

  rewinddir(pstDirectory);
  iCount = 0;
  while (1)
  {
    pstEntry = readdir(pstDirectory);
    if (pstEntry == NULL)
    {
      break;
    }

    kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);
    vcBuffer[sizeof(vcBuffer) - 1] = '\0';

    kMemCpy(vcBuffer, pstEntry->d_name,
      kStrLen(pstEntry->d_name));

    kSPrintf(vcTempValue, "%d Byte", pstEntry->dwFileSize);
    kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));

    kSPrintf(vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex);
    kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue) + 1);
    kPrintf("    %s\n", vcBuffer);

    if ((iCount != 0) && ((iCount % 20) == 0))
    {
      kPrintf("Press any key to continue... ('q' is exit) : ");
      if (kGetCh() == 'q')
      {
        kPrintf("\n");
        break;
      }
    }
    iCount++;
  }

  kPrintf("\t\tTotal File Count: %d\n", iTotalCount);
  kPrintf("\t\tTotal File Size: %d KByte (%d Cluster)\n", dwTotalByte,
    dwUsedClusterCount);

  kPrintf("\t\tFree Space: %d KByte (%d Cluster)\n",
    (stManager.dwTotalClusterCount - dwUsedClusterCount) *
    FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount -
    dwUsedClusterCount);

  closedir(pstDirectory);
}

/*
  Write to HDD Test
*/
static void kWriteDataToFile(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcFileName[50];
  int iLength;
  FILE* fp;
  int iEnterCount;
  BYTE bKey;

  kInitializeParameter(&stList, pcParameterBuffer);
  iLength = kGetNextParameter(&stList, vcFileName);
  vcFileName[iLength] = '\0';
  if ((iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0))
  {
    kPrintf("Too Long or Too Short File Name\n");
    return;
  }

  fp = fopen(vcFileName, "w");
  if (fp == NULL)
  {
    kPrintf("%s File Open Fail\n", vcFileName);
    return;
  }

  iEnterCount = 0;
  while (1)
  {
    bKey = kGetCh();
    if (bKey == KEY_ENTER)
    {
      iEnterCount++;
      if (iEnterCount >= 3)
      {
        break;
      }
    }
    else
    {
      iEnterCount = 0;
    }

    kPrintf("%c", bKey);
    if (fwrite(&bKey, 1, 1, fp) != 1)
    {
      kPrintf("File Wirte Fail\n");
      break;
    }
  }

  kPrintf("File Create Success\n");
  fclose(fp);
}

/*
  Read from HDD Test 
*/
static void kReadDataFromFile(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcFileName[50];
  int iLength;
  FILE* fp;
  int iEnterCount;
  BYTE bKey;

  kInitializeParameter(&stList, pcParameterBuffer);
  iLength = kGetNextParameter(&stList, vcFileName);
  vcFileName[iLength] = '\0';
  if ((iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0))
  {
    kPrintf("Too Long or Too Short File Name\n");
    return;
  }

  fp = fopen(vcFileName, "r");
  if (fp == NULL)
  {
    kPrintf("%s File Open Fail\n", vcFileName);
    return;
  }

  iEnterCount = 0;
  while (1)
  {
    if (fread(&bKey, 1, 1, fp) != 1)
    {
      break;
    }
    kPrintf("%c", bKey);

    if (bKey == KEY_ENTER)
    {
      iEnterCount++;

      if ((iEnterCount != 0) && ((iEnterCount % 20) == 0))
      {
        kPrintf("Press any key to continue... ('q' is exit) : ");
        if (kGetCh() == 'q')
        {
          kPrintf("\n");
          break;
        }
        kPrintf("\n");
        iEnterCount = 0;
      }
    }
  }
  fclose(fp);
}

/*
  Flush Cache to Drive
*/
static void kFlushCache(const char* pcParameterBuffer)
{
  QWORD qwTickCount;

  qwTickCount = kGetTickCount();
  kPrintf("Cache Flush... ");
  if (kFlushFileSystemCache() == TRUE)
  {
    kPrintf("Pass\n");
  }
  else
  {
    kPrintf("Fail\n");
  }
  kPrintf("Total Time = %d ms\n", kGetTickCount() - qwTickCount);
}

/*
  DownLoad file using Rs232 Serial Controller
*/
static void kDownloadFile(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcFileName[50];
  int iFileNameLength;
  DWORD dwDataLength;
  FILE* fp;
  DWORD dwReceivedSize;
  DWORD dwTempSize;
  BYTE vbDataBuffer[SERIAL_FIFOMAXSIZE];
  QWORD qwLastReceivedTickCount;

  kInitializeParameter(&stList, pcParameterBuffer);
  iFileNameLength = kGetNextParameter(&stList, vcFileName);
  vcFileName[iFileNameLength] = '\0';
  if ((iFileNameLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) ||
    (iFileNameLength == 0))
  {
    kPrintf("Too Long or Too Short File Name\n");
    kPrintf("ex)download a.txt\n");
    return;
  }

  kClearSerialFIFO();

  kPrintf("Waiting For Data Length.....");
  dwReceivedSize = 0;
  qwLastReceivedTickCount = kGetTickCount();
  while (dwReceivedSize < 4)
  {
    dwTempSize = kReceiveSerialData(((BYTE*)&dwDataLength) +
      dwReceivedSize, 4 - dwReceivedSize);
    dwReceivedSize += dwTempSize;

    if (dwTempSize == 0)
    {
      kSleep(0);

      if ((kGetTickCount() - qwLastReceivedTickCount) > 30000)
      {
        kPrintf("Time Out Occur~!!\n");
        return;
      }
    }
    else
    {
      qwLastReceivedTickCount = kGetTickCount();
    }
  }
  kPrintf("[%d] Byte\n", dwDataLength);

  kSendSerialData("A", 1);

  fp = fopen(vcFileName, "w");
  if (fp == NULL)
  {
    kPrintf("%s File Open Fail\n", vcFileName);
    return;
  }

  kPrintf("Data Receive Start: ");
  dwReceivedSize = 0;
  qwLastReceivedTickCount = kGetTickCount();
  while (dwReceivedSize < dwDataLength)
  {
    dwTempSize = kReceiveSerialData(vbDataBuffer, SERIAL_FIFOMAXSIZE);
    dwReceivedSize += dwTempSize;

    if (dwTempSize != 0)
    {
      if (((dwReceivedSize % SERIAL_FIFOMAXSIZE) == 0) ||
        ((dwReceivedSize == dwDataLength)))
      {
        kSendSerialData("A", 1);
        kPrintf("#");
      }

      if (fwrite(vbDataBuffer, 1, dwTempSize, fp) != dwTempSize)
      {
        kPrintf("File Write Error Occur\n");
        break;
      }

      qwLastReceivedTickCount = kGetTickCount();
    }
    else
    {
      kSleep(0);

      if ((kGetTickCount() - qwLastReceivedTickCount) > 10000)
      {
        kPrintf("Time Out Occur~!!\n");
        break;
      }
    }
  }

  if (dwReceivedSize != dwDataLength)
  {
    kPrintf("\nError Occur. Total Size [%d] Received Size [%d]\n",
      dwReceivedSize, dwDataLength);
  }
  else
  {
    kPrintf("\nReceive Complete. Total Size [%d] Byte\n", dwReceivedSize);
  }

  fclose(fp);
  kFlushFileSystemCache();
}

/*
  Print MP Configuration Table
*/
static void kShowMPConfigurationTable(const char* pcParameterBuffer)
{
  kPrintMPConfigurationTable();
}

/*
  Print IRQ <-> I/O APIC INTIN Map Table
*/
static void kShowIRQINTINMappingTable(const char* pcParameterBuffer)
{
  kPrintIRQToINTINMap();
}

/*
  Show All Core's Interrupt Processing Count
*/
static void kShowInterruptProcessingCount(const char* pcParameterBuffer)
{
  INTERRUPTMANAGER* pstInterruptManager;
  int i;
  int j;
  int iProcessCount;
  char vcBuffer[20];
  int iRemainLength;
  int iLineCount;

  kPrintf("========================== Interrupt Count ==========================\n");

  // Read Core Count
  iProcessCount = kGetProcessorCount();

  // Print Column
  for (i = 0; i < iProcessCount; i++)
  {
    if (i == 0)
    {
      kPrintf("IRQ Num\t\t");
    }
    else if ((i % 4) == 0)
    {
      kPrintf("\n       \t\t");
    }
    kSPrintf(vcBuffer, "Core %d", i);
    kPrintf(vcBuffer);

    // Remain fill space
    iRemainLength = 15 - kStrLen(vcBuffer);
    kMemSet(vcBuffer, ' ', iRemainLength);
    vcBuffer[iRemainLength] = '\0';
    kPrintf(vcBuffer);
  }
  kPrintf("\n");

  // Row : Interrupt Processing Count
  iLineCount = 0;
  pstInterruptManager = kGetInterruptManager();
  for (i = 0; i < INTERRUPT_MAXVECTORCOUNT; i++)
  {
    for (j = 0; j < iProcessCount; j++)
    {
      if (j == 0)
      {
        // more
        if ((iLineCount != 0) && (iLineCount > 10))
        {
          kPrintf("\nPress any key to continue... ('q' is exit) : ");
          if (kGetCh() == 'q')
          {
            kPrintf("\n");
            return;
          }
          iLineCount = 0;
          kPrintf("\n");
        }
        kPrintf("---------------------------------------------------------------------\n");
        kPrintf("IRQ %d\t\t", i);
        iLineCount += 2;
      }
      else if ((j % 4) == 0)
      {
        kPrintf("\n      \t\t");
        iLineCount++;
      }

      kSPrintf(vcBuffer, "0x%Q", pstInterruptManager->vvqwCoreInterruptCount[j][i]);

      // Remain fill space
      kPrintf(vcBuffer);
      iRemainLength = 15 - kStrLen(vcBuffer);
      kMemSet(vcBuffer, ' ', iRemainLength);
      vcBuffer[iRemainLength] = '\0';
      kPrintf(vcBuffer);
    }
    kPrintf("\n");
  }
}

/*
  Change Task affinity
*/
static void kChangeTaskAffinity(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcID[30];
  char vcAffinity[30];
  QWORD qwID;
  BYTE bAffinity;

  kInitializeParameter(&stList, pcParameterBuffer);
  kGetNextParameter(&stList, vcID);
  kGetNextParameter(&stList, vcAffinity);

  if (kMemCmp(vcID, "0x", 2) == 0)
  {
    qwID = kAToI(vcID + 2, 16);
  }
  else
  {
    qwID = kAToI(vcID, 10);
  }

  if (kMemCmp(vcID, "0x", 2) == 0)
  {
    bAffinity = kAToI(vcAffinity + 2, 16);
  }
  else
  {
    bAffinity = kAToI(vcAffinity, 10);
  }

  kPrintf("Change Task Affinity ID [0x%q] Affinity[0x%x] ", qwID, bAffinity);
  if (kChangeProcessorAffinity(qwID, bAffinity) == TRUE)
  {
    kPrintf("Success\n");
  }
  else
  {
    kPrintf("Fail\n");
  }
}


/*
  Print VBE Info Block
*/
static void kShowVBEModeInfo(const char* pcParameterBuffer)
{
  VBEMODEINFOBLOCK* pstModeInfo;

  pstModeInfo = kGetVBEModeInfoBlock();

  kPrintf("VESA %x\n", pstModeInfo->wWinGranulity);
  kPrintf("X Resolution: %d\n", pstModeInfo->wXResolution);
  kPrintf("Y Resolution: %d\n", pstModeInfo->wYResolution);
  kPrintf("Bits Per Pixel: %d\n", pstModeInfo->bBitsPerPixel);

  kPrintf("Red Mask Size: %d, Field Position: %d\n", pstModeInfo->bRedMaskSize,
    pstModeInfo->bRedFieldPosition);
  kPrintf("Green Mask Size: %d, Field Position: %d\n", pstModeInfo->bGreenMaskSize,
    pstModeInfo->bGreenFieldPosition);
  kPrintf("Blue Mask Size: %d, Field Position: %d\n", pstModeInfo->bBlueMaskSize,
    pstModeInfo->bBlueFieldPosition);
  kPrintf("Physical Base Pointer: 0x%X\n", pstModeInfo->dwPhysicalBasePointer);

  kPrintf("Linear Red Mask Size: %d, Field Position: %d\n",
    pstModeInfo->bLinearRedMaskSize, pstModeInfo->bLinearRedFieldPosition);
  kPrintf("Linear Green Mask Size: %d, Field Position: %d\n",
    pstModeInfo->bLinearGreenMaskSize, pstModeInfo->bLinearGreenFieldPosition);
  kPrintf("Linear Blue Mask Size: %d, Field Position: %d\n",
    pstModeInfo->bLinearBlueMaskSize, pstModeInfo->bLinearBlueFieldPosition);
}

/*
  Execute User Application
*/
static void kExecuteApplicationProgram(const char* pcParameterBuffer)
{
  PARAMETERLIST stList;
  char vcFileName[512];
  char vcArgumentString[1024];
  QWORD qwID;

  kInitializeParameter(&stList, pcParameterBuffer);
  if (kGetNextParameter(&stList, vcFileName) == 0)
  {
    kPrintf("ex)exec a.elf argument\n");
    return;
  }

  if (kGetNextParameter(&stList, vcArgumentString) == 0)
  {
    vcArgumentString[0] = '\0';
  }

  kPrintf("Execute Program... File [%s], Argument [%s]\n", vcFileName,
    vcArgumentString);

  // Create Task
  qwID = kExecuteProgram(vcFileName, vcArgumentString, TASK_LOADBALANCINGID);
  kPrintf("Task ID = 0x%Q\n", qwID);
}

static void kTelnetC(const char* pcParameterBuffer)
{
  /*
    TODO : Error Handle
  */
  PARAMETERLIST stList;
  char vcIP[20];
  char vcPort[10];
  DWORD dwIP;
  WORD wPort;
  char* pcTemp;
  BYTE bTemp;

  kInitializeParameter(&stList, pcParameterBuffer);
  if (kGetNextParameter(&stList, vcIP) == 0)
  {
    kPrintf("ex)telnet <ip-addr> <port>\n");
    return;
  }

  // IP 주소 정수로 변환
  dwIP = 0;
  pcTemp = vcIP;
  bTemp = 0;
  while (*pcTemp != '\0') {
    if (*pcTemp == '.') {
      dwIP = (dwIP << 8) + bTemp;
      bTemp = 0;
    }
    else {
      bTemp = (bTemp * 10) + (*pcTemp - '0');
    }
    pcTemp++;
  }
  dwIP = (dwIP << 8) + bTemp;

  if (kGetNextParameter(&stList, vcPort) == 0)
  {
    kPrintf("ex)telnet <ip-addr> <port>\n");
    return;
  }
  wPort = kAToI(vcPort, 10);

  kTelent_SimpleClient(dwIP, wPort);
}

static void kInitNetwork(const char* pcParameterBuffer)
{
  if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_HIGHEST , 0, 0,
    (QWORD)kEthernet_Task, TASK_LOADBALANCINGID) == NULL)
  {
    kPrintf("Create kEthernet_Task Fail\n");
    return;
  }

  if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0,
    (QWORD)kARP_Task, TASK_LOADBALANCINGID) == NULL)
  {
    kPrintf("Create kInternet_Task Fail\n");
  }

  if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_HIGHEST, 0, 0,
    (QWORD)kIP_Task, TASK_LOADBALANCINGID) == NULL)
  {
    kPrintf("Create kIP_Task Fail\n");
  }

  if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0,
    (QWORD)kICMP_Task, TASK_LOADBALANCINGID) == NULL)
  {
    kPrintf("Create kICMP_Task Fail\n");
  }

  if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0,
    (QWORD)kUDP_Task, TASK_LOADBALANCINGID) == NULL)
  {
    kPrintf("Create kUDP_Task Fail\n");
  }

  if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_HIGHEST, 0, 0,
    (QWORD)kTCP_Task, TASK_LOADBALANCINGID) == NULL)
  {
    kPrintf("Create kTCP_Task Fail\n");
  }

  if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0,
    (QWORD)kDNS_Task, TASK_LOADBALANCINGID) == NULL)
  {
    kPrintf("Create kDNS_Task Fail\n");
  }

  if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0,
    (QWORD)kDHCP_Task, TASK_LOADBALANCINGID) == NULL)
  {
    kPrintf("Create kDHCP_Task Fail\n");
  }

}

static void kShowDNSState(const char* pcParameterBuffer)
{
  kDNSCache_Print();
}

static void kShowARPState(const char* pcParameterBuffer)
{
  kARPTable_Print();
}

static void kShowDHCPState(const char* pcParameterBuffer)
{
  kDHCP_ShowState();
}
