#include "Types.h"
#include "Keyboard.h"
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

// AP C Kernele Entry
void MainForApplicationProcessor(void);

// Graphic Mode Test
void kStartGraphicModeTest();

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

  // idle task start
  kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask, kGetAPICID());

  // if CLI Mode : Shell
  if (*(BYTE*)VBE_STARTGRAPHICMODEFLAGADDRESS == 0) {
    // Shell Start
    kStartConsoleShell();
  }
  else {
    kStartGraphicModeTest();
  }
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

  kPrintf("Application Processor[APIC ID: %d] is activation\n", kGetAPICID());

  kIdleTask();
}

/*
  Main for Graphic Mode Test
*/
void kStartGraphicModeTest()
{
  VBEMODEINFOBLOCK* pstVBEMode;
  WORD* pwFrameBufferAddress;
  WORD wColor = 0;
  int iBandHeight;
  int i, j;

  kGetCh();

  // Get Frame Buffer Address
  pstVBEMode = kGetVBEModeInfoBlock();
  pwFrameBufferAddress = (WORD*)((QWORD)pstVBEMode->dwPhysicalBasePointer);

  iBandHeight = pstVBEMode->wYResolution / 32;

  while (1) {
    for (j = 0; j < pstVBEMode->wYResolution; j++) {
      for (i = 0; i < pstVBEMode->wXResolution; i++) {
        pwFrameBufferAddress[(j * pstVBEMode->wXResolution) + i] = wColor;
      }

      if ((j % iBandHeight) == 0) {
        wColor = kRandom() & 0xFFFF;
      }
    }

    kGetCh();
  }
}