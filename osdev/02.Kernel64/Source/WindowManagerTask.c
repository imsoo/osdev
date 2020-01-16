#include "Types.h"
#include "Window.h"
#include "WindowManagerTask.h"
#include "VBE.h"
#include "Mouse.h"
#include "Task.h"
#include "GUITask.h"

/*
  Window Manager Task
*/
void kStartWindowManager(void)
{
  int iMouseX, iMouseY;
  BOOL bMouseDataResult;
  BOOL bKeyDataResult;
  BOOL bEventQueueResult;

  // Init GUI
  kInitializeGUISystem();

  // Draw Mouse
  kGetCursorPosition(&iMouseX, &iMouseY);
  kMoveCursor(iMouseX, iMouseY);

  while (1)
  {
    // Mouse
    bMouseDataResult = kProcessMouseData();

    // Key
    bKeyDataResult = kProcessKeyData();

    // Event
    bEventQueueResult = FALSE;
    while (kProcessEventQueueData() == TRUE)
    {
      bEventQueueResult = TRUE;
    }

    if ((bMouseDataResult == FALSE) &&
      (bKeyDataResult == FALSE) &&
      (bEventQueueResult == FALSE))
    {
      kSleep(0);
    }
  }
}

/*
  Process Mouse Packet
*/
BOOL kProcessMouseData(void)
{
  QWORD qwWindowIDUnderMouse;
  BYTE bButtonStatus;
  int iRelativeX, iRelativeY;
  int iMouseX, iMouseY;
  int iPreviousMouseX, iPreviousMouseY;
  BYTE bChangedButton;
  RECT stWindowArea;
  EVENT stEvent;
  WINDOWMANAGER* pstWindowManager;
  char vcTempTitle[WINDOW_TITLEMAXLENGTH];
  static int iWindowCount = 0;
  QWORD qwWindowID;

  if (kGetMouseDataFromMouseQueue(&bButtonStatus, &iRelativeX, &iRelativeY) == FALSE) {
    return FALSE;
  }

  pstWindowManager = kGetWindowManager();

  // Update Cursor
  kGetCursorPosition(&iMouseX, &iMouseY);

  iPreviousMouseX = iMouseX;
  iPreviousMouseY = iMouseY;
  
  iMouseX += iRelativeX;
  iMouseY += iRelativeY;
  kMoveCursor(iMouseX, iMouseY);
  kGetCursorPosition(&iMouseX, &iMouseY);

  qwWindowIDUnderMouse = kFindWindowByPoint(iMouseX, iMouseY);

  bChangedButton = pstWindowManager->bPreviousButtonStatus ^ bButtonStatus;

  if (bChangedButton & MOUSE_LBUTTONDOWN) {

    // L Button Down
    if (bButtonStatus & MOUSE_LBUTTONDOWN) {

      // Move to Top
      if (qwWindowIDUnderMouse != pstWindowManager->qwBackgroundWindowID)
      {
        kMoveWindowToTop(qwWindowIDUnderMouse);
      }

      // if in Title Bar
      if (kIsInTitleBar(qwWindowIDUnderMouse, iMouseX, iMouseY) == TRUE)
      {
        // Close button
        if (kIsInCloseButton(qwWindowIDUnderMouse, iMouseX, iMouseY) == TRUE)
        {
          // Send Close Event
          kSetWindowEvent(qwWindowIDUnderMouse, EVENT_WINDOW_CLOSE,
            &stEvent);
          kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
        }
        // Move
        else
        {
          pstWindowManager->bWindowMoveMode = TRUE;
          pstWindowManager->qwMovingWindowID = qwWindowIDUnderMouse;
        }
      }
      // In Window Frame
      else {
        kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_LBUTTONDOWN,
          iMouseX, iMouseY, bButtonStatus, &stEvent);
        kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
      }
    }
    // L Button Up
    else {
      if (pstWindowManager->bWindowMoveMode == TRUE) {
        pstWindowManager->bWindowMoveMode = FALSE;
        pstWindowManager->qwMovingWindowID = WINDOW_INVALIDID;
      }
      else {
        kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_LBUTTONUP,
          iMouseX, iMouseY, bButtonStatus, &stEvent);
        kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
      }
    }

  }
  else if (bChangedButton & MOUSE_RBUTTONDOWN) {

    // R Button Down
    if (bButtonStatus & MOUSE_RBUTTONDOWN) {
      kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_RBUTTONDOWN,
        iMouseX, iMouseY, bButtonStatus, &stEvent);
      kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);

      // Temp
      kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, NULL, NULL, (QWORD)kHelloWorldGUITask, TASK_LOADBALANCINGID);
    }
    // R Button Up
    else {
      kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_RBUTTONUP,
        iMouseX, iMouseY, bButtonStatus, &stEvent);
      kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
    }

  }
  else if (bChangedButton & MOUSE_MBUTTONDOWN) {

    // M Button Down
    if (bButtonStatus & MOUSE_MBUTTONDOWN) {
      kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MBUTTONDOWN,
        iMouseX, iMouseY, bButtonStatus, &stEvent);
      kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
    }
    // M Button Up
    else {
      kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MBUTTONUP,
        iMouseX, iMouseY, bButtonStatus, &stEvent);
      kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
    }

  }
  // move
  else {
    kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MOVE,
      iMouseX, iMouseY, bButtonStatus, &stEvent);
    kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
  }

  // Process window Move 
  if (pstWindowManager->bWindowMoveMode == TRUE)
  {
    if (kGetWindowArea(pstWindowManager->qwMovingWindowID, &stWindowArea) == TRUE)
    {
      // Move Window
      kMoveWindow(pstWindowManager->qwMovingWindowID,
        stWindowArea.iX1 + iMouseX - iPreviousMouseX,
        stWindowArea.iY1 + iMouseY - iPreviousMouseY);
    }
    else
    {
      pstWindowManager->bWindowMoveMode = FALSE;
      pstWindowManager->qwMovingWindowID = WINDOW_INVALIDID;
    }
  }

  pstWindowManager->bPreviousButtonStatus = bButtonStatus;
  return TRUE;
}

