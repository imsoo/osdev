#include "Window.h"
#include "DynamicMemory.h"
#include "Task.h"
#include "VBE.h"
#include "Console.h"
#include "Font.h"
#include "Utility.h"
#include "JPEG.h"

static WINDOWPOOLMANAGER gs_stWindowPoolManager;
static WINDOWMANAGER gs_stWindowManager;

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

/*
  Init Window Pool Manager
*/
static void kInitializeWindowPool(void)
{
  int i;
  void* pvWindowPoolAddress;

  kMemSet(&gs_stWindowPoolManager, 0, sizeof(gs_stWindowPoolManager));

  // Allocate Pool Area
  pvWindowPoolAddress = (void*)kAllocateMemory(sizeof(WINDOW) * WINDOW_MAXCOUNT);
  if (pvWindowPoolAddress == NULL) {
    kPrintf("Window Pool Allocate Fail\n");
    while (1);
  }

  gs_stWindowPoolManager.pstStartAddress = (WINDOW*)pvWindowPoolAddress;
  kMemSet(pvWindowPoolAddress, 0, sizeof(WINDOW) * WINDOW_MAXCOUNT);

  // assign Window ID
  for (i = 0; i < WINDOW_MAXCOUNT; i++) {
    gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID = i;
  }

  gs_stWindowPoolManager.iMaxCount = WINDOW_MAXCOUNT;
  gs_stWindowPoolManager.iAllocatedCount = 1;

  // Init Mutex
  kInitializeMutex(&(gs_stWindowPoolManager.stLock));
}

/*
  Allocate Empty Window
*/
static WINDOW* kAllocateWindow(void)
{
  WINDOW* pstEmptyWindow;
  int i;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowPoolManager.stLock));

  // if All Window in Pool are allocated
  if (gs_stWindowPoolManager.iUseCount == gs_stWindowPoolManager.iMaxCount) {
    kUnlock(&(gs_stWindowPoolManager.stLock));
    // --- CRITCAL SECTION END ---
    return NULL;
  }

  // Find Empty Window
  for (i = 0; i < WINDOW_MAXCOUNT; i++) {
    if ((gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0) {
      pstEmptyWindow = &(gs_stWindowPoolManager.pstStartAddress[i]);
      break;
    }
  }
  pstEmptyWindow->stLink.qwID = ((QWORD)gs_stWindowPoolManager.iAllocatedCount << 32) | i;

  gs_stWindowPoolManager.iUseCount++;
  gs_stWindowPoolManager.iAllocatedCount++;
  if (gs_stWindowPoolManager.iAllocatedCount == 0) {
    gs_stWindowPoolManager.iAllocatedCount = 1;
  }

  kUnlock(&(gs_stWindowPoolManager.stLock));
  // --- CRITCAL SECTION END ---

  // Init Window Mutex
  kInitializeMutex(&(pstEmptyWindow->stLock));

  return pstEmptyWindow;
}

/*
  Free Window
*/
static void kFreeWindow(QWORD qwID)
{
  int i;

  i = GETWINDOWOFFSET(qwID);

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowPoolManager.stLock));

  // Clean Window
  kMemSet(&(gs_stWindowPoolManager.pstStartAddress[i]), 0, sizeof(WINDOW));
  gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID = i;

  gs_stWindowPoolManager.iUseCount--;

  kUnlock(&(gs_stWindowPoolManager.stLock));
  // --- CRITCAL SECTION END ---
}

void kInitializeGUISystem(void)
{
  VBEMODEINFOBLOCK* pstModeInfo;
  QWORD qwBackgroundWindowID;
  EVENT* pstEventBuffer;

  kInitializeWindowPool();

  // Set Video Memory Address
  pstModeInfo = kGetVBEModeInfoBlock();
  gs_stWindowManager.pstVideoMemory = (COLOR*)((QWORD)pstModeInfo->dwPhysicalBasePointer & 0xFFFFFFFF);

  // Set Mouse Cursor Position
  gs_stWindowManager.iMouseX = pstModeInfo->wXResolution / 2;
  gs_stWindowManager.iMouseY = pstModeInfo->wYResolution / 2;

  // Set Screen
  gs_stWindowManager.stScreenArea.iX1 = 0;
  gs_stWindowManager.stScreenArea.iY1 = 0;
  gs_stWindowManager.stScreenArea.iX2 = pstModeInfo->wXResolution - 1;
  gs_stWindowManager.stScreenArea.iY2 = pstModeInfo->wYResolution - 1;

  // Init Mutex
  kInitializeMutex(&(gs_stWindowManager.stLock));

  // Init Window List
  kInitializeList(&(gs_stWindowManager.stWindowList));

  // Init Event Queue
  pstEventBuffer = (EVENT*)kAllocateMemory(sizeof(EVENT) * EVENTQUEUE_WNIDOWMANAGERMAXCOUNT);
  if (pstEventBuffer == NULL) {
    kPrintf("Window Manager Event Queue Allocate Fail\n");
    while (1);
  }
  kInitializeQueue(&(gs_stWindowManager.stEventQueue), pstEventBuffer, EVENTQUEUE_WNIDOWMANAGERMAXCOUNT, sizeof(EVENT));

  // Init Draw bitmap
  gs_stWindowManager.pbDrawBitmap = kAllocateMemory((pstModeInfo->wXResolution * pstModeInfo->wYResolution + 7) / 8);
  if (gs_stWindowManager.pbDrawBitmap == NULL) {
    kPrintf("Window Manager Draw Bitmap Allocate Fail\n");
    while (1);
  }

  // Init Mouse Info
  gs_stWindowManager.bPreviousButtonStatus = 0;
  gs_stWindowManager.bWindowMoveMode = FALSE;
  gs_stWindowManager.qwMovingWindowID = WINDOW_INVALIDID;

  // Create Background Window
  qwBackgroundWindowID = kCreateWindow(0, 0, pstModeInfo->wXResolution, pstModeInfo->wYResolution,
    0, WINDOW_BACKGROUNDWINDOWTITLE);
  gs_stWindowManager.qwBackgroundWindowID = qwBackgroundWindowID;

  kDrawRect(qwBackgroundWindowID, 0, 0, pstModeInfo->wXResolution - 1, pstModeInfo->wYResolution - 1,
    WINDOW_COLOR_SYSTEMBACKGROUND, TRUE);

  kDrawBackgroundImage();

  kShowWindow(qwBackgroundWindowID, TRUE);
}

/*
  Get Window Manager
*/
WINDOWMANAGER* kGetWindowManager(void)
{
  return &gs_stWindowManager;
}

/*
  Get Backgroud Window ID
*/
QWORD kGetBackgroundWindowID(void)
{
  return gs_stWindowManager.qwBackgroundWindowID;
}

/*
  Get Screen Area Info
*/
void kGetScreenArea(RECT* pstScreenArea)
{
  kMemCpy(pstScreenArea, &(gs_stWindowManager.stScreenArea), sizeof(RECT));
}

