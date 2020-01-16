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

// AP C Kernele Entry
void MainForApplicationProcessor(void);

BOOL kChangeMultiCoreMode(void);

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

  kIdleTask();
}

#define ABS( x )    ( ( ( x ) >= 0 ) ? ( x ) : -( x ) )

/*
  Get Random X, Y
*/
void kGetRandomXY(int* piX, int* piY)
{
  int iRandom;

  // X
  iRandom = kRandom();
  *piX = ABS(iRandom) % 1000;

  // Y
  iRandom = kRandom();
  *piY = ABS(iRandom) % 700;
}

/*
  Get Random Color
*/
COLOR kGetRandomColor(void)
{
  int iR, iG, iB;
  int iRandom;

  iRandom = kRandom();
  iR = ABS(iRandom) % 256;

  iRandom = kRandom();
  iG = ABS(iRandom) % 256;

  iRandom = kRandom();
  iB = ABS(iRandom) % 256;

  return RGB(iR, iG, iB);
}

/*
  Draw Window Frame
*/
void kDrawWindowFrame(int iX, int iY, int iWidth, int iHeight, const char* pcTitle)
{
  char* pcTestString1 = "Window prototype";
  char* pcTestString2 = "Hello, World";
  VBEMODEINFOBLOCK* pstVBEMode;
  COLOR* pstVideoMemory;
  RECT stScreenArea;

  pstVBEMode = kGetVBEModeInfoBlock();
  pstVideoMemory = (COLOR*)((QWORD)pstVBEMode->dwPhysicalBasePointer & 0xFFFFFFFF);

  stScreenArea.iX1 = 0;
  stScreenArea.iY1 = 0;
  stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
  stScreenArea.iY2 = pstVBEMode->wYResolution - 1;

  kInternalDrawRect(&stScreenArea, pstVideoMemory, iX, iY, iX + iWidth, iY + iHeight, RGB(109, 218, 22), FALSE);
  kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + 1, iY + 1, iX + iWidth - 1, iY + iHeight - 1, RGB(109, 218, 22),
    FALSE);

  // Title BackGround
  kInternalDrawRect(&stScreenArea, pstVideoMemory, iX, iY + 3, iX + iWidth - 1, iY + 21, RGB(79, 204, 11), TRUE);
  // Title Text
  kInternalDrawText(&stScreenArea, pstVideoMemory, iX + 6, iY + 3, RGB(255, 255, 255), RGB(79, 204, 11),
    pcTitle, kStrLen(pcTitle));

  // Title Box Line 
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 1, iY + 1, iX + iWidth - 1, iY + 1, RGB(183, 249, 171));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 1, iY + 2, iX + iWidth - 1, iY + 2, RGB(150, 210, 140));

  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 1, iY + 2, iX + 1, iY + 20, RGB(183, 249, 171));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 2, iY + 2, iX + 2, iY + 20, RGB(150, 210, 140));

  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 2, iY + 19, iX + iWidth - 2, iY + 19, RGB(46, 59, 30));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 2, iY + 20, iX + iWidth - 2, iY + 20, RGB(46, 59, 30));

  // Close button Box
  kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18, iY + 1, iX + iWidth - 2, iY + 19,
    RGB(255, 255, 255), TRUE);
   
  // Close button  Box Line
  kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + iWidth - 2, iY + 1, iX + iWidth - 2, iY + 19 - 1,
    RGB(86, 86, 86), TRUE);
  kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 1, iY + 1, iX + iWidth - 2 - 1, iY + 19 - 1,
    RGB(86, 86, 86), TRUE);
  kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 1, iY + 19, iX + iWidth - 2, iY + 19,
    RGB(86, 86, 86), TRUE);
  kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 1, iY + 19 - 1, iX + iWidth - 2, iY + 19 - 1,
    RGB(86, 86, 86), TRUE);

  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18, iY + 1, iX + iWidth - 2 - 1, iY + 1,
    RGB(229, 229, 229));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18, iY + 1 + 1, iX + iWidth - 2 - 2, iY + 1 + 1,
    RGB(229, 229, 229));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18, iY + 1, iX + iWidth - 2 - 18, iY + 19,
    RGB(229, 229, 229));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 1, iY + 1, iX + iWidth - 2 - 18 + 1, iY + 19 - 1,
    RGB(229, 229, 229));

  // Close Sign (X)
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 4, iY + 1 + 4, iX + iWidth - 2 - 4, iY + 19 - 4,
    RGB(71, 199, 21));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 5, iY + 1 + 4, iX + iWidth - 2 - 4, iY + 19 - 5,
    RGB(71, 199, 21));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 4, iY + 1 + 5, iX + iWidth - 2 - 5, iY + 19 - 4,
    RGB(71, 199, 21));

  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 4, iY + 19 - 4, iX + iWidth - 2 - 4, iY + 1 + 4,
    RGB(71, 199, 21));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 5, iY + 19 - 4, iX + iWidth - 2 - 4, iY + 1 + 5,
    RGB(71, 199, 21));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 4, iY + 19 - 5, iX + iWidth - 2 - 5, iY + 1 + 4,
    RGB(71, 199, 21));

  // Content Box
  kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + 2, iY + 21, iX + iWidth - 2, iY + iHeight - 2,
    RGB(255, 255, 255), TRUE);

  // Content String
  kInternalDrawText(&stScreenArea, pstVideoMemory, iX + 10, iY + 30, RGB(0, 0, 0), RGB(255, 255, 255), pcTestString1,
    kStrLen(pcTestString1));
  kInternalDrawText(&stScreenArea, pstVideoMemory, iX + 10, iY + 50, RGB(0, 0, 0), RGB(255, 255, 255), pcTestString2,
    kStrLen(pcTestString2));
}

