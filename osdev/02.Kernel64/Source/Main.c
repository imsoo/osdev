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
#include "2DGraphics.h"

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

/*
  Main for Graphic Mode Test
*/
void kStartGraphicModeTest()
{
  VBEMODEINFOBLOCK* pstVBEMode;
  int iX1, iY1, iX2, iY2;
  COLOR stColor1, stColor2;
  int i;
  char* vpcString[] = { "Pixel", "Line", "Rectangle", "Circle", "Hello, World" };
  COLOR* pstVideoMemory;
  RECT stScreenArea;

  pstVBEMode = kGetVBEModeInfoBlock();
  pstVideoMemory = (COLOR*)((QWORD)pstVBEMode->dwPhysicalBasePointer & 0xFFFFFFFF);

  stScreenArea.iX1 = 0;
  stScreenArea.iY1 = 0;
  stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
  stScreenArea.iY2 = pstVBEMode->wYResolution - 1;


  // Print String Pixel
  kInternalDrawText(&stScreenArea, pstVideoMemory, 0, 0, RGB(255, 255, 255), RGB(0, 0, 0), vpcString[0],
    kStrLen(vpcString[0]));
  kInternalDrawPixel(&stScreenArea, pstVideoMemory, 1, 20, RGB(255, 255, 255));
  kInternalDrawPixel(&stScreenArea, pstVideoMemory, 2, 20, RGB(255, 255, 255));

  // Draw Red Line 
  kInternalDrawText(&stScreenArea, pstVideoMemory, 0, 25, RGB(255, 0, 0), RGB(0, 0, 0), vpcString[1],
    kStrLen(vpcString[1]));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, 20, 50, 1000, 50, RGB(255, 0, 0));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, 20, 50, 1000, 100, RGB(255, 0, 0));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, 20, 50, 1000, 150, RGB(255, 0, 0));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, 20, 50, 1000, 200, RGB(255, 0, 0));
  kInternalDrawLine(&stScreenArea, pstVideoMemory, 20, 50, 1000, 250, RGB(255, 0, 0));

  // Draw Green Rect
  kInternalDrawText(&stScreenArea, pstVideoMemory, 0, 180, RGB(0, 255, 0), RGB(0, 0, 0), vpcString[2],
    kStrLen(vpcString[2]));
  kInternalDrawRect(&stScreenArea, pstVideoMemory, 20, 200, 70, 250, RGB(0, 255, 0), FALSE);
  kInternalDrawRect(&stScreenArea, pstVideoMemory, 120, 200, 220, 300, RGB(0, 255, 0), TRUE);
  kInternalDrawRect(&stScreenArea, pstVideoMemory, 270, 200, 420, 350, RGB(0, 255, 0), FALSE);
  kInternalDrawRect(&stScreenArea, pstVideoMemory, 470, 200, 670, 400, RGB(0, 255, 0), TRUE);

  // Draw Blue Circle
  kInternalDrawText(&stScreenArea, pstVideoMemory, 0, 550, RGB(0, 0, 255), RGB(0, 0, 0), vpcString[3],
    kStrLen(vpcString[3]));
  kInternalDrawCircle(&stScreenArea, pstVideoMemory, 45, 600, 25, RGB(0, 0, 255), FALSE);
  kInternalDrawCircle(&stScreenArea, pstVideoMemory, 170, 600, 50, RGB(0, 0, 255), TRUE);
  kInternalDrawCircle(&stScreenArea, pstVideoMemory, 345, 600, 75, RGB(0, 0, 255), FALSE);
  kInternalDrawCircle(&stScreenArea, pstVideoMemory, 570, 600, 100, RGB(0, 0, 255), TRUE);

  kGetCh();

  do
  {
    // Rnadom Pixel
    for (i = 0; i < 100; i++)
    {
      kGetRandomXY(&iX1, &iY1);
      stColor1 = kGetRandomColor();

      kInternalDrawPixel(&stScreenArea, pstVideoMemory, iX1, iY1, stColor1);
    }

    // Rnadom Line
    for (i = 0; i < 100; i++)
    {
      kGetRandomXY(&iX1, &iY1);
      kGetRandomXY(&iX2, &iY2);
      stColor1 = kGetRandomColor();
      kInternalDrawLine(&stScreenArea, pstVideoMemory, iX1, iY1, iX2, iY2, stColor1);
    }

    // Rnadom Rect
    for (i = 0; i < 20; i++)
    {
      kGetRandomXY(&iX1, &iY1);
      kGetRandomXY(&iX2, &iY2);
      stColor1 = kGetRandomColor();

      kInternalDrawRect(&stScreenArea, pstVideoMemory, iX1, iY1, iX2, iY2, stColor1, kRandom() % 2);
    }

    // Rnadom Circle
    for (i = 0; i < 100; i++)
    {
      kGetRandomXY(&iX1, &iY1);
      stColor1 = kGetRandomColor();

      kInternalDrawCircle(&stScreenArea, pstVideoMemory, iX1, iY1, ABS(kRandom() % 50 + 1), stColor1, kRandom() % 2);
    }

    // Rnadom Text
    for (i = 0; i < 100; i++)
    {
      kGetRandomXY(&iX1, &iY1);
      stColor1 = kGetRandomColor();
      stColor2 = kGetRandomColor();
      kInternalDrawText(&stScreenArea, pstVideoMemory, iX1, iY1, stColor1, stColor2, vpcString[4],
        kStrLen(vpcString[4]));
    }
  } while (kGetCh() != 'q');

  while (1)
  {
    // Background
    kInternalDrawRect(&stScreenArea, pstVideoMemory, 0, 0, 1024, 768, RGB(232, 255, 232), TRUE);

    // Random WindowFrame
    for (i = 0; i < 3; i++)
    {
      kGetRandomXY(&iX1, &iY1);
      kDrawWindowFrame(iX1, iY1, 400, 200, "Window prototype");
    }

    kGetCh();
  }
}