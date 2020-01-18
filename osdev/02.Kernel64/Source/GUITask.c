#include "GUITask.h"
#include "Utility.h"
#include "Window.h"
#include "DynamicMemory.h"
#include "Font.h"
#include "MultiProcessor.h"
#include "Task.h"
#include "Console.h"
#include "ConsoleShell.h"

// for GUI Console Shell
static CHARACTER gs_vstPreviousScreenBuffer[CONSOLE_WIDTH * CONSOLE_HEIGHT];

/*
  Base GUI Task 
*/
void kBaseGUITask(void)
{
  QWORD qwWindowID;
  int iMouseX, iMouseY;
  int iWindowWidth, iWindowHeight;

  EVENT stReceivedEvent;
  MOUSEEVENT* pstMouseEvent;
  KEYEVENT* pstKeyEvent;
  WINDOWEVENT* pstWindowEvent;

  // Check GUI Enabled
  if (kIsGraphicMode() == FALSE) {
    kPrintf("This task can run only GUI Mode...\n");
    return;
  }

  // Get Cursor Info
  kGetCursorPosition(&iMouseX, &iMouseY);

  // Window Info Set
  iWindowWidth = 500;
  iWindowHeight = 200;
  
  // create Window
  qwWindowID = kCreateWindow(iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2,
    iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT, "Hello World Window");
  if (qwWindowID == WINDOW_INVALIDID) {
    return;
  }

  // Event Loop
  while (1) {

    // Get Event From Window Event Queue
    if (kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stReceivedEvent.qwType)
    {
    // Mouse Event
    case EVENT_MOUSE_MOVE:
    case EVENT_MOUSE_LBUTTONDOWN:
    case EVENT_MOUSE_LBUTTONUP:
    case EVENT_MOUSE_RBUTTONDOWN:
    case EVENT_MOUSE_RBUTTONUP:
    case EVENT_MOUSE_MBUTTONDOWN:
    case EVENT_MOUSE_MBUTTONUP:
      pstMouseEvent = &(stReceivedEvent.stMouseEvent);
      break;
    

    // Key Event
    case EVENT_KEY_DOWN:
    case EVENT_KEY_UP:
      pstKeyEvent = &(stReceivedEvent.stKeyEvent);
      break;

    // Window Event
    case EVENT_WINDOW_SELECT:
    case EVENT_WINDOW_DESELECT:
    case EVENT_WINDOW_MOVE:
    case EVENT_WINDOW_RESIZE:
    case EVENT_WINDOW_CLOSE:
      pstWindowEvent = &(stReceivedEvent.stWindowEvent);

      // If WINDOW_CLOSE
      if (stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
      {
        kDeleteWindow(qwWindowID);
        return;
      }
      break;

    // ETC Event
    default:
      break;
    }
  }
}

/*
  Hello World GUI Task
*/
void kHelloWorldGUITask(void)
{
  QWORD qwWindowID;
  int iMouseX, iMouseY;
  int iWindowWidth, iWindowHeight;

  EVENT stReceivedEvent;
  MOUSEEVENT* pstMouseEvent;
  KEYEVENT* pstKeyEvent;
  WINDOWEVENT* pstWindowEvent;

  // Info
  int iY;
  char vcTempBuffer[50];
  static int s_iWindowCount = 0;
  char* vpcEventString[] = {
            "Unknown",
            "MOUSE_MOVE       ",
            "MOUSE_LBUTTONDOWN",
            "MOUSE_LBUTTONUP  ",
            "MOUSE_RBUTTONDOWN",
            "MOUSE_RBUTTONUP  ",
            "MOUSE_MBUTTONDOWN",
            "MOUSE_MBUTTONUP  ",
            "WINDOW_SELECT    ",
            "WINDOW_DESELECT  ",
            "WINDOW_MOVE      ",
            "WINDOW_RESIZE    ",
            "WINDOW_CLOSE     ",
            "KEY_DOWN         ",
            "KEY_UP           " };

  // Button
  RECT stButtonArea;
  QWORD qwFindWindowID;
  EVENT stSendEvent;
  int i;


  // Check GUI Enabled
  if (kIsGraphicMode() == FALSE) {
    kPrintf("This task can run only GUI Mode...\n");
    return;
  }

  // Get Cursor Info
  kGetCursorPosition(&iMouseX, &iMouseY);

  // Window Info Set
  iWindowWidth = 500;
  iWindowHeight = 200;

  // create Window
  kSPrintf(vcTempBuffer, "Hello World Window %d", s_iWindowCount++);
  qwWindowID = kCreateWindow(iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2,
    iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT, vcTempBuffer);
  if (qwWindowID == WINDOW_INVALIDID) {
    return;
  }

  // Event Info Area
  iY = WINDOW_TITLEBAR_HEIGHT + 10;
  kDrawRect(qwWindowID, 10, iY + 8, iWindowWidth - 10, iY + 70, WINDOW_COLOR_WHITE, FALSE);
  kSPrintf(vcTempBuffer, "GUI Event Information[Window ID : 0x%Q]", qwWindowID);
  kDrawText(qwWindowID, 20, iY, WINDOW_COLOR_WHITE, WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

  // Buttuon Area
  kSetRectangleData(10, iY + 80, iWindowWidth - 10, iWindowHeight - 10,
    &stButtonArea);
  kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BUTTONINACTIVATEBACKGROUND,
    "User Message Send Button(Up)", WINDOW_COLOR_WHITE);
  kShowWindow(qwWindowID, TRUE);

  // Event Loop
  while (1) {

    // Get Event From Window Event Queue
    if (kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE) {
      kSleep(0);
      continue;
    }

    // Info Area Clear
    kDrawRect(qwWindowID, 11, iY + 20, iWindowWidth - 11, iY + 69, WINDOW_COLOR_BACKGROUND, TRUE);

    switch (stReceivedEvent.qwType)
    {
      // Mouse Event
    case EVENT_MOUSE_MOVE:
    case EVENT_MOUSE_LBUTTONDOWN:
    case EVENT_MOUSE_LBUTTONUP:
    case EVENT_MOUSE_RBUTTONDOWN:
    case EVENT_MOUSE_RBUTTONUP:
    case EVENT_MOUSE_MBUTTONDOWN:
    case EVENT_MOUSE_MBUTTONUP:
      pstMouseEvent = &(stReceivedEvent.stMouseEvent);

      // Print Event Type
      kSPrintf(vcTempBuffer, "Mouse Event: %s",
        vpcEventString[stReceivedEvent.qwType]);
      kDrawText(qwWindowID, 20, iY + 20, WINDOW_COLOR_WHITE,
        WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

      // Print Data
      kSPrintf(vcTempBuffer, "Data: X = %d, Y = %d, Button = %X",
        pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY,
        pstMouseEvent->bButtonStatus);
      kDrawText(qwWindowID, 20, iY + 40, WINDOW_COLOR_WHITE,
        WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

      // L Button Down
      if (stReceivedEvent.qwType == EVENT_MOUSE_LBUTTONDOWN) {
        if (kIsInRectangle(&stButtonArea, pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY) == TRUE) {
          kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BUTTONACTIVATEBACKGROUND,
            "User Message Send Button(Down)", WINDOW_COLOR_WHITE);
          kUpdateScreenByID(qwWindowID);

          // Create User Event 
          stSendEvent.qwType = EVENT_USER_TESTMESSAGE;
          stSendEvent.vqwData[0] = qwWindowID;
          stSendEvent.vqwData[1] = 0x1234;
          stSendEvent.vqwData[2] = 0x5678;

          // Send to Other window
          for (i = 0; i < s_iWindowCount; i++)
          {
            // Find Window using Title Name
            kSPrintf(vcTempBuffer, "Hello World Window %d", i);
            qwFindWindowID = kFindWindowByTitle(vcTempBuffer);

            // Send Event
            if ((qwFindWindowID != WINDOW_INVALIDID) &&
              (qwFindWindowID != qwWindowID))
            {
              kSendEventToWindow(qwFindWindowID, &stSendEvent);
            }
          }
        }
      }
      // L Button Up
      else if (stReceivedEvent.qwType == EVENT_MOUSE_LBUTTONUP)
      {
        kDrawButton(qwWindowID, &stButtonArea,
          WINDOW_COLOR_BUTTONINACTIVATEBACKGROUND, "User Message Send Button(Up)",
          WINDOW_COLOR_WHITE);
      }

      break;

      // Key Event
    case EVENT_KEY_DOWN:
    case EVENT_KEY_UP:
      pstKeyEvent = &(stReceivedEvent.stKeyEvent);

      // Print Event Type
      kSPrintf(vcTempBuffer, "Key Event: %s",
        vpcEventString[stReceivedEvent.qwType]);
      kDrawText(qwWindowID, 20, iY + 20, WINDOW_COLOR_WHITE,
        WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

      // Print Data
      kSPrintf(vcTempBuffer, "Data: Key = %c, Flag = %X",
        pstKeyEvent->bASCIICode, pstKeyEvent->bFlags);
      kDrawText(qwWindowID, 20, iY + 40, WINDOW_COLOR_WHITE,
        WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

      break;

      // Window Event
    case EVENT_WINDOW_SELECT:
    case EVENT_WINDOW_DESELECT:
    case EVENT_WINDOW_MOVE:
    case EVENT_WINDOW_RESIZE:
    case EVENT_WINDOW_CLOSE:
      pstWindowEvent = &(stReceivedEvent.stWindowEvent);

      // Print Event Type
      kSPrintf(vcTempBuffer, "Window Event: %s",
        vpcEventString[stReceivedEvent.qwType]);
      kDrawText(qwWindowID, 20, iY + 20, WINDOW_COLOR_WHITE,
        WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

      // Print Data
      kSPrintf(vcTempBuffer, "Data: X1 = %d, Y1 = %d, X2 = %d, Y2 = %d",
        pstWindowEvent->stArea.iX1, pstWindowEvent->stArea.iY1,
        pstWindowEvent->stArea.iX2, pstWindowEvent->stArea.iY2);
      kDrawText(qwWindowID, 20, iY + 40, WINDOW_COLOR_WHITE,
        WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

      // If WINDOW_CLOSE
      if (stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
      {
        kDeleteWindow(qwWindowID);
        return;
      }
      break;

      // ETC Event
    default:

      // Print Event Type
      kSPrintf(vcTempBuffer, "Unknown Event: 0x%X", stReceivedEvent.qwType);
      kDrawText(qwWindowID, 20, iY + 20, WINDOW_COLOR_WHITE, WINDOW_COLOR_BACKGROUND,
        vcTempBuffer, kStrLen(vcTempBuffer));

      // Print Data
      kSPrintf(vcTempBuffer, "Data0 = 0x%Q, Data1 = 0x%Q, Data2 = 0x%Q",
        stReceivedEvent.vqwData[0], stReceivedEvent.vqwData[1],
        stReceivedEvent.vqwData[2]);
      kDrawText(qwWindowID, 20, iY + 40, WINDOW_COLOR_WHITE,
        WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

      break;
    }

    kShowWindow(qwWindowID, TRUE);
  }
}

/*
  System Monitor GUI Task
  Draw Processor, Memory Usage 
*/
void kSystemMonitorTask(void)
{
  QWORD qwWindowID;
  int i;
  int iWindowWidth;
  int iProcessorCount;
  DWORD vdwLastCPULoad[MAXPROCESSORCOUNT];
  int viLastTaskCount[MAXPROCESSORCOUNT];
  QWORD qwLastTickCount;
  EVENT stReceivedEvent;
  WINDOWEVENT* pstWindowEvent;
  BOOL bChanged;
  RECT stScreenArea;
  QWORD qwLastDynamicMemoryUsedSize;
  QWORD qwDynamicMemoryUsedSize;
  QWORD qwTemp;

  // Check GUI Enabled
  if (kIsGraphicMode() == FALSE) {
    kPrintf("This task can run only GUI Mode...\n");
    return;
  }

  kGetScreenArea(&stScreenArea);

  // Calc window size (Proceesor usage Count)
  iProcessorCount = kGetProcessorCount();
  iWindowWidth = iProcessorCount * (SYSTEMMONITOR_PROCESSOR_WIDTH +
    SYSTEMMONITOR_PROCESSOR_MARGIN) + SYSTEMMONITOR_PROCESSOR_MARGIN;

  // Create Empty Window
  qwWindowID = kCreateWindow((stScreenArea.iX2 - iWindowWidth) / 2,
    (stScreenArea.iY2 - SYSTEMMONITOR_WINDOW_HEIGHT) / 2,
    iWindowWidth, SYSTEMMONITOR_WINDOW_HEIGHT, WINDOW_FLAGS_DEFAULT &
    ~WINDOW_FLAGS_SHOW, "System Monitor");
  if (qwWindowID == WINDOW_INVALIDID)
  {
    return;
  }

  // Draw Processor Area
  kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + 15, iWindowWidth - 5,
    WINDOW_TITLEBAR_HEIGHT + 15, WINDOW_COLOR_WHITE);
  kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + 16, iWindowWidth - 5,
    WINDOW_TITLEBAR_HEIGHT + 16, WINDOW_COLOR_WHITE);
  kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + 17, iWindowWidth - 5,
    WINDOW_TITLEBAR_HEIGHT + 17, WINDOW_COLOR_WHITE);
  kDrawText(qwWindowID, 9, WINDOW_TITLEBAR_HEIGHT + 8, WINDOW_COLOR_WHITE,
    WINDOW_COLOR_BACKGROUND, "Processor Information", 21);


  // Draw Memory Area
  kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 50,
    iWindowWidth - 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 50,
    WINDOW_COLOR_WHITE);
  kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 51,
    iWindowWidth - 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 51,
    WINDOW_COLOR_WHITE);
  kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 52,
    iWindowWidth - 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 52,
    WINDOW_COLOR_WHITE);
  kDrawText(qwWindowID, 9, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 43,
    WINDOW_COLOR_WHITE, WINDOW_COLOR_BACKGROUND, "Memory Information", 18);

  // Show Window
  kShowWindow(qwWindowID, TRUE);

  qwLastTickCount = 0;
  kMemSet(vdwLastCPULoad, 0, sizeof(vdwLastCPULoad));
  kMemSet(viLastTaskCount, 0, sizeof(viLastTaskCount));
  qwLastDynamicMemoryUsedSize = 0;

  while (1)
  {
    if (kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == TRUE)
    {
      switch (stReceivedEvent.qwType)
      {
      case EVENT_WINDOW_CLOSE:
        kDeleteWindow(qwWindowID);
        return;
        break;

      default:
        break;
      }
    }

    // Check Period : 500ms
    if ((kGetTickCount() - qwLastTickCount) < 500)
    {
      kSleep(1);
      continue;
    }

    qwLastTickCount = kGetTickCount();

    // Proccesor Usage
    for (i = 0; i < iProcessorCount; i++)
    {
      bChanged = FALSE;

      // Processor Load
      if (vdwLastCPULoad[i] != kGetProcessorLoad(i))
      {
        vdwLastCPULoad[i] = kGetProcessorLoad(i);
        bChanged = TRUE;
      }
      // Task Count
      else if (viLastTaskCount[i] != kGetTaskCount(i))
      {
        viLastTaskCount[i] = kGetTaskCount(i);
        bChanged = TRUE;
      }

      // if changed, update
      if (bChanged == TRUE)
      {
        kDrawProcessorInformation(qwWindowID, i * SYSTEMMONITOR_PROCESSOR_WIDTH +
          (i + 1) * SYSTEMMONITOR_PROCESSOR_MARGIN, WINDOW_TITLEBAR_HEIGHT + 28,
          i);
      }
    }

    // Dynamic Mem
    kGetDynamicMemoryInformation(&qwTemp, &qwTemp, &qwTemp,
      &qwDynamicMemoryUsedSize);

    if (qwDynamicMemoryUsedSize != qwLastDynamicMemoryUsedSize)
    {
      qwLastDynamicMemoryUsedSize = qwDynamicMemoryUsedSize;

      kDrawMemoryInformation(qwWindowID, WINDOW_TITLEBAR_HEIGHT +
        SYSTEMMONITOR_PROCESSOR_HEIGHT + 60, iWindowWidth);
    }
  }
}

/*
  Draw Processor Infomation (System Monitor)
*/
static void kDrawProcessorInformation(QWORD qwWindowID, int iX, int iY,
  BYTE bAPICID)
{
  char vcBuffer[100];
  RECT stArea;
  QWORD qwProcessorLoad;
  QWORD iUsageBarHeight;
  int iMiddleX;

  // Draw Processor ID
  kSPrintf(vcBuffer, "Processor ID: %d", bAPICID);
  kDrawText(qwWindowID, iX + 10, iY, WINDOW_COLOR_WHITE, WINDOW_COLOR_BACKGROUND,
    vcBuffer, kStrLen(vcBuffer));

  // Draw Task Count
  kSPrintf(vcBuffer, "Task Count: %d   ", kGetTaskCount(bAPICID));
  kDrawText(qwWindowID, iX + 10, iY + 18, WINDOW_COLOR_WHITE, WINDOW_COLOR_BACKGROUND,
    vcBuffer, kStrLen(vcBuffer));

  // Calc Processor Load
  qwProcessorLoad = kGetProcessorLoad(bAPICID);
  if (qwProcessorLoad > 100)
  {
    qwProcessorLoad = 100;
  }

  // Draw Bar Graph Frame
  kDrawRect(qwWindowID, iX, iY + 36, iX + SYSTEMMONITOR_PROCESSOR_WIDTH,
    iY + SYSTEMMONITOR_PROCESSOR_HEIGHT, WINDOW_COLOR_WHITE, FALSE);

  // Calc Processor Usage (%)
  iUsageBarHeight = (SYSTEMMONITOR_PROCESSOR_HEIGHT - 40) * qwProcessorLoad / 100;

  // Fill Bar Graph
  kDrawRect(qwWindowID, iX + 2,
    iY + (SYSTEMMONITOR_PROCESSOR_HEIGHT - iUsageBarHeight) - 2,
    iX + SYSTEMMONITOR_PROCESSOR_WIDTH - 2,
    iY + SYSTEMMONITOR_PROCESSOR_HEIGHT - 2, SYSTEMMONITOR_BAR_COLOR, TRUE);
  kDrawRect(qwWindowID, iX + 2, iY + 38, iX + SYSTEMMONITOR_PROCESSOR_WIDTH - 2,
    iY + (SYSTEMMONITOR_PROCESSOR_HEIGHT - iUsageBarHeight) - 1,
    WINDOW_COLOR_BACKGROUND, TRUE);

  // Draw Usage in bar graph
  kSPrintf(vcBuffer, "Usage: %d%%", qwProcessorLoad);
  iMiddleX = (SYSTEMMONITOR_PROCESSOR_WIDTH -
    (kStrLen(vcBuffer) * FONT_ENGLISHWIDTH)) / 2;
  kDrawText(qwWindowID, iX + iMiddleX, iY + 80, WINDOW_COLOR_WHITE,
    WINDOW_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));

  // Update screen
  kSetRectangleData(iX, iY, iX + SYSTEMMONITOR_PROCESSOR_WIDTH,
    iY + SYSTEMMONITOR_PROCESSOR_HEIGHT, &stArea);
  kUpdateScreenByWindowArea(qwWindowID, &stArea);
}

/*
  Draw Memory Infomation (System Monitor)
*/
static void kDrawMemoryInformation(QWORD qwWindowID, int iY, int iWindowWidth)
{
  char vcBuffer[100];
  QWORD qwTotalRAMKbyteSize;
  QWORD qwDynamicMemoryStartAddress;
  QWORD qwDynamicMemoryUsedSize;
  QWORD qwUsedPercent;
  QWORD qwTemp;
  int iUsageBarWidth;
  RECT stArea;
  int iMiddleX;

  // Mbyte to Kbyte
  qwTotalRAMKbyteSize = kGetTotalRAMSize() * 1024;

  // Memory Info Text (Total)
  kSPrintf(vcBuffer, "Total Size: %d KB        ", qwTotalRAMKbyteSize);
  kDrawText(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 10, iY + 3, WINDOW_COLOR_WHITE,
    WINDOW_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));

  // Get Dynamic Memory
  kGetDynamicMemoryInformation(&qwDynamicMemoryStartAddress, &qwTemp,
    &qwTemp, &qwDynamicMemoryUsedSize);

  // Memory Info Text (Used)
  kSPrintf(vcBuffer, "Used Size: %d KB        ", (qwDynamicMemoryUsedSize +
    qwDynamicMemoryStartAddress) / 1024);
  kDrawText(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 10, iY + 21, WINDOW_COLOR_WHITE,
    WINDOW_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));

  // Draw bar graph Frame
  kDrawRect(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN, iY + 40,
    iWindowWidth - SYSTEMMONITOR_PROCESSOR_MARGIN,
    iY + SYSTEMMONITOR_MEMORY_HEIGHT - 32, WINDOW_COLOR_WHITE, FALSE);

  // Calc Memory Usage (%)
  qwUsedPercent = (qwDynamicMemoryStartAddress + qwDynamicMemoryUsedSize) *
    100 / 1024 / qwTotalRAMKbyteSize;
  if (qwUsedPercent > 100)
  {
    qwUsedPercent = 100;
  }

  // Calc bar graph
  iUsageBarWidth = (iWindowWidth - 2 * SYSTEMMONITOR_PROCESSOR_MARGIN) *
    qwUsedPercent / 100;

  // Fill bar graph
  kDrawRect(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 2, iY + 42,
    SYSTEMMONITOR_PROCESSOR_MARGIN + 2 + iUsageBarWidth,
    iY + SYSTEMMONITOR_MEMORY_HEIGHT - 34, SYSTEMMONITOR_BAR_COLOR, TRUE);
  kDrawRect(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 2 + iUsageBarWidth,
    iY + 42, iWindowWidth - SYSTEMMONITOR_PROCESSOR_MARGIN - 2,
    iY + SYSTEMMONITOR_MEMORY_HEIGHT - 34, WINDOW_COLOR_BACKGROUND, TRUE);

  // Draw Memory Usage in bar graph
  kSPrintf(vcBuffer, "Usage: %d%%", qwUsedPercent);
  iMiddleX = (iWindowWidth - (kStrLen(vcBuffer) * FONT_ENGLISHWIDTH)) / 2;
  kDrawText(qwWindowID, iMiddleX, iY + 45, WINDOW_COLOR_WHITE, WINDOW_COLOR_BACKGROUND,
    vcBuffer, kStrLen(vcBuffer));

  // Update screen
  kSetRectangleData(0, iY, iWindowWidth, iY + SYSTEMMONITOR_MEMORY_HEIGHT, &stArea);
  kUpdateScreenByWindowArea(qwWindowID, &stArea);
}

