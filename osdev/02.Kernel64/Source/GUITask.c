#include "GUITask.h"
#include "Utility.h"
#include "Window.h"

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