/*
  Create Window
*/
QWORD kCreateWindow(int iX, int iY, int iWidth, int iHeight, DWORD dwFlags,
  const char* pcTitle)
{
  WINDOW* pstWindow;
  TCB* pstTask;
  QWORD qwActiveWindowID;
  EVENT stEvent;

  // Check Window size 
  if ((iWidth <= 0) || (iHeight <= 0))
  {
    return WINDOW_INVALIDID;
  }

  // Allocate Window
  pstWindow = kAllocateWindow();
  if (pstWindow == NULL)
  {
    return WINDOW_INVALIDID;
  }

  // Set Window Area
  pstWindow->stArea.iX1 = iX;
  pstWindow->stArea.iY1 = iY;
  pstWindow->stArea.iX2 = iX + iWidth - 1;
  pstWindow->stArea.iY2 = iY + iHeight - 1;

  // Set Window Title
  kMemCpy(pstWindow->vcWindowTitle, pcTitle, WINDOW_TITLEMAXLENGTH);
  pstWindow->vcWindowTitle[WINDOW_TITLEMAXLENGTH] = '\0';

  // Allocate Event Queue
  pstWindow->pstEventBuffer = (EVENT*)kAllocateMemory(EVENTQUEUE_WINDOWMAXCOUNT * sizeof(EVENT));

  // Allocate Window buffer
  pstWindow->pstWindowBuffer = (COLOR*)kAllocateMemory(iWidth * iHeight * sizeof(COLOR));

  if ((pstWindow->pstEventBuffer == NULL) ||
    (pstWindow->pstWindowBuffer == NULL))
  {
    kFreeMemory(pstWindow->pstEventBuffer);
    kFreeMemory(pstWindow->pstWindowBuffer);

    kFreeWindow(pstWindow->stLink.qwID);
    return WINDOW_INVALIDID;
  }

  // Inint Event Queue
  kInitializeQueue(&(pstWindow->stEventQueue), pstWindow->pstEventBuffer, EVENTQUEUE_WINDOWMAXCOUNT, sizeof(EVENT));

  // Set Task ID
  pstTask = kGetRunningTask(kGetAPICID());
  pstWindow->qwTaskID = pstTask->stLink.qwID;

  // Set Flag
  pstWindow->dwFlags = dwFlags;

  // Draw Background
  kDrawWindowBackground(pstWindow->stLink.qwID);

  // Draw Frame
  if (dwFlags & WINDOW_FLAGS_DRAWFRAME)
  {
    kDrawWindowFrame(pstWindow->stLink.qwID);
  }

  // Draw Title
  if (dwFlags & WINDOW_FLAGS_DRAWTITLE)
  {
    kDrawWindowTitle(pstWindow->stLink.qwID, pcTitle, TRUE);
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowManager.stLock));

  qwActiveWindowID = kGetTopWindowID();

  // Add Window List 
  kAddListToHeader(&gs_stWindowManager.stWindowList, pstWindow);

  kUnlock(&(gs_stWindowManager.stLock));
  // --- CRITCAL SECTION END ---

  // Send WINDOW Event
  kUpdateScreenByID(pstWindow->stLink.qwID);
  kSetWindowEvent(pstWindow->stLink.qwID, EVENT_WINDOW_SELECT, &stEvent);
  kSendEventToWindow(pstWindow->stLink.qwID, &stEvent);

  // Prev Window Deselect Event
  if (qwActiveWindowID != gs_stWindowManager.qwBackgroundWindowID)
  {
    kUpdateWindowTitle(qwActiveWindowID, FALSE);
    kSetWindowEvent(qwActiveWindowID, EVENT_WINDOW_DESELECT, &stEvent);
    kSendEventToWindow(qwActiveWindowID, &stEvent);
  }

  return pstWindow->stLink.qwID;
}


/*
  Delete Window Using windowID
*/
BOOL kDeleteWindow(QWORD qwWindowID)
{
  WINDOW* pstWindow;
  RECT stArea;
  QWORD qwActiveWindowID;
  BOOL bActiveWindow;
  EVENT stEvent;

  // --- CRITCAL SECTION BEGIN 1 ---
  kLock(&(gs_stWindowManager.stLock));

  // Find Window
  // --- CRITCAL SECTION BEGIN 2 ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    kUnlock(&(gs_stWindowManager.stLock));
    // --- CRITCAL SECTION END 1 ---
    // --- CRITCAL SECTION END 2 ---
    return FALSE;
  }

  // Store Window Area for Redraw
  kMemCpy(&stArea, &(pstWindow->stArea), sizeof(RECT));

  qwActiveWindowID = kGetTopWindowID();
  if (qwActiveWindowID == qwWindowID) {
    bActiveWindow = TRUE;
  }
  else {
    bActiveWindow = FALSE;
  }

  // Remove Window in Window List
  if (kRemoveList(&(gs_stWindowManager.stWindowList), qwWindowID) == NULL)
  {
    kUnlock(&(pstWindow->stLock));
    // --- CRITCAL SECTION END 2 ---

    kUnlock(&(gs_stWindowManager.stLock));
    // --- CRITCAL SECTION END 1 ---
    return FALSE;
  }

  // Free Window buffer
  kFreeMemory(pstWindow->pstWindowBuffer);
  pstWindow->pstWindowBuffer = NULL;

  // Free Window Event Queue
  kFreeMemory(pstWindow->pstEventBuffer);
  pstWindow->pstEventBuffer = NULL;

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END 2 ---

  // Free Window
  kFreeWindow(qwWindowID);

  kUnlock(&(gs_stWindowManager.stLock));
  // --- CRITCAL SECTION END 1 ---

  // Redraw 
  kUpdateScreenByScreenArea(&stArea);


  // if delete Top window, send WINDOW_SELECT_EVENT to next top WINDOW
  if (bActiveWindow == TRUE)
  {
    qwActiveWindowID = kGetTopWindowID();

    if (qwActiveWindowID != WINDOW_INVALIDID)
    {
      kUpdateWindowTitle(qwActiveWindowID, TRUE);

      kSetWindowEvent(qwActiveWindowID, EVENT_WINDOW_SELECT, &stEvent);
      kSendEventToWindow(qwActiveWindowID, &stEvent);
    }
  }
  return TRUE;
}

/*
  Remove Window using TaskID
*/
BOOL kDeleteAllWindowInTaskID(QWORD qwTaskID)
{
  WINDOW* pstWindow;
  WINDOW* pstNextWindow;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowManager.stLock));

  // Find List From Header
  pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
  while (pstWindow != NULL)
  {
    pstNextWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList),
      pstWindow);

    // if found and Window is not Background Window, delete it
    if ((pstWindow->stLink.qwID != gs_stWindowManager.qwBackgroundWindowID) &&
      (pstWindow->qwTaskID == qwTaskID))
    {
      kDeleteWindow(pstWindow->stLink.qwID);
    }

    pstWindow = pstNextWindow;
  }

  kUnlock(&(gs_stWindowManager.stLock));
  // --- CRITCAL SECTION END ---
}

/*
  Get Window Pointer using window ID
*/
WINDOW* kGetWindow(QWORD qwWindowID)
{
  WINDOW* pstWindow;

  // check Widow ID
  if (GETWINDOWOFFSET(qwWindowID) >= WINDOW_MAXCOUNT)
  {
    return NULL;
  }

  // Find window
  pstWindow = &gs_stWindowPoolManager.pstStartAddress[GETWINDOWOFFSET(qwWindowID)];
  if (pstWindow->stLink.qwID == qwWindowID)
  {
    return pstWindow;
  }

  return NULL;
}

/*
  Get Window Pointer using window ID and Lock Mutex
*/
WINDOW* kGetWindowWithWindowLock(QWORD qwWindowID)
{
  WINDOW* pstWindow;
  BOOL bResult;

  // Find window
  pstWindow = kGetWindow(qwWindowID);
  if (pstWindow == NULL)
  {
    return NULL;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(pstWindow->stLock));

  // double check
  pstWindow = kGetWindow(qwWindowID);
  if (pstWindow == NULL)
  {
    kUnlock(&(pstWindow->stLock));
    // --- CRITCAL SECTION END ---
    return NULL;
  }

  return pstWindow;
}

/*
  Set Window Show Flag and Redraw
*/
BOOL kShowWindow(QWORD qwWindowID, BOOL bShow)
{
  WINDOW* pstWindow;
  RECT stWindowArea;

  // Find Window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // Set Flag
  if (bShow == TRUE)
  {
    pstWindow->dwFlags |= WINDOW_FLAGS_SHOW;
  }
  else
  {
    pstWindow->dwFlags &= ~WINDOW_FLAGS_SHOW;
  }

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  // Redraw
  if (bShow == TRUE)
  {
    kUpdateScreenByID(qwWindowID);
  }
  else
  {
    kGetWindowArea(qwWindowID, &stWindowArea);
    kUpdateScreenByScreenArea(&stWindowArea);
  }
  return TRUE;
}