/*
  GUI Console Shell Task
*/
void kGUIConsoleShellTask(void)
{
  static QWORD s_qwWindowID = WINDOW_INVALIDID;
  int iWindowWidth, iWindowHeight;
  EVENT stReceivedEvent;
  KEYEVENT* pstKeyEvent;
  RECT stScreenArea;
  KEYDATA stKeyData;
  TCB* pstConsoleShellTask;
  QWORD qwConsoleShellTaskID;

  // Check GUI Enabled
  if (kIsGraphicMode() == FALSE) {
    kPrintf("This task can run only GUI Mode...\n");
    return;
  }

  // if shell is exist
  if (s_qwWindowID != WINDOW_INVALIDID) {
    kMoveWindowToTop(s_qwWindowID);
    return;
  }

  kGetScreenArea(&stScreenArea);

  iWindowWidth = CONSOLE_WIDTH * FONT_ENGLISHWIDTH + 4;
  iWindowHeight = CONSOLE_HEIGHT * FONT_ENGLISHHEIGHT + WINDOW_TITLEBAR_HEIGHT + 2;

  // Create Console Shell Window
  s_qwWindowID = kCreateWindow((stScreenArea.iX2 - iWindowWidth) / 2,
    (stScreenArea.iY2 - iWindowHeight) / 2, iWindowWidth, iWindowHeight,
    WINDOW_FLAGS_DEFAULT, "Console Shell for GUI");
  if (s_qwWindowID == WINDOW_INVALIDID) {
    return;
  }

  // Create Console Shell Task
  kSetConsoleShellExitFlag(FALSE);
  pstConsoleShellTask = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0,
    (QWORD)kStartConsoleShell, TASK_LOADBALANCINGID);
  if (pstConsoleShellTask == NULL) {
    kDeleteWindow(s_qwWindowID);
    return;
  }
  qwConsoleShellTaskID = pstConsoleShellTask->stLink.qwID;

  // Buffer clear
  kMemSet(gs_vstPreviousScreenBuffer, 0xFF, sizeof(gs_vstPreviousScreenBuffer));

  while (1) {
    kProcessConsoleBuffer(s_qwWindowID);

    if (kReceiveEventFromWindowQueue(s_qwWindowID, &stReceivedEvent) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stReceivedEvent.qwType)
    {
    case EVENT_KEY_DOWN:
    case EVENT_KEY_UP:
      pstKeyEvent = &(stReceivedEvent.stKeyEvent);
      stKeyData.bASCIICode = pstKeyEvent->bASCIICode;
      stKeyData.bFlags = pstKeyEvent->bFlags;
      stKeyData.bScanCode = pstKeyEvent->bScanCode;

      kPutKeyToGUIKeyQueue(&stKeyData);
      break;

    case EVENT_WINDOW_CLOSE:

      // Wait Shell Task is closed
      kSetConsoleShellExitFlag(TRUE);
      while (kIsTaskExist(qwConsoleShellTaskID) == TRUE)
      {
        kSleep(1);
      }

      kDeleteWindow(s_qwWindowID);
      s_qwWindowID = WINDOW_INVALIDID;
      return;
      break;

    default:
      break;
    }
  }
}

