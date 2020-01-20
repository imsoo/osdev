#include "Types.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Descriptor.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"
#include "MultiProcessor.h"
#include "VBE.h"
#include "2DGraphics.h"
#include "MPConfigurationTable.h"
#include "WindowManagerTask.h"
#include "SystemCall.h"

// AP C Kernele Entry
void MainForApplicationProcessor(void);

BOOL kChangeMultiCoreMode(void);

// C Kernel Entry Point
void Main(void)
{
  int iCursorX, iCursorY;

  // if AP
  if (*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0) {
    MainForApplicationProcessor();
  }
  *((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) = 0;

  // Init Console
  kInitializeConsole(0, 10);
  kPrintf("Switch To IA - 32e Mode Success...\n");
  kPrintf("Switch To IA-32e Mode Success...\n");
  kPrintf("IA-32e C Language Kernel Start..............[Pass]\n");

  kGetCursor(&iCursorX, &iCursorY);
  kPrintf("GDT Initialize And Switch For IA-32e Mode...[    ]");
  kInitializeGDTTableAndTSS();
  kLoadGDTR(GDTR_STARTADDRESS);
  kSetCursor(45, iCursorY++);
  kPrintf("Pass\n");

  kPrintf("TSS Segment Load............................[    ]");
  kLoadTR(GDT_TSSSEGMENT);
  kSetCursor(45, iCursorY++);
  kPrintf("Pass\n");

  kPrintf("IDT Initialize..............................[    ]");
  kInitializeIDTTables();
  kLoadIDTR(IDTR_STARTADDRESS);
  kSetCursor(45, iCursorY++);
  kPrintf("Pass\n");

  kPrintf("Total RAM Size Check........................[    ]");
  kCheckTotalRAMSize();
  kSetCursor(45, iCursorY++);
  kPrintf("Pass], Size = %dMB\n", kGetTotalRAMSize());

  kPrintf("TCB Pool And Scheduler Initialize...........[Pass]\n");
  iCursorY++;
  kInitializeScheduler();
  // set PIT 1ms
  kInitializePIT(MSTOCOUNT(1), TRUE);

  kPrintf("Dynamic MEmory Initialize...................[Pass]\n");
  iCursorY++;
  kInitializeDynamicMemory();

  kPrintf("Keyboard Activate And Queue Initialize......[    ]");

  // key board enable
  if (kInitializeKeyboard() == TRUE) {
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");
    kChangeKeyboardLED(FALSE, FALSE, FALSE);
  } 
  else {
    kSetCursor(45, iCursorY++);
    kPrintf("Fail\n");
    while (1);
  } 

  kPrintf("Mouse Activate And Queue Initialize.........[    ]");

  // Mouse board enable
  if (kInitializeMouse() == TRUE) {
    kEnableMouseInterrupt();
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");
  }
  else {
    kSetCursor(45, iCursorY++);
    kPrintf("Fail\n");
    while (1);
  }

  kPrintf("PIC Controller And Interrupt Initialize.....[    ]");
  // Init PIC
  kInitializePIC();
  // Interrupt Mask clear
  kMaskPICInterrupt(0);
  // Enable interrupt
  kEnableInterrupt();
  kSetCursor(45, iCursorY++);
  kPrintf("Pass\n");

  // Init FileSystem
  kPrintf("File System Initialize......................[    ]");

  if (kInitializeFileSystem() == TRUE) {
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");
  }
  else {
    kSetCursor(45, iCursorY++);
    kPrintf("Fail\n");
  }

  // Init Serial Port
  kPrintf("Serial Port Initialize......................[Pass]\n");
  iCursorY++;
  kInitializeSerialPort();

  // Change to multicore Processor Mode
  kPrintf("Change To MultiCore Processor Mode..........[    ]");
  if (kChangeMultiCoreMode() == TRUE) {
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");
  }
  else {
    kSetCursor(45, iCursorY++);
    kPrintf("Fail\n");
  }

  // Init System Call MSR
  kPrintf("System Call MSR Initialize..................[Pass]\n");
  iCursorY++;
  kInitializeSystemCall();

  // idle task start
  kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask, kGetAPICID());

  // if CLI Mode : Shell
  if (*(BYTE*)VBE_STARTGRAPHICMODEFLAGADDRESS == 0) {
    // Shell Start
    kStartConsoleShell();
  }
  else {
    kStartWindowManager();
  }
}

/*
  Change To MultiCore Processor Mode
*/
BOOL kChangeMultiCoreMode(void)
{
  MPCONFIGRUATIONMANAGER* pstMPManager;
  BOOL bInterruptFlag;
  int i;

  // Application Processor
  if (kStartUpApplicationProcessor() == FALSE)
  {
    return FALSE;
  }

  // Check PIC Mode
  pstMPManager = kGetMPConfigurationManager();
  if (pstMPManager->bUsePICMode == TRUE)
  {
    // Disable PIC Mode
    kOutPortByte(0x22, 0x70);
    kOutPortByte(0x23, 0x01);
  }

  // Disable PIC Controller Interrupt
  kMaskPICInterrupt(0xFFFF);

  // Enable Global APIC
  kEnableGlobalLocalAPIC();

  // Enable Local APIC
  kEnableSoftwareLocalAPIC();

  bInterruptFlag = kSetInterruptFlag(FALSE);

  // Set TPR
  kSetTaskPriority(0);

  // Init Local Vector Table
  kInitializeLocalVectorTable();

  // Set SymmetricIOMode Flag
  kSetSymmetricIOMode(TRUE);

  // Init I/O APIC
  kInitializeIORedirectionTable();

  kSetInterruptFlag(bInterruptFlag);

  // Set InterruptLoadBalancing
  kSetInterruptLoadBalancing(TRUE);

  // Set TaskLoadBalancing
  for (i = 0; i < MAXPROCESSORCOUNT; i++)
  {
    kSetTaskLoadBalancing(i, TRUE);
  }

  return TRUE;
}

/*
  AP (Application Processor) Kernel Entry Point
*/
void MainForApplicationProcessor(void)
{
  QWORD qwTickCount;

  // LOAD GDT
  kLoadGDTR(GDTR_STARTADDRESS);

  // LOAD TSS
  kLoadTR(GDT_TSSSEGMENT + (kGetAPICID() * sizeof(GDTENTRY16)));

  // Load IDTR
  kLoadIDTR(IDTR_STARTADDRESS);

  // Init Scheduler
  kInitializeScheduler();

  // Enable Software Local APIC
  kEnableSoftwareLocalAPIC();

  // Set TSR to 0 for receive all interrupt 
  kSetTaskPriority(0);

  // Init Local APIC Vector Table
  kInitializeLocalVectorTable();

  // Enable Interrupt
  kEnableInterrupt();

  // Init System Call MSR
  kInitializeSystemCall();

  kIdleTask();
}