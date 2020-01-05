#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"

// C Kernel 
void Main(void)
{
  int iCursorX, iCursorY;

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

  kPrintf("TCB Pool And Scheduler Initialize...........[PASS]\n");
  iCursorY++;
  kInitializeScheduler();
  // set PIT 1ms
  kInitializePIT(MSTOCOUNT(1), 0);

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

  // idle task start
  kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask);
  // Shell Start
  kStartConsoleShell();
}