/*
  Redraw Area for WindowManager
*/
BOOL kRedrawWindowByArea(const RECT* pstArea, QWORD qwDrawWindowID)
{
  WINDOW* pstWindow;
  WINDOW* pstTargetWindow = NULL;
  RECT stOverlappedArea;
  RECT stCursorArea;
  DRAWBITMAP stDrawBitmap;
  RECT stTempOverlappedArea;
  RECT vstLargestOverlappedArea[WINDOW_OVERLAPPEDAREALOGMAXCOUNT];
  int viLargestOverlappedAreaSize[WINDOW_OVERLAPPEDAREALOGMAXCOUNT];
  int iTempOverlappedAreaSize;
  int iMinAreaSize;
  int iMinAreaIndex;
  int i;

  // Not in screen
  if (kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), pstArea, &stOverlappedArea) == FALSE) {
    return FALSE;
  }

  // Init History
  kMemSet(viLargestOverlappedAreaSize, 0, sizeof(viLargestOverlappedAreaSize));
  kMemSet(vstLargestOverlappedArea, 0, sizeof(vstLargestOverlappedArea));

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowManager.stLock));

  // Create Draw Bitmap
  kCreateDrawBitmap(&stOverlappedArea, &stDrawBitmap);

  // Redraw pstArea from Header
  pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
  while (pstWindow != NULL) {
    if ((pstWindow->dwFlags & WINDOW_FLAGS_SHOW) &&
      (kGetOverlappedRectangle(&(pstWindow->stArea), &stOverlappedArea,
        &stTempOverlappedArea) == TRUE)) {

      // Check History
      iTempOverlappedAreaSize = kGetRectangleWidth(&stTempOverlappedArea) *
        kGetRectangleHeight(&stTempOverlappedArea);

      for (i = 0; i < WINDOW_OVERLAPPEDAREALOGMAXCOUNT; i++)
      {
        if ((iTempOverlappedAreaSize <= viLargestOverlappedAreaSize[i]) &&
          (kIsInRectangle(&(vstLargestOverlappedArea[i]),
            stTempOverlappedArea.iX1, stTempOverlappedArea.iY1) == TRUE) &&
            (kIsInRectangle(&(vstLargestOverlappedArea[i]),
              stTempOverlappedArea.iX2, stTempOverlappedArea.iY2) == TRUE))
        {
          break;
        }
      }

      // If Previous Update Area contain currnet Area, move next window
      if (i < WINDOW_OVERLAPPEDAREALOGMAXCOUNT)
      {
        pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList),
          pstWindow);
        continue;
      }

      // find Min Area and Compare
      iMinAreaSize = 0xFFFFFF;
      iMinAreaIndex = 0;
      for (i = 0; i < WINDOW_OVERLAPPEDAREALOGMAXCOUNT; i++)
      {
        if (viLargestOverlappedAreaSize[i] < iMinAreaSize)
        {
          iMinAreaSize = viLargestOverlappedAreaSize[i];
          iMinAreaIndex = i;
        }
      }
      // Update History
      if (iMinAreaSize < iTempOverlappedAreaSize)
      {
        kMemCpy(&(vstLargestOverlappedArea[iMinAreaIndex]),
          &stTempOverlappedArea, sizeof(RECT));
        viLargestOverlappedAreaSize[iMinAreaIndex] = iTempOverlappedAreaSize;
      }

      // --- CRITCAL SECTION BEGIN ---
      kLock(&(pstWindow->stLock));

      // if Specific Window ID (qwDrawWindowID) is set
      if ((qwDrawWindowID != WINDOW_INVALIDID) &&
        (qwDrawWindowID != pstWindow->stLink.qwID))
      {
        // Bitmap Update
        kFillDrawBitmap(&stDrawBitmap, &(pstWindow->stArea), FALSE);
      }
      else
      {
        // Copy Window Buffer to VideoMemory
        kCopyWindowBufferToFrameBuffer(pstWindow, &stDrawBitmap);
      }

      kUnlock(&(pstWindow->stLock));
      // --- CRITCAL SECTION END ---
    }

    // Draw Bitmap is Clear
    if (kIsDrawBitmapAllOff(&stDrawBitmap) == TRUE)
    {
      break;
    }

    pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList), pstWindow);
  }

  kUnlock(&(gs_stWindowManager.stLock));
  // --- CRITCAL SECTION END ---

  // if mouse in there, redraw mouse cursor
  kSetRectangleData(gs_stWindowManager.iMouseX, gs_stWindowManager.iMouseY,
    gs_stWindowManager.iMouseX + MOUSE_CURSOR_WIDTH,
    gs_stWindowManager.iMouseY + MOUSE_CURSOR_HEIGHT, &stCursorArea);

  if (kIsRectangleOverlapped(&stOverlappedArea, &stCursorArea) == TRUE) {
    kDrawCursor(gs_stWindowManager.iMouseX, gs_stWindowManager.iMouseY);
  }
}

/*
  Copy Area Data from WindowBuffer to Video Memory
*/
static void kCopyWindowBufferToFrameBuffer(const WINDOW* pstWindow, DRAWBITMAP* pstDrawBitmap)
{
  RECT stTempArea;
  RECT stOverlappedArea;
  int iOverlappedWidth;
  int iOverlappedHeight;
  int iScreenWidth;
  int iWindowWidth;
  int i;
  COLOR* pstCurrentVideoMemoryAddress;
  COLOR* pstCurrentWindowBufferAddress;
  BYTE bTempBitmap;
  int iByteOffset;
  int iBitOffset;
  int iOffsetX;
  int iOffsetY;
  int iLastBitOffset;
  int iBulkCount;

  if (kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), &(pstDrawBitmap->stArea), &stTempArea) == FALSE) {
    return;
  }

  if (kGetOverlappedRectangle(&stTempArea, &(pstWindow->stArea), &stOverlappedArea) == FALSE) {
    return;
  }

  iScreenWidth = kGetRectangleWidth(&(gs_stWindowManager.stScreenArea));
  iWindowWidth = kGetRectangleWidth(&(pstWindow->stArea));
  iOverlappedWidth = kGetRectangleWidth(&stOverlappedArea);
  iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);

  for (iOffsetY = 0; iOffsetY < iOverlappedHeight; iOffsetY++)
  {
    if (kGetStartPositionInDrawBitmap(pstDrawBitmap, stOverlappedArea.iX1,
      stOverlappedArea.iY1 + iOffsetY, &iByteOffset, &iBitOffset) == FALSE)
    {
      break;
    }

    // Calc Memory Address
    pstCurrentVideoMemoryAddress = gs_stWindowManager.pstVideoMemory +
      (stOverlappedArea.iY1 + iOffsetY)* iScreenWidth + stOverlappedArea.iX1;
    pstCurrentWindowBufferAddress = pstWindow->pstWindowBuffer +
      (stOverlappedArea.iY1 - pstWindow->stArea.iY1 + iOffsetY) * iWindowWidth +
      (stOverlappedArea.iX1 - pstWindow->stArea.iX1);


    for (iOffsetX = 0; iOffsetX < iOverlappedWidth; )
    {
      // 0xFF
      if ((pstDrawBitmap->pbBitmap[iByteOffset] == 0xFF) &&
        (iBitOffset == 0x00) &&
        ((iOverlappedWidth - iOffsetX) >= 8))
      {
        for (iBulkCount = 0;
          (iBulkCount < ((iOverlappedWidth - iOffsetX) >> 3));
          iBulkCount++)
        {
          if (pstDrawBitmap->pbBitmap[iByteOffset + iBulkCount] != 0xFF)
          {
            break;
          }
        }

        kMemCpy(pstCurrentVideoMemoryAddress, pstCurrentWindowBufferAddress,
          (sizeof(COLOR) * iBulkCount) << 3);

        pstCurrentVideoMemoryAddress += iBulkCount << 3;
        pstCurrentWindowBufferAddress += iBulkCount << 3;
        kMemSet(pstDrawBitmap->pbBitmap + iByteOffset, 0x00,
          iBulkCount);

        iOffsetX += iBulkCount << 3;

        iByteOffset += iBulkCount;
        iBitOffset = 0;
      }
      // 0x00
      else if ((pstDrawBitmap->pbBitmap[iByteOffset] == 0x00) &&
        (iBitOffset == 0x00) &&
        ((iOverlappedWidth - iOffsetX) >= 8))
      {
        for (iBulkCount = 0;
          (iBulkCount < ((iOverlappedWidth - iOffsetX) >> 3));
          iBulkCount++)
        {
          if (pstDrawBitmap->pbBitmap[iByteOffset + iBulkCount] != 0x00)
          {
            break;
          }
        }

        pstCurrentVideoMemoryAddress += iBulkCount << 3;
        pstCurrentWindowBufferAddress += iBulkCount << 3;

        iOffsetX += iBulkCount << 3;

        iByteOffset += iBulkCount;
        iBitOffset = 0;
      }
      // Bit
      else
      {
        bTempBitmap = pstDrawBitmap->pbBitmap[iByteOffset];

        iLastBitOffset = MIN(8, iOverlappedWidth - iOffsetX + iBitOffset);

        for (i = iBitOffset; i < iLastBitOffset; i++)
        {
          if (bTempBitmap & (0x01 << i))
          {
            *pstCurrentVideoMemoryAddress = *pstCurrentWindowBufferAddress;

            bTempBitmap &= ~(0x01 << i);
          }

          pstCurrentVideoMemoryAddress++;
          pstCurrentWindowBufferAddress++;
        }

        iOffsetX += (iLastBitOffset - iBitOffset);

        pstDrawBitmap->pbBitmap[iByteOffset] = bTempBitmap;
        iByteOffset++;
        iBitOffset = 0;
      }
    }
  }
}