// Mouse Cursor Width / Height
#define MOUSE_CURSOR_WIDTH      20
#define MOUSE_CURSOR_HEIGHT     20

// Mouse cursor Image
static BYTE gs_vwMouseBuffer[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT] =
{
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1, 0, 0,
    0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 3, 2, 1, 1, 2, 3, 2, 2, 2, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 2, 2, 1, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0,
    0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0,
    0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
    0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
};

// Mouse Cursor Color
#define MOUSE_CURSOR_OUTERLINE      RGB(0, 0, 0 )
#define MOUSE_CURSOR_OUTER          RGB( 79, 204, 11 )
#define MOUSE_CURSOR_INNER          RGB( 232, 255, 232 )

/*
  Draw Cursor
*/
void kDrawCursor(RECT* pstArea, COLOR* pstVideoMemory, int iX, int iY)
{
  int i;
  int j;
  BYTE* pbCurrentPos;

  pbCurrentPos = gs_vwMouseBuffer;

  for (j = 0; j < MOUSE_CURSOR_HEIGHT; j++)
  {
    for (i = 0; i < MOUSE_CURSOR_WIDTH; i++)
    {
      switch (*pbCurrentPos)
      {
      case 0:
        // nothing
        break;

        // OuterLine
      case 1:
        kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY,
          MOUSE_CURSOR_OUTERLINE);
        break;

        // Outer
      case 2:
        kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY,
          MOUSE_CURSOR_OUTER);
        break;

        // Inner
      case 3:
        kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY,
          MOUSE_CURSOR_INNER);
        break;
      }

      pbCurrentPos++;
    }
  }
}

/*
  Main for Graphic Mode Test
*/
void kStartGraphicModeTest()
{
  VBEMODEINFOBLOCK* pstVBEMode;
  int iX, iY;
  COLOR* pstVideoMemory;
  RECT stScreenArea;
  int iRelativeX, iRelativeY;
  BYTE bButton;

  pstVBEMode = kGetVBEModeInfoBlock();
  pstVideoMemory = (COLOR*)((QWORD)pstVBEMode->dwPhysicalBasePointer & 0xFFFFFFFF);

  stScreenArea.iX1 = 0;
  stScreenArea.iY1 = 0;
  stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
  stScreenArea.iY2 = pstVBEMode->wYResolution - 1;

  // Mouse Mosition
  iX = pstVBEMode->wXResolution / 2;
  iY = pstVBEMode->wYResolution / 2;

  // BackGround
  kInternalDrawRect(&stScreenArea, pstVideoMemory,
    stScreenArea.iX1, stScreenArea.iY1, stScreenArea.iX2, stScreenArea.iY2,
    RGB(232, 255, 232), TRUE);

  // Draw Cursor
  kDrawCursor(&stScreenArea, pstVideoMemory, iX, iY);

  while (1)
  {
    // Wait Mouse Packet
    if (kGetMouseDataFromMouseQueue(&bButton, &iRelativeX, &iRelativeY) ==
      FALSE)
    {
      kSleep(0);
      continue;
    }


    // previose cursor's position clear
    kInternalDrawRect(&stScreenArea, pstVideoMemory, iX, iY,
      iX + MOUSE_CURSOR_WIDTH, iY + MOUSE_CURSOR_HEIGHT,
      RGB(232, 255, 232), TRUE);

    // Calc current position
    iX += iRelativeX;
    iY += iRelativeY;

    // if position is out of screen
    if (iX < stScreenArea.iX1)
    {
      iX = stScreenArea.iX1;
    }
    else if (iX > stScreenArea.iX2)
    {
      iX = stScreenArea.iX2;
    }

    if (iY < stScreenArea.iY1)
    {
      iY = stScreenArea.iY1;
    }
    else if (iY > stScreenArea.iY2)
    {
      iY = stScreenArea.iY2;
    }

    // left Button : Draw Window
    if (bButton & MOUSE_LBUTTONDOWN)
    {
      kDrawWindowFrame(iX - 200, iY - 100, 400, 200, "Window prototype");
    }
    // right button : clear screen
    else if (bButton & MOUSE_RBUTTONDOWN)
    {
      kInternalDrawRect(&stScreenArea, pstVideoMemory,
        stScreenArea.iX1, stScreenArea.iY1, stScreenArea.iX2,
        stScreenArea.iY2, RGB(232, 255, 232), TRUE);
    }

    // Draw Cursor
    kDrawCursor(&stScreenArea, pstVideoMemory, iX, iY);
  }
}