/*
  Process Keyboard Data
*/
BOOL kProcessKeyData(void)
{
  KEYDATA stKeyData;
  EVENT stEvent;
  QWORD qwAcitveWindowID;

  if (kGetKeyFromKeyQueue(&stKeyData) == FALSE)
  {
    return FALSE;
  }

  // Send Key to Top Window
  qwAcitveWindowID = kGetTopWindowID();
  kSetKeyEvent(qwAcitveWindowID, &stKeyData, &stEvent);
  return kSendEventToWindow(qwAcitveWindowID, &stEvent);
}

/*
  Process Event
*/
BOOL kProcessEventQueueData(void)
{
  EVENT stEvent;
  WINDOWEVENT* pstWindowEvent;
  QWORD qwWindowID;
  RECT stArea;

  if (kReceiveEventFromWindowManagerQueue(&stEvent) == FALSE)
  {
    return FALSE;
  }

  pstWindowEvent = &(stEvent.stWindowEvent);

  switch (stEvent.qwType)
  {
  case EVENT_WINDOWMANAGER_UPDATESCREENBYID:
    if (kGetWindowArea(pstWindowEvent->qwWindowID, &stArea) == TRUE)
    {
      kRedrawWindowByArea(&stArea);
    }
    break;

  case EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA:
    if (kConvertRectClientToScreen(pstWindowEvent->qwWindowID,
      &(pstWindowEvent->stArea), &stArea) == TRUE)
    {
      kRedrawWindowByArea(&stArea);
    }
    break;

  case EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA:
    kRedrawWindowByArea(&(pstWindowEvent->stArea));
    break;

  default:
    break;
  }

  return TRUE;
}