/*
  Draw Window Frame
*/
BOOL kDrawWindowFrame(QWORD qwWindowID)
{
  WINDOW* pstWindow;
  RECT stArea;
  int iWidth;
  int iHeight;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // For Clipping
  iWidth = kGetRectangleWidth(&(pstWindow->stArea));
  iHeight = kGetRectangleHeight(&(pstWindow->stArea));
  kSetRectangleData(0, 0, iWidth - 1, iHeight - 1, &stArea);

  // Draw Frame
  kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
    0, 0, iWidth - 1, iHeight - 1, WINDOW_COLOR_FRAME, FALSE);

  kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
    1, 1, iWidth - 2, iHeight - 2, WINDOW_COLOR_FRAME, FALSE);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  return TRUE;
}

/*
  Draw Window BackGround
*/
BOOL kDrawWindowBackground(QWORD qwWindowID)
{
  WINDOW* pstWindow;
  int iWidth;
  int iHeight;
  RECT stArea;
  int iX;
  int iY;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // For Clipping
  iWidth = kGetRectangleWidth(&(pstWindow->stArea));
  iHeight = kGetRectangleHeight(&(pstWindow->stArea));
  kSetRectangleData(0, 0, iWidth - 1, iHeight - 1, &stArea);

  // if has Title
  if (pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE)
  {
    iY = WINDOW_TITLEBAR_HEIGHT;
  }
  else
  {
    iY = 0;
  }

  // if has Frame
  if (pstWindow->dwFlags & WINDOW_FLAGS_DRAWFRAME)
  {
    iX = 2;
  }
  else
  {
    iX = 0;
  }

  // Fill Window
  kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
    iX, iY, iWidth - 1 - iX, iHeight - 1 - iX, WINDOW_COLOR_BACKGROUND,
    TRUE);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  return TRUE;
}

BOOL kDrawWindowTitle(QWORD qwWindowID, const char* pcTitle, BOOL bSelectedTitle)
{
  WINDOW* pstWindow;
  int iWidth;
  int iHeight;
  int iX;
  int iY;
  RECT stArea;
  RECT stButtonArea;
  COLOR stTitleBarTextColor;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // for clipping
  iWidth = kGetRectangleWidth(&(pstWindow->stArea));
  iHeight = kGetRectangleHeight(&(pstWindow->stArea));
  kSetRectangleData(0, 0, iWidth - 1, iHeight - 1, &stArea);

  if (bSelectedTitle == TRUE)
  {
    stTitleBarTextColor = WINDOW_COLOR_TITLEBARACTIVATETEXT;
  }
  else
  {
    stTitleBarTextColor = WINDOW_COLOR_TITLEBARINACTIVATETEXT;
  }


  // Fill Title Area
  kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
    0, 3, iWidth - 1, WINDOW_TITLEBAR_HEIGHT - 1,
    WINDOW_COLOR_TITLEBARABACKGROUND, TRUE);

  // Draw Title Text
  kInternalDrawText(&stArea, pstWindow->pstWindowBuffer,
    10, 3, stTitleBarTextColor, WINDOW_COLOR_TITLEBARABACKGROUND,
    pcTitle, kStrLen(pcTitle));

  // Draw Title frame upper line
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    1, 1, iWidth - 1, 1, WINDOW_COLOR_TITLEBARBRIGHT1);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    1, 2, iWidth - 1, 2, WINDOW_COLOR_TITLEBARBRIGHT2);

  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    1, 2, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT1);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    2, 2, 2, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT2);

  // Draw Title frmae lower line
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    2, WINDOW_TITLEBAR_HEIGHT - 2, iWidth - 2, WINDOW_TITLEBAR_HEIGHT - 2,
    WINDOW_COLOR_TITLEBARUNDERLINE);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    2, WINDOW_TITLEBAR_HEIGHT - 1, iWidth - 2, WINDOW_TITLEBAR_HEIGHT - 1,
    WINDOW_COLOR_TITLEBARUNDERLINE);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  // Draw Close button
  stButtonArea.iX1 = iWidth - WINDOW_XBUTTON_SIZE - 1;
  stButtonArea.iY1 = 1;
  stButtonArea.iX2 = iWidth - 2;
  stButtonArea.iY2 = WINDOW_XBUTTON_SIZE - 1;
  kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "",
    WINDOW_COLOR_BACKGROUND);

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // X Symbol in close button 
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    iWidth - 2 - 18 + 4, 1 + 4, iWidth - 2 - 4,
    WINDOW_TITLEBAR_HEIGHT - 6, WINDOW_COLOR_XBUTTONLINECOLOR);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    iWidth - 2 - 18 + 5, 1 + 4, iWidth - 2 - 4,
    WINDOW_TITLEBAR_HEIGHT - 7, WINDOW_COLOR_XBUTTONLINECOLOR);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    iWidth - 2 - 18 + 4, 1 + 5, iWidth - 2 - 5,
    WINDOW_TITLEBAR_HEIGHT - 6, WINDOW_COLOR_XBUTTONLINECOLOR);

  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    iWidth - 2 - 18 + 4, 19 - 4, iWidth - 2 - 4, 1 + 4,
    WINDOW_COLOR_XBUTTONLINECOLOR);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    iWidth - 2 - 18 + 5, 19 - 4, iWidth - 2 - 4, 1 + 5,
    WINDOW_COLOR_XBUTTONLINECOLOR);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    iWidth - 2 - 18 + 4, 19 - 5, iWidth - 2 - 5, 1 + 4,
    WINDOW_COLOR_XBUTTONLINECOLOR);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  return TRUE;
}

/*
  Draw button
*/
BOOL kDrawButton(QWORD qwWindowID, RECT* pstButtonArea, COLOR stBackgroundColor,
  const char* pcText, COLOR stTextColor)
{
  WINDOW* pstWindow;
  RECT stArea;
  int iWindowWidth;
  int iWindowHeight;
  int iTextLength;
  int iTextWidth;
  int iButtonWidth;
  int iButtonHeight;
  int iTextX;
  int iTextY;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // For Clipping
  iWindowWidth = kGetRectangleWidth(&(pstWindow->stArea));
  iWindowHeight = kGetRectangleHeight(&(pstWindow->stArea));
  kSetRectangleData(0, 0, iWindowWidth - 1, iWindowHeight - 1, &stArea);

  // Button Background
  kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
    pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX2,
    pstButtonArea->iY2, stBackgroundColor, TRUE);

  // Calc Text position
  iButtonWidth = kGetRectangleWidth(pstButtonArea);
  iButtonHeight = kGetRectangleHeight(pstButtonArea);
  iTextLength = kStrLen(pcText);
  iTextWidth = iTextLength * FONT_ENGLISHWIDTH;
  iTextX = (pstButtonArea->iX1 + iButtonWidth / 2) - iTextWidth / 2;
  iTextY = (pstButtonArea->iY1 + iButtonHeight / 2) - FONT_ENGLISHHEIGHT / 2;

  // Draw Text in middle of button
  kInternalDrawText(&stArea, pstWindow->pstWindowBuffer, iTextX, iTextY,
    stTextColor, stBackgroundColor, pcText, iTextLength);

  // left, upper
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX2,
    pstButtonArea->iY1, WINDOW_COLOR_BUTTONBRIGHT);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    pstButtonArea->iX1, pstButtonArea->iY1 + 1, pstButtonArea->iX2 - 1,
    pstButtonArea->iY1 + 1, WINDOW_COLOR_BUTTONBRIGHT);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX1,
    pstButtonArea->iY2, WINDOW_COLOR_BUTTONBRIGHT);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    pstButtonArea->iX1 + 1, pstButtonArea->iY1, pstButtonArea->iX1 + 1,
    pstButtonArea->iY2 - 1, WINDOW_COLOR_BUTTONBRIGHT);

  // roght, lower
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    pstButtonArea->iX1 + 1, pstButtonArea->iY2, pstButtonArea->iX2,
    pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    pstButtonArea->iX1 + 2, pstButtonArea->iY2 - 1, pstButtonArea->iX2,
    pstButtonArea->iY2 - 1, WINDOW_COLOR_BUTTONDARK);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    pstButtonArea->iX2, pstButtonArea->iY1 + 1, pstButtonArea->iX2,
    pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK);
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
    pstButtonArea->iX2 - 1, pstButtonArea->iY1 + 2, pstButtonArea->iX2 - 1,
    pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  return TRUE;
}