/*
  Process Console buffer
*/
static void kProcessConsoleBuffer(QWORD qwWindowID)
{
  int i;
  int j;
  CONSOLEMANAGER* pstManager;
  CHARACTER* pstScreenBuffer;
  CHARACTER* pstPreviousScreenBuffer;
  RECT stLineArea;
  BOOL bChanged;
  static QWORD s_qwLastTickCount = 0;
  BOOL bFullRedraw;

  pstManager = kGetConsoleManager();
  pstScreenBuffer = pstManager->pstScreenBuffer;
  pstPreviousScreenBuffer = gs_vstPreviousScreenBuffer;

  if (kGetTickCount() - s_qwLastTickCount > 5000) {
    s_qwLastTickCount = kGetTickCount();
    bFullRedraw = TRUE;
  }
  else {
    bFullRedraw = FALSE;
  }

  for (j = 0; j < CONSOLE_HEIGHT; j++) {
    bChanged = FALSE;

    // Check Line Change
    for (i = 0; i < CONSOLE_WIDTH; i++) {
      if ((pstScreenBuffer->bCharactor != pstPreviousScreenBuffer->bCharactor) ||
        (bFullRedraw == TRUE)) {
        kDrawText(qwWindowID, i * FONT_ENGLISHWIDTH + 2,
          j * FONT_ENGLISHHEIGHT + WINDOW_TITLEBAR_HEIGHT,
          WINDOW_COLOR_WHITE, WINDOW_COLOR_BACKGROUND,
          &(pstScreenBuffer->bCharactor), 1);

        kMemCpy(pstPreviousScreenBuffer, pstScreenBuffer, sizeof(CHARACTER));
        bChanged = TRUE;
      }

      pstScreenBuffer++;
      pstPreviousScreenBuffer++;
    }

    // Send Update Event
    if (bChanged == TRUE)
    {
      kSetRectangleData(2, j * FONT_ENGLISHHEIGHT + WINDOW_TITLEBAR_HEIGHT,
        5 + FONT_ENGLISHWIDTH * CONSOLE_WIDTH,
        (j + 1) * FONT_ENGLISHHEIGHT + WINDOW_TITLEBAR_HEIGHT - 1,
        &stLineArea);
      kUpdateScreenByWindowArea(qwWindowID, &stLineArea);
    }
  }
}