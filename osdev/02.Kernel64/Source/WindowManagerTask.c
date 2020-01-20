#include "Window.h"
#include "WindowManagerTask.h"
#include "VBE.h"
#include "Mouse.h"
#include "Task.h"
#include "GUITask.h"
#include "ApplicationPanelTask.h"

/*
  Window Manager Task
*/
void kStartWindowManager(void)
{
  int iMouseX, iMouseY;
  BOOL bMouseDataResult;
  BOOL bKeyDataResult;
  BOOL bEventQueueResult;
  WINDOWMANAGER* pstWindowManager;

  // Init GUI
  kInitializeGUISystem();

  // Draw Mouse
  kGetCursorPosition(&iMouseX, &iMouseY);
  kMoveCursor(iMouseX, iMouseY);

  // Create Application Panel
  kCreateTask(TASK_FLAGS_SYSTEM | TASK_FLAGS_THREAD | TASK_FLAGS_LOW,
    0, 0, (QWORD)kApplicationPanelGUITask, TASK_LOADBALANCINGID);

  pstWindowManager = kGetWindowManager();

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

    if ((bEventQueueResult == TRUE) &&
      (pstWindowManager->bWindowResizeMode == TRUE)) {
      kDrawResizeMarker(&(pstWindowManager->stResizingWindowArea), TRUE);
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
  int i;
  int iWidth, iHeight;

  pstWindowManager = kGetWindowManager();

  // Accumulate Mouse Event
  for (i = 0; i < WINDOWMANAGER_DATAACCUMULATECOUNT; i++)
  {
    // Wait Event
    if (kGetMouseDataFromMouseQueue(&bButtonStatus, &iRelativeX, &iRelativeY) ==
      FALSE)
    {
      if (i == 0)
      {
        return FALSE;
      }
      else
      {
        break;
      }
    }

    kGetCursorPosition(&iMouseX, &iMouseY);

    if (i == 0)
    {
      iPreviousMouseX = iMouseX;
      iPreviousMouseY = iMouseY;
    }

    iMouseX += iRelativeX;
    iMouseY += iRelativeY;

    kMoveCursor(iMouseX, iMouseY);
    kGetCursorPosition(&iMouseX, &iMouseY);

    // if button Changed, Stop accumulate and Process event
    bChangedButton = pstWindowManager->bPreviousButtonStatus ^ bButtonStatus;
    if (bChangedButton != 0)
    {
      break;
    }
  }

  qwWindowIDUnderMouse = kFindWindowByPoint(iMouseX, iMouseY);

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
        // Resize Button
        else if (kIsInResizeButton(qwWindowIDUnderMouse, iMouseX, iMouseY) == TRUE)
        {
          // Set Resize Info
          pstWindowManager->bWindowResizeMode = TRUE;
          pstWindowManager->qwResizingWindowID = qwWindowIDUnderMouse;
          kGetWindowArea(qwWindowIDUnderMouse, &(pstWindowManager->stResizingWindowArea));
          // Draw Marker
          kDrawResizeMarker(&(pstWindowManager->stResizingWindowArea), TRUE);
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
      // is moving
      if (pstWindowManager->bWindowMoveMode == TRUE) {
        pstWindowManager->bWindowMoveMode = FALSE;
        pstWindowManager->qwMovingWindowID = WINDOW_INVALIDID;
      }
      // is resizing
      else if (pstWindowManager->bWindowResizeMode == TRUE) {
        // Resize Window
        iWidth = kGetRectangleWidth(&(pstWindowManager->stResizingWindowArea));
        iHeight = kGetRectangleHeight(&(pstWindowManager->stResizingWindowArea));
        kResizeWindow(pstWindowManager->qwResizingWindowID,
          pstWindowManager->stResizingWindowArea.iX1,
          pstWindowManager->stResizingWindowArea.iY1,
          iWidth, iHeight);

        // Remove Marker
        kDrawResizeMarker(&(pstWindowManager->stResizingWindowArea), FALSE);

        kSetWindowEvent(pstWindowManager->qwResizingWindowID, EVENT_WINDOW_RESIZE, &stEvent);
        kSendEventToWindow(pstWindowManager->qwResizingWindowID, &stEvent);

        // Reset Resize Info
        pstWindowManager->bWindowResizeMode = FALSE;
        pstWindowManager->qwResizingWindowID = WINDOW_INVALIDID;
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
  // Process window Resize Move
  else if (pstWindowManager->bWindowResizeMode == TRUE) {
    // Remove Previous Marker
    kDrawResizeMarker(&(pstWindowManager->stResizingWindowArea), FALSE);

    // Update Resizing Window Area Info
    pstWindowManager->stResizingWindowArea.iX2 += iMouseX - iPreviousMouseX;
    pstWindowManager->stResizingWindowArea.iY1 += iMouseY - iPreviousMouseY;

    // Check Area
    if ((pstWindowManager->stResizingWindowArea.iX2 < pstWindowManager->stResizingWindowArea.iX1) ||
      (kGetRectangleWidth(&(pstWindowManager->stResizingWindowArea)) < WINDOW_WIDTH_MIN)) {
      pstWindowManager->stResizingWindowArea.iX2 = pstWindowManager->stResizingWindowArea.iX1 + WINDOW_WIDTH_MIN - 1;
    }
    if ((pstWindowManager->stResizingWindowArea.iY2 < pstWindowManager->stResizingWindowArea.iY1) ||
      (kGetRectangleHeight(&(pstWindowManager->stResizingWindowArea)) < WINDOW_HEIGHT_MIN)) {
      pstWindowManager->stResizingWindowArea.iY1 = pstWindowManager->stResizingWindowArea.iY2 - WINDOW_HEIGHT_MIN - 1;
    }

    // Draw New Marker
    kDrawResizeMarker(&(pstWindowManager->stResizingWindowArea), TRUE);
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
  EVENT vstEvent[WINDOWMANAGER_DATAACCUMULATECOUNT];
  int iEventCount;
  WINDOWEVENT* pstWindowEvent;
  WINDOWEVENT* pstNextWindowEvent;
  QWORD qwWindowID;
  RECT stArea;
  RECT stOverlappedArea;
  int i;
  int j;

  // Accumulate Event
  for (i = 0; i < WINDOWMANAGER_DATAACCUMULATECOUNT; i++)
  {
    if (kReceiveEventFromWindowManagerQueue(&(vstEvent[i])) == FALSE)
    {
      if (i == 0)
      {
        return FALSE;
      }
      else
      {
        break;
      }
    }

    pstWindowEvent = &(vstEvent[i].stWindowEvent);
    // UPDATESCREENBYID
    if (vstEvent[i].qwType == EVENT_WINDOWMANAGER_UPDATESCREENBYID)
    {
      // Set Update Area
      if (kGetWindowArea(pstWindowEvent->qwWindowID, &stArea) == FALSE)
      {
        kSetRectangleData(0, 0, 0, 0, &(pstWindowEvent->stArea));
      }
      else
      {
        kSetRectangleData(0, 0, kGetRectangleWidth(&stArea) - 1,
          kGetRectangleHeight(&stArea) - 1, &(pstWindowEvent->stArea));
      }
    }
  }

  iEventCount = i;
  for (j = 0; j < iEventCount; j++)
  {
    pstWindowEvent = &(vstEvent[j].stWindowEvent);
    if ((vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYID) &&
      (vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA) &&
      (vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA))
    {
      continue;
    }

    for (i = j + 1; i < iEventCount; i++)
    {
      pstNextWindowEvent = &(vstEvent[i].stWindowEvent);
      if (((vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYID) &&
        (vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA) &&
        (vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA)) ||
        (pstWindowEvent->qwWindowID != pstNextWindowEvent->qwWindowID))
      {
        continue;
      }

      if (kGetOverlappedRectangle(&(pstWindowEvent->stArea),
        &(pstNextWindowEvent->stArea), &stOverlappedArea) == FALSE)
      {
        continue;
      }

      // if a Event Contain another Event Area 
      if (kMemCmp(&(pstWindowEvent->stArea), &stOverlappedArea,
        sizeof(RECT)) == 0)
      {
        kMemCpy(&(pstWindowEvent->stArea), &(pstNextWindowEvent->stArea),
          sizeof(RECT));
        vstEvent[i].qwType = EVENT_UNKNOWN;
      }
      else if (kMemCmp(&(pstNextWindowEvent->stArea), &stOverlappedArea,
        sizeof(RECT)) == 0)
      {
        vstEvent[i].qwType = EVENT_UNKNOWN;
      }
    }
  }

  // Process Event
  for (i = 0; i < iEventCount; i++)
  {
    pstWindowEvent = &(vstEvent[i].stWindowEvent);

    switch (vstEvent[i].qwType)
    {
    case EVENT_WINDOWMANAGER_UPDATESCREENBYID:
    case EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA:
      if (kConvertRectClientToScreen(pstWindowEvent->qwWindowID,
        &(pstWindowEvent->stArea), &stArea) == TRUE)
      {
        kRedrawWindowByArea(&stArea, pstWindowEvent->qwWindowID);
      }
      break;

    case EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA:
      kRedrawWindowByArea(&(pstWindowEvent->stArea), WINDOW_INVALIDID);
      break;

    default:
      break;
    }
  }

  return TRUE;
}

/*
  Draw Resize Marker
*/
void kDrawResizeMarker(const RECT* pstArea, BOOL bShowMarker)
{
  RECT stMarkerArea;
  WINDOWMANAGER* pstWindowManager;

  pstWindowManager = kGetWindowManager();

  // Draw Marker
  if (bShowMarker == TRUE)
  {
    // Draw Left Upper
    kSetRectangleData(pstArea->iX1, pstArea->iY1,
      pstArea->iX1 + WINDOWMANAGER_RESIZEMARKERSIZE,
      pstArea->iY1 + WINDOWMANAGER_RESIZEMARKERSIZE, &stMarkerArea);
    kInternalDrawRect(&(pstWindowManager->stScreenArea),
      pstWindowManager->pstVideoMemory, stMarkerArea.iX1, stMarkerArea.iY1,
      stMarkerArea.iX2, stMarkerArea.iY1 + WINDOWMANAGER_THICK_RESIZEMARKER,
      WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);
    kInternalDrawRect(&(pstWindowManager->stScreenArea),
      pstWindowManager->pstVideoMemory, stMarkerArea.iX1, stMarkerArea.iY1,
      stMarkerArea.iX1 + WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iY2,
      WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);

    // Draw Right Upper
    kSetRectangleData(pstArea->iX2 - WINDOWMANAGER_RESIZEMARKERSIZE,
      pstArea->iY1, pstArea->iX2, pstArea->iY1 + WINDOWMANAGER_RESIZEMARKERSIZE,
      &stMarkerArea);
    kInternalDrawRect(&(pstWindowManager->stScreenArea),
      pstWindowManager->pstVideoMemory, stMarkerArea.iX1, stMarkerArea.iY1,
      stMarkerArea.iX2, stMarkerArea.iY1 + WINDOWMANAGER_THICK_RESIZEMARKER,
      WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);
    kInternalDrawRect(&(pstWindowManager->stScreenArea),
      pstWindowManager->pstVideoMemory, stMarkerArea.iX2 -
      WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iY1,
      stMarkerArea.iX2, stMarkerArea.iY2, WINDOWMANAGER_COLOR_RESIZEMARKER,
      TRUE);

    // Draw Left Lower
    kSetRectangleData(pstArea->iX1, pstArea->iY2 - WINDOWMANAGER_RESIZEMARKERSIZE,
      pstArea->iX1 + WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iY2, &stMarkerArea);
    kInternalDrawRect(&(pstWindowManager->stScreenArea),
      pstWindowManager->pstVideoMemory, stMarkerArea.iX1, stMarkerArea.iY2 -
      WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iX2, stMarkerArea.iY2,
      WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);
    kInternalDrawRect(&(pstWindowManager->stScreenArea),
      pstWindowManager->pstVideoMemory, stMarkerArea.iX1, stMarkerArea.iY1,
      stMarkerArea.iX1 + WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iY2,
      WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);

    // Draw Right Lower
    kSetRectangleData(pstArea->iX2 - WINDOWMANAGER_RESIZEMARKERSIZE,
      pstArea->iY2 - WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iX2, pstArea->iY2,
      &stMarkerArea);
    kInternalDrawRect(&(pstWindowManager->stScreenArea),
      pstWindowManager->pstVideoMemory, stMarkerArea.iX1, stMarkerArea.iY2 -
      WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iX2, stMarkerArea.iY2,
      WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);
    kInternalDrawRect(&(pstWindowManager->stScreenArea),
      pstWindowManager->pstVideoMemory, stMarkerArea.iX2 -
      WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iY1, stMarkerArea.iX2,
      stMarkerArea.iY2, WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);
  }

  // Remove Marker
  else
  {
    // Draw Left Upper
    kSetRectangleData(pstArea->iX1, pstArea->iY1,
      pstArea->iX1 + WINDOWMANAGER_RESIZEMARKERSIZE,
      pstArea->iY1 + WINDOWMANAGER_RESIZEMARKERSIZE, &stMarkerArea);
    kRedrawWindowByArea(&stMarkerArea, WINDOW_INVALIDID);

    // Draw Right Upper
    kSetRectangleData(pstArea->iX2 - WINDOWMANAGER_RESIZEMARKERSIZE,
      pstArea->iY1, pstArea->iX2, pstArea->iY1 + WINDOWMANAGER_RESIZEMARKERSIZE,
      &stMarkerArea);
    kRedrawWindowByArea(&stMarkerArea, WINDOW_INVALIDID);

    // Draw Left Lower
    kSetRectangleData(pstArea->iX1, pstArea->iY2 - WINDOWMANAGER_RESIZEMARKERSIZE,
      pstArea->iX1 + WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iY2, &stMarkerArea);
    kRedrawWindowByArea(&stMarkerArea, WINDOW_INVALIDID);

    // Draw Left Lower
    kSetRectangleData(pstArea->iX2 - WINDOWMANAGER_RESIZEMARKERSIZE,
      pstArea->iY2 - WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iX2, pstArea->iY2,
      &stMarkerArea);
    kRedrawWindowByArea(&stMarkerArea, WINDOW_INVALIDID);
  }
}