/*
  Draw Mouse cursor
*/
static void kDrawCursor(int iX, int iY)
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
        kInternalDrawPixel(&(gs_stWindowManager.stScreenArea),
          gs_stWindowManager.pstVideoMemory, i + iX, j + iY,
          MOUSE_CURSOR_OUTERLINE);
        break;

        // Outer
      case 2:
        kInternalDrawPixel(&(gs_stWindowManager.stScreenArea),
          gs_stWindowManager.pstVideoMemory, i + iX, j + iY,
          MOUSE_CURSOR_OUTER);
        break;

        // Inner
      case 3:
        kInternalDrawPixel(&(gs_stWindowManager.stScreenArea),
          gs_stWindowManager.pstVideoMemory, i + iX, j + iY,
          MOUSE_CURSOR_INNER);
        break;
      }

      pbCurrentPos++;
    }
  }
}

/*
  Move Mouse cursor
*/
void kMoveCursor(int iX, int iY)
{
  RECT stPreviousArea;

  // if position is out of screen
  if (iX < gs_stWindowManager.stScreenArea.iX1)
  {
    iX = gs_stWindowManager.stScreenArea.iX1;
  }
  else if (iX > gs_stWindowManager.stScreenArea.iX2)
  {
    iX = gs_stWindowManager.stScreenArea.iX2;
  }

  if (iY < gs_stWindowManager.stScreenArea.iY1)
  {
    iY = gs_stWindowManager.stScreenArea.iY1;
  }
  else if (iY > gs_stWindowManager.stScreenArea.iY2)
  {
    iY = gs_stWindowManager.stScreenArea.iY2;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowManager.stLock));

  // store previose cursor's position for redraw
  stPreviousArea.iX1 = gs_stWindowManager.iMouseX;
  stPreviousArea.iY1 = gs_stWindowManager.iMouseY;
  stPreviousArea.iX2 = gs_stWindowManager.iMouseX + MOUSE_CURSOR_WIDTH - 1;
  stPreviousArea.iY2 = gs_stWindowManager.iMouseY + MOUSE_CURSOR_HEIGHT - 1;

  // set new position
  gs_stWindowManager.iMouseX = iX;
  gs_stWindowManager.iMouseY = iY;

  kUnlock(&(gs_stWindowManager.stLock));
  // --- CRITCAL SECTION END ---

  // redraw  Window
  kRedrawWindowByArea(&stPreviousArea, WINDOW_INVALIDID);

  // draw Cursor
  kDrawCursor(iX, iY);
}

/*
  Get current mouse cursor position
*/
void kGetCursorPosition(int* piX, int* piY)
{
  *piX = gs_stWindowManager.iMouseX;
  *piY = gs_stWindowManager.iMouseY;
}

/*
  Draw Pixel in Window
*/
BOOL kDrawPixel(QWORD qwWindowID, int iX, int iY, COLOR stColor)
{
  WINDOW* pstWindow;
  RECT stArea;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // Convert Position
  kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
    pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);

  // Draw
  kInternalDrawPixel(&stArea, pstWindow->pstWindowBuffer, iX, iY,
    stColor);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  return TRUE;
}


/*
  Draw Line in Window
*/
BOOL kDrawLine(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor)
{
  WINDOW* pstWindow;
  RECT stArea;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // Convert Position
  kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
    pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);

  // Draw
  kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, iX1, iY1,
    iX2, iY2, stColor);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---
  return TRUE;
}

/*
  Draw Rect in Window
*/
BOOL kDrawRect(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2,
  COLOR stColor, BOOL bFill)
{
  WINDOW* pstWindow;
  RECT stArea;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // Convert Position
  kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
    pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);

  // Draw
  kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer, iX1, iY1,
    iX2, iY2, stColor, bFill);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  return TRUE;
}

/*
  Draw Circle in Window
*/
BOOL kDrawCircle(QWORD qwWindowID, int iX, int iY, int iRadius, COLOR stColor,
  BOOL bFill)
{
  WINDOW* pstWindow;
  RECT stArea;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // Convert Position
  kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
    pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);

  // Draw
  kInternalDrawCircle(&stArea, pstWindow->pstWindowBuffer,
    iX, iY, iRadius, stColor, bFill);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  return TRUE;
}

/*
  Draw Text in Window
*/
BOOL kDrawText(QWORD qwWindowID, int iX, int iY, COLOR stTextColor,
  COLOR stBackgroundColor, const char* pcString, int iLength)
{
  WINDOW* pstWindow;
  RECT stArea;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // Convert Position
  kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
    pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);

  // Draw
  kInternalDrawText(&stArea, pstWindow->pstWindowBuffer, iX, iY,
    stTextColor, stBackgroundColor, pcString, iLength);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  return TRUE;
}

/*
  Draw Background image on Background window
*/
extern unsigned char g_vbWallPaper[];
extern unsigned int size_g_vbWallPaper;

void kDrawBackgroundImage(void)
{
  JPEG* pstJpeg;
  COLOR* pstOutputBuffer;
  WINDOWMANAGER* pstWindowManager;
  int i;
  int j;
  int iMiddleX;
  int iMiddleY;
  int iScreenWidth;
  int iScreenHeight;

  // Get WindowManager
  pstWindowManager = kGetWindowManager();

  // Init JPEG 
  pstJpeg = (JPEG*)kAllocateMemory(sizeof(JPEG));
  if (kJPEGInit(pstJpeg, g_vbWallPaper, size_g_vbWallPaper) == FALSE)
  {
    return;
  }

  // Allocat Memory 
  pstOutputBuffer = (COLOR*)kAllocateMemory(pstJpeg->width * pstJpeg->height *
    sizeof(COLOR));
  if (pstOutputBuffer == NULL)
  {
    kFreeMemory(pstJpeg);
    return;
  }

  // Decode
  if (kJPEGDecode(pstJpeg, pstOutputBuffer) == FALSE)
  {
    kFreeMemory(pstOutputBuffer);
    kFreeMemory(pstJpeg);
    return;
  }

  // Calc Area Info
  iScreenWidth = kGetRectangleWidth(&(pstWindowManager->stScreenArea));
  iScreenHeight = kGetRectangleHeight(&(pstWindowManager->stScreenArea));
  iMiddleX = (iScreenWidth - pstJpeg->width) / 2;
  iMiddleY = (iScreenHeight - pstJpeg->height) / 2;

  // Copy to WindowManager Screen Buffer
  kBitBlt(pstWindowManager->qwBackgroundWindowID, iMiddleX, iMiddleY,
    pstOutputBuffer, pstJpeg->width, pstJpeg->height);

  // Free Memory
  kFreeMemory(pstOutputBuffer);
  kFreeMemory(pstJpeg);
}

/*
  Copy Window buffer -> Window Manager Screen Buffer
*/
BOOL kBitBlt(QWORD qwWindowID, int iX, int iY, COLOR* pstBuffer, int iWidth,
  int iHeight)
{
  WINDOW* pstWindow;
  RECT stWindowArea;
  RECT stBufferArea;
  RECT stOverlappedArea;
  int iWindowWdith;
  int iOverlappedWidth;
  int iOverlappedHeight;
  int i;
  int j;
  int iWindowPosition;
  int iBufferPosition;
  int iStartX;
  int iStartY;

  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
    pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stWindowArea);
  kSetRectangleData(iX, iY, iX + iWidth - 1, iY + iHeight - 1, &stBufferArea);

  if (kGetOverlappedRectangle(&stWindowArea, &stBufferArea, &stOverlappedArea)
    == FALSE)
  {
    kUnlock(&pstWindow->stLock);
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // Calc Area
  iWindowWdith = kGetRectangleWidth(&stWindowArea);
  iOverlappedWidth = kGetRectangleWidth(&stOverlappedArea);
  iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);

  if (iX < 0)
  {
    iStartX = iX;
  }
  else
  {
    iStartX = 0;
  }

  if (iY < 0)
  {
    iStartY = iY;
  }
  else
  {
    iStartY = 0;
  }

  // Copy 
  for (j = 0; j < iOverlappedHeight; j++)
  {
    iWindowPosition = (iWindowWdith * (stOverlappedArea.iY1 + j)) +
      stOverlappedArea.iX1;
    iBufferPosition = (iWidth * j + iStartY) + iStartX;

    kMemCpy(pstWindow->pstWindowBuffer + iWindowPosition,
      pstBuffer + iBufferPosition, iOverlappedWidth * sizeof(COLOR));
  }

  kUnlock(&pstWindow->stLock);
  // --- CRITCAL SECTION END ---

  return TRUE;
}

/*
  Put Event To Window Event Queue
*/
BOOL kSendEventToWindow(QWORD qwWindowID, const EVENT* pstEvent)
{
  WINDOW* pstWindow;
  BOOL bResult;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL) {
    return FALSE;
  }

  // Put Event
  bResult = kPutQueue(&(pstWindow->stEventQueue), pstEvent);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

/*
  Get Event From Window Event Queue
*/
BOOL kReceiveEventFromWindowQueue(QWORD qwWindowID, EVENT* pstEvent)
{
  WINDOW* pstWindow;
  BOOL bResult;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL) {
    return FALSE;
  }

  // Get Event
  bResult = kGetQueue(&(pstWindow->stEventQueue), pstEvent);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

/*
  Put Event To Window Manager Event Queue
*/
BOOL kSendEventToWindowManager(const EVENT* pstEvent)
{
  BOOL bResult;

  if (kIsQueueFull(&(gs_stWindowManager.stEventQueue)) == FALSE) {
    // --- CRITCAL SECTION BEGIN ---
    kLock(&(gs_stWindowManager.stLock));

    // Put Event
    bResult = kPutQueue(&(gs_stWindowManager.stEventQueue), pstEvent);

    kUnlock(&(gs_stWindowManager.stLock));
    // --- CRITCAL SECTION END ---
  }

  return bResult;
}

/*
  Get Event From Window Manager Event Queue
*/
BOOL kReceiveEventFromWindowManagerQueue(EVENT* pstEvent)
{
  BOOL bResult;

  if (kIsQueueEmpty(&(gs_stWindowManager.stEventQueue)) == FALSE) {
    // --- CRITCAL SECTION BEGIN ---
    kLock(&(gs_stWindowManager.stLock));

    // Put Event
    bResult = kGetQueue(&(gs_stWindowManager.stEventQueue), pstEvent);

    kUnlock(&(gs_stWindowManager.stLock));
    // --- CRITCAL SECTION END ---
  }

  return bResult;
}

/*
  Set Mouse Event
  Mouse Point is Converted to Logical Point.
*/
BOOL kSetMouseEvent(QWORD qwWindowID, QWORD qwEventType, int iMouseX, int iMouseY,
  BYTE bButtonStatus, EVENT* pstEvent)
{
  POINT stMouseXYInWindow;
  POINT stMouseXY;

  switch (qwEventType)
  {
  case EVENT_MOUSE_MOVE:
  case EVENT_MOUSE_LBUTTONDOWN:
  case EVENT_MOUSE_LBUTTONUP:
  case EVENT_MOUSE_RBUTTONDOWN:
  case EVENT_MOUSE_RBUTTONUP:
  case EVENT_MOUSE_MBUTTONDOWN:
  case EVENT_MOUSE_MBUTTONUP:
    stMouseXY.iX = iMouseX;
    stMouseXY.iY = iMouseY;

    if (kConvertPointScreenToClient(qwWindowID, &stMouseXY, &stMouseXYInWindow) == FALSE) {
      return FALSE;
    }

    pstEvent->qwType = qwEventType;
    pstEvent->stMouseEvent.qwWindowID = qwWindowID;
    pstEvent->stMouseEvent.bButtonStatus = bButtonStatus;
    kMemCpy(&(pstEvent->stMouseEvent.stPoint), &stMouseXYInWindow, sizeof(POINT));
    break;
  default:
    return FALSE;
    break;
  }
  return TRUE;
}

/*
  Set Window Event
  Window Point is Physical Point.
*/
BOOL kSetWindowEvent(QWORD qwWindowID, QWORD qwEventType, EVENT* pstEvent)
{
  RECT stArea;

  switch (qwEventType)
  {
  case EVENT_WINDOW_SELECT:
  case EVENT_WINDOW_DESELECT:
  case EVENT_WINDOW_MOVE:
  case EVENT_WINDOW_RESIZE:
  case EVENT_WINDOW_CLOSE:

    pstEvent->qwType = qwEventType;
    pstEvent->stWindowEvent.qwWindowID = qwWindowID;
    if (kGetWindowArea(qwWindowID, &stArea) == FALSE)
    {
      return FALSE;
    }

    kMemCpy(&(pstEvent->stWindowEvent.stArea), &stArea, sizeof(RECT));
    break;

  default:
    return FALSE;
    break;
  }
  return TRUE;
}

/*
  Set Key Event
*/
void kSetKeyEvent(QWORD qwWindow, const KEYDATA* pstKeyData, EVENT* pstEvent)
{
  if (pstKeyData->bFlags & KEY_FLAGS_DOWN)
  {
    pstEvent->qwType = EVENT_KEY_DOWN;
  }
  else
  {
    pstEvent->qwType = EVENT_KEY_UP;
  }

  pstEvent->stKeyEvent.bASCIICode = pstKeyData->bASCIICode;
  pstEvent->stKeyEvent.bScanCode = pstKeyData->bScanCode;
  pstEvent->stKeyEvent.bFlags = pstKeyData->bFlags;
}

/*
  Send UPDATE SCREEN Event to WindowManager
*/
BOOL kUpdateScreenByID(QWORD qwWindowID)
{
  EVENT stEvent;
  WINDOW* pstWindow;

  pstWindow = kGetWindow(qwWindowID);
  if ((pstWindow == NULL) &&
    ((pstWindow->dwFlags & WINDOW_FLAGS_SHOW) == 0))
  {
    return FALSE;
  }

  stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYID;
  stEvent.stWindowEvent.qwWindowID = qwWindowID;

  return kSendEventToWindowManager(&stEvent);
}

/*
  Send UPDATE SCREEN WINDOW AREA Event to WindowManager
  Area is Logical area

*/
BOOL kUpdateScreenByWindowArea(QWORD qwWindowID, const RECT* pstArea)
{
  EVENT stEvent;
  WINDOW* pstWindow;

  pstWindow = kGetWindow(qwWindowID);
  if ((pstWindow == NULL) &&
    ((pstWindow->dwFlags & WINDOW_FLAGS_SHOW) == 0))
  {
    return FALSE;
  }

  stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA;
  stEvent.stWindowEvent.qwWindowID = qwWindowID;
  kMemCpy(&(stEvent.stWindowEvent.stArea), pstArea, sizeof(RECT));

  return kSendEventToWindowManager(&stEvent);
}

/*
  Send UPDATE SCREEN WINDOW AREA Event to WindowManager
  Area is Physical area
*/
BOOL kUpdateScreenByScreenArea(const RECT* pstArea)
{
  EVENT stEvent;

  stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA;
  stEvent.stWindowEvent.qwWindowID = WINDOW_INVALIDID;
  kMemCpy(&(stEvent.stWindowEvent.stArea), pstArea, sizeof(RECT));

  return kSendEventToWindowManager(&stEvent);
}

/*
  Get Top Window ID that the point be in
*/
QWORD kFindWindowByPoint(int iX, int iY)
{
  QWORD qwWindowID;
  WINDOW* pstWindow;

  qwWindowID = gs_stWindowManager.qwBackgroundWindowID;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowManager.stLock));

  // Find From Window List
  pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
  do
  {
    if ((pstWindow->dwFlags & WINDOW_FLAGS_SHOW) &&
      (kIsInRectangle(&(pstWindow->stArea), iX, iY) == TRUE))
    {
      qwWindowID = pstWindow->stLink.qwID;
      break;
    }
    pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList), pstWindow);
  } while (pstWindow != NULL);

  kUnlock(&(gs_stWindowManager.stLock));
  // --- CRITCAL SECTION END ---

  return qwWindowID;
}

/*
  Get Window Id using Window Title
*/
QWORD kFindWindowByTitle(const char* pcTitle)
{
  QWORD qwWindowID;
  WINDOW* pstWindow;
  int iTitleLength;

  qwWindowID = WINDOW_INVALIDID;
  iTitleLength = kStrLen(pcTitle);

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowManager.stLock));

  // Find From Window List
  pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
  while (pstWindow != NULL)
  {
    // Compare title 
    if ((kStrLen(pstWindow->vcWindowTitle) == iTitleLength) &&
      (kMemCmp(pstWindow->vcWindowTitle, pcTitle, iTitleLength) == 0))
    {
      qwWindowID = pstWindow->stLink.qwID;
      break;
    }

    pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList),
      pstWindow);
  }

  kUnlock(&(gs_stWindowManager.stLock));
  // --- CRITCAL SECTION END ---

  return qwWindowID;
}

/*
  Get Top Window ID
*/
QWORD kGetTopWindowID(void)
{
  WINDOW* pstActiveWindow;
  QWORD qwActiveWindowID;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowManager.stLock));

  // Get Window List tail 
  pstActiveWindow = (WINDOW*)kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
  if (pstActiveWindow != NULL)
  {
    qwActiveWindowID = pstActiveWindow->stLink.qwID;
  }
  else
  {
    qwActiveWindowID = WINDOW_INVALIDID;
  }

  kUnlock(&(gs_stWindowManager.stLock));
  // --- CRITCAL SECTION END ---

  return qwActiveWindowID;
}

/*
  Move window To Top and send WINDOW_SELECT Event
  and send WINDOW_DESELECT to Previous top window
*/
BOOL kMoveWindowToTop(QWORD qwWindowID)
{
  WINDOW* pstWindow;
  RECT stArea;
  DWORD dwFlags;
  QWORD qwTopWindowID;
  EVENT stEvent;

  qwTopWindowID = kGetTopWindowID();

  // if already Top
  if (qwTopWindowID == qwWindowID) {
    return TRUE;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowManager.stLock));

  // Remove And Move to Header
  pstWindow = kRemoveList(&(gs_stWindowManager.stWindowList), qwWindowID);
  if (pstWindow != NULL)
  {
    kAddListToHeader(&(gs_stWindowManager.stWindowList), pstWindow);

    // Store Area and flags for Redraw
    kConvertRectScreenToClient(qwWindowID, &(pstWindow->stArea), &stArea);
    dwFlags = pstWindow->dwFlags;
  }

  kUnlock(&(gs_stWindowManager.stLock));
  // --- CRITCAL SECTION END ---

  if (pstWindow != NULL) {

    // Send Window Select Event
    kSetWindowEvent(qwWindowID, EVENT_WINDOW_SELECT, &stEvent);
    kSendEventToWindow(qwWindowID, &stEvent);

    // if window has title, update title area
    if (dwFlags & WINDOW_FLAGS_DRAWTITLE)
    {
      kUpdateWindowTitle(qwWindowID, TRUE);
      stArea.iY1 += WINDOW_TITLEBAR_HEIGHT;
      kUpdateScreenByWindowArea(qwWindowID, &stArea);
    }
    else
    {
      kUpdateScreenByID(qwWindowID);
    }

    // Send Window Deselect Event to previous Top window
    kSetWindowEvent(qwTopWindowID, EVENT_WINDOW_DESELECT, &stEvent);
    kSendEventToWindow(qwTopWindowID, &stEvent);
    kUpdateWindowTitle(qwTopWindowID, FALSE);
    return TRUE;
  }
  return FALSE;
}

/*
  Move Window and Send WINDOW_MOVE Evnet
*/
BOOL kMoveWindow(QWORD qwWindowID, int iX, int iY)
{
  WINDOW* pstWindow;
  RECT stPreviousArea;
  int iWidth;
  int iHeight;
  EVENT stEvent;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  // Store Previous Area
  kMemCpy(&stPreviousArea, &(pstWindow->stArea), sizeof(RECT));

  // Move Window
  iWidth = kGetRectangleWidth(&stPreviousArea);
  iHeight = kGetRectangleHeight(&stPreviousArea);
  kSetRectangleData(iX, iY, iX + iWidth - 1, iY + iHeight - 1,
    &(pstWindow->stArea));

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---

  // Update Previous Area
  kUpdateScreenByScreenArea(&stPreviousArea);

  // Update Current Area
  kUpdateScreenByID(qwWindowID);

  // Send WINDOW MOVE Event
  kSetWindowEvent(qwWindowID, EVENT_WINDOW_MOVE, &stEvent);
  kSendEventToWindow(qwWindowID, &stEvent);

  return TRUE;
}

/*
  Check that a point is on Title Bar
*/
BOOL kIsInTitleBar(QWORD qwWindowID, int iX, int iY)
{
  WINDOW* pstWindow;

  pstWindow = kGetWindow(qwWindowID);

  if ((pstWindow == NULL) ||
    ((pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE) == 0))
  {
    return FALSE;
  }

  // Check
  if ((pstWindow->stArea.iX1 <= iX) && (iX <= pstWindow->stArea.iX2) &&
    (pstWindow->stArea.iY1 <= iY) &&
    (iY <= pstWindow->stArea.iY1 + WINDOW_TITLEBAR_HEIGHT))
  {
    return TRUE;
  }

  return FALSE;
}

/*
  Check that a point is on Close button
*/
BOOL kIsInCloseButton(QWORD qwWindowID, int iX, int iY)
{
  WINDOW* pstWindow;

  pstWindow = kGetWindow(qwWindowID);
  if ((pstWindow == NULL) &&
    ((pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE) == 0))
  {
    return FALSE;
  }

  // Check
  if (((pstWindow->stArea.iX2 - WINDOW_XBUTTON_SIZE - 1) <= iX) &&
    (iX <= (pstWindow->stArea.iX2 - 1)) &&
    ((pstWindow->stArea.iY1 + 1) <= iY) &&
    (iY <= (pstWindow->stArea.iY1 + 1 + WINDOW_XBUTTON_SIZE)))
  {
    return TRUE;
  }

  return FALSE;
}

/*
  Update Window Title Area
*/
static BOOL kUpdateWindowTitle(QWORD qwWindowID, BOOL bSelectedTitle)
{
  WINDOW* pstWindow;
  RECT stTitleBarArea;

  // Find Window
  pstWindow = kGetWindow(qwWindowID);

  if ((pstWindow != NULL) &&
    (pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE))
  {
    kDrawWindowTitle(pstWindow->stLink.qwID, pstWindow->vcWindowTitle,
      bSelectedTitle);

    stTitleBarArea.iX1 = 0;
    stTitleBarArea.iY1 = 0;
    stTitleBarArea.iX2 = kGetRectangleWidth(&(pstWindow->stArea)) - 1;
    stTitleBarArea.iY2 = WINDOW_TITLEBAR_HEIGHT;

    kUpdateScreenByWindowArea(qwWindowID, &stTitleBarArea);

    return TRUE;
  }

  return FALSE;
}


/*
  Get Window Area (Physical)
*/
BOOL kGetWindowArea(QWORD qwWindowID, RECT* pstArea)
{
  WINDOW* pstWindow;

  // Find window
  // --- CRITCAL SECTION BEGIN ---
  pstWindow = kGetWindowWithWindowLock(qwWindowID);
  if (pstWindow == NULL)
  {
    return FALSE;
  }

  kMemCpy(pstArea, &(pstWindow->stArea), sizeof(RECT));

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END ---
  return TRUE;
}

/*
  Convert Area Physical(Screen) to Logical(Window)
*/
BOOL kConvertPointScreenToClient(QWORD qwWindowID, const POINT* pstXY,
  POINT* pstXYInWindow)
{
  RECT stArea;

  if (kGetWindowArea(qwWindowID, &stArea) == FALSE)
  {
    return FALSE;
  }

  pstXYInWindow->iX = pstXY->iX - stArea.iX1;
  pstXYInWindow->iY = pstXY->iY - stArea.iY1;
  return TRUE;
}

/*
  Convert Area Logical(Window) to Physical(Screen)
*/
BOOL kConvertPointClientToScreen(QWORD qwWindowID, const POINT* pstXY,
  POINT* pstXYInScreen)
{
  RECT stArea;

  if (kGetWindowArea(qwWindowID, &stArea) == FALSE)
  {
    return FALSE;
  }

  pstXYInScreen->iX = pstXY->iX + stArea.iX1;
  pstXYInScreen->iY = pstXY->iY + stArea.iY1;
  return TRUE;
}

/*
  Convert Rect Area Physical(Screen) to Logical(Window)
*/
BOOL kConvertRectScreenToClient(QWORD qwWindowID, const RECT* pstArea,
  RECT* pstAreaInWindow)
{
  RECT stWindowArea;

  if (kGetWindowArea(qwWindowID, &stWindowArea) == FALSE)
  {
    return FALSE;
  }

  pstAreaInWindow->iX1 = pstArea->iX1 - stWindowArea.iX1;
  pstAreaInWindow->iY1 = pstArea->iY1 - stWindowArea.iY1;
  pstAreaInWindow->iX2 = pstArea->iX2 - stWindowArea.iX1;
  pstAreaInWindow->iY2 = pstArea->iY2 - stWindowArea.iY1;
  return TRUE;
}

/*
  Convert Rect Area Logical(Window) to Physical(Screen)
*/
BOOL kConvertRectClientToScreen(QWORD qwWindowID, const RECT* pstArea,
  RECT* pstAreaInScreen)
{
  RECT stWindowArea;

  if (kGetWindowArea(qwWindowID, &stWindowArea) == FALSE)
  {
    return FALSE;
  }

  pstAreaInScreen->iX1 = pstArea->iX1 + stWindowArea.iX1;
  pstAreaInScreen->iY1 = pstArea->iY1 + stWindowArea.iY1;
  pstAreaInScreen->iX2 = pstArea->iX2 + stWindowArea.iX1;
  pstAreaInScreen->iY2 = pstArea->iY2 + stWindowArea.iY1;
  return TRUE;
}

/*
  Create Draw Bitmap using draw Area Info
*/
BOOL kCreateDrawBitmap(const RECT* pstArea, DRAWBITMAP* pstDrawBitmap)
{
  if (kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), pstArea,
    &(pstDrawBitmap->stArea)) == FALSE)
  {
    return FALSE;
  }

  pstDrawBitmap->pbBitmap = gs_stWindowManager.pbDrawBitmap;

  return kFillDrawBitmap(pstDrawBitmap, &(pstDrawBitmap->stArea), TRUE);
}

/*
  Get Bitmap offset using Physical(Screen) Position Info
*/
BOOL kGetStartPositionInDrawBitmap(const DRAWBITMAP* pstDrawBitmap,
  int iX, int iY, int* piByteOffset, int* piBitOffset)
{
  int iWidth;
  int iOffsetX;
  int iOffsetY;

  if (kIsInRectangle(&(pstDrawBitmap->stArea), iX, iY) == FALSE)
  {
    return FALSE;
  }

  // Calc Offset
  iOffsetX = iX - pstDrawBitmap->stArea.iX1;
  iOffsetY = iY - pstDrawBitmap->stArea.iY1;
  iWidth = kGetRectangleWidth(&(pstDrawBitmap->stArea));

  // Byte Offset (1 Byte = 8 pixel in Bitmap)
  *piByteOffset = (iOffsetY * iWidth + iOffsetX) >> 3;
  // Bit Offset
  *piBitOffset = (iOffsetY * iWidth + iOffsetX) & 0x7;

  return TRUE;
}

/*
  Fill overlapeed area in Draw Bitmap
*/
static BOOL kFillDrawBitmap(DRAWBITMAP* pstDrawBitmap, RECT* pstArea, BOOL bFill)
{
  RECT stOverlappedArea;
  int iByteOffset;
  int iBitOffset;
  int iAreaSize;
  int iOverlappedWidth;
  int iOverlappedHeight;
  BYTE bTempBitmap;
  int i;
  int iOffsetX;
  int iOffsetY;
  int iBulkCount;
  int iLastBitOffset;

  if (kGetOverlappedRectangle(&(pstDrawBitmap->stArea), pstArea,
    &stOverlappedArea) == FALSE)
  {
    return FALSE;
  }

  // Calc Overlapped Area
  iOverlappedWidth = kGetRectangleWidth(&stOverlappedArea);
  iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);

  // Height
  for (iOffsetY = 0; iOffsetY < iOverlappedHeight; iOffsetY++)
  {
    // Get Byte, Bit Offset in Bitmap
    if (kGetStartPositionInDrawBitmap(pstDrawBitmap, stOverlappedArea.iX1,
      stOverlappedArea.iY1 + iOffsetY, &iByteOffset, &iBitOffset) ==
      FALSE)
    {
      break;
    }

    // Width, iOffsetX is Bit offset
    for (iOffsetX = 0; iOffsetX < iOverlappedWidth; )
    {
      // if fill area's width is graeter than or equal to 8 using memset
      if ((iBitOffset == 0x00) && ((iOverlappedWidth - iOffsetX) >= 8))
      {
        iBulkCount = (iOverlappedWidth - iOffsetX) >> 3;

        // Fill Bitmap
        if (bFill == TRUE)
        {
          kMemSet(pstDrawBitmap->pbBitmap + iByteOffset, 0xFF,
            iBulkCount);
        }
        else
        {
          kMemSet(pstDrawBitmap->pbBitmap + iByteOffset, 0x00,
            iBulkCount);
        }

        iOffsetX += iBulkCount << 3;

        iByteOffset += iBulkCount;
        iBitOffset = 0;
      }
      // Remain part Bit operation
      else
      {
        iLastBitOffset = MIN(8, iOverlappedWidth - iOffsetX + iBitOffset);

        bTempBitmap = 0;
        for (i = iBitOffset; i < iLastBitOffset; i++)
        {
          bTempBitmap |= (0x01 << i);
        }

        iOffsetX += (iLastBitOffset - iBitOffset);

        // Fill Bitmap
        if (bFill == TRUE)
        {
          pstDrawBitmap->pbBitmap[iByteOffset] |= bTempBitmap;
        }
        else
        {
          pstDrawBitmap->pbBitmap[iByteOffset] &= ~(bTempBitmap);
        }
        iByteOffset++;
        iBitOffset = 0;
      }
    }
  }
  return TRUE;
}

/*
  Check that Draw Bitmap is all off
*/
BOOL kIsDrawBitmapAllOff(const DRAWBITMAP* pstDrawBitmap)
{
  int iByteCount;
  int iLastBitIndex;
  int iWidth;
  int iHeight;
  int i;
  BYTE* pbTempPosition;
  int iSize;

  iWidth = kGetRectangleWidth(&(pstDrawBitmap->stArea));
  iHeight = kGetRectangleHeight(&(pstDrawBitmap->stArea));

  iSize = iWidth * iHeight;
  iByteCount = iSize >> 3;

  // check 8 Bytes at once
  pbTempPosition = pstDrawBitmap->pbBitmap;
  for (i = 0; i < (iByteCount >> 3); i++)
  {
    if (*(QWORD*)(pbTempPosition) != 0)
    {
      return FALSE;
    }
    pbTempPosition += 8;
  }

  // check Remain Bytes
  for (i = 0; i < (iByteCount & 0x7); i++)
  {
    if (*pbTempPosition != 0)
    {
      return FALSE;
    }
    pbTempPosition++;
  }

  // check Remain bits
  iLastBitIndex = iSize & 0x7;
  for (i = 0; i < iLastBitIndex; i++)
  {
    if (*pbTempPosition & (0x01 << i))
    {
      return FALSE;
    }
  }

  return TRUE;
}