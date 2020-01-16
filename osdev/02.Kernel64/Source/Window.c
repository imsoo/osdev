#include "Window.h"
#include "DynamicMemory.h"
#include "Task.h"
#include "VBE.h"
#include "Console.h"
#include "Font.h"

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

  // Create Background Window
  qwBackgroundWindowID = kCreateWindow(0, 0, pstModeInfo->wXResolution, pstModeInfo->wYResolution,
    0, WINDOW_BACKGROUNDWINDOWTITLE);
  gs_stWindowManager.qwBackgroundWindowID = qwBackgroundWindowID;

  kDrawRect(qwBackgroundWindowID, 0, 0, pstModeInfo->wXResolution - 1, pstModeInfo->wYResolution - 1,
    WINDOW_COLOR_SYSTEMBACKGROUND, TRUE);

  kShowWindow(qwBackgroundWindowID, TRUE);
}

/*
  Create Window
*/
QWORD kCreateWindow(int iX, int iY, int iWidth, int iHeight, DWORD dwFlags,
  const char* pcTitle)
{
  WINDOW* pstWindow;
  TCB* pstTask;

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

  // Allocate Window buffer
  pstWindow->pstWindowBuffer = (COLOR*)kAllocateMemory(iWidth * iHeight * sizeof(COLOR));
  if (pstWindow == NULL)
  {
    kFreeWindow(pstWindow->stLink.qwID);
    return WINDOW_INVALIDID;
  }

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
    kDrawWindowTitle(pstWindow->stLink.qwID, pcTitle);
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowManager.stLock));

  // Add Window List 
  kAddListToTail(&gs_stWindowManager.stWindowList, pstWindow);

  kUnlock(&(gs_stWindowManager.stLock));
  // --- CRITCAL SECTION END ---

  // if Show Flag is set, Draw Window
  if (dwFlags & WINDOW_FLAGS_SHOW)
  {
    kRedrawWindowByArea(&(pstWindow->stArea));
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

  // Remove Window in Window List
  if (kRemoveList(&(gs_stWindowManager.stWindowList), qwWindowID) == NULL)
  {
    kUnlock(&(pstWindow->stLock));
    // --- CRITCAL SECTION END 2 ---

    kUnlock(&(gs_stWindowManager.stLock));
    // --- CRITCAL SECTION END 1 ---
    return FALSE;
  }

  // --- CRITCAL SECTION BEGIN 3 ---
  kLock(&(pstWindow->stLock));

  // Free Window buffer
  kFreeMemory(pstWindow->pstWindowBuffer);

  kUnlock(&(pstWindow->stLock));
  // --- CRITCAL SECTION END 3 ---

  // Free Window
  kFreeWindow(qwWindowID);
  // --- CRITCAL SECTION END 2 ---

  kUnlock(&(gs_stWindowManager.stLock));
  // --- CRITCAL SECTION END 1 ---

  // Redraw 
  kRedrawWindowByArea(&stArea);
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
  kRedrawWindowByArea(&(pstWindow->stArea));
  return TRUE;
}

/*
  Redraw Area
*/
BOOL kRedrawWindowByArea(const RECT* pstArea)
{
  WINDOW* pstWindow;
  WINDOW* pstTargetWindow = NULL;
  RECT stOverlappedArea;
  RECT stCursorArea;

  // Not in screen
  if (kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), pstArea, &stOverlappedArea) == FALSE) {
    return FALSE;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stWindowManager.stLock));


  // Redraw pstArea from Header
  pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
  while (pstWindow != NULL) {
    if ((pstWindow->dwFlags & WINDOW_FLAGS_SHOW) &&
      (kIsRectangleOverlapped(&(pstWindow->stArea), &stOverlappedArea)) == TRUE) {

      // --- CRITCAL SECTION BEGIN ---
      kLock(&(pstWindow->stLock));

      kCopyWindowBufferToFrameBuffer(pstWindow, &stOverlappedArea);

      kUnlock(&(pstWindow->stLock));
      // --- CRITCAL SECTION END ---
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
static void kCopyWindowBufferToFrameBuffer(const WINDOW* pstWindow, const RECT* pstCopyArea)
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

  if (kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), pstCopyArea, &stTempArea) == FALSE) {
    return;
  }

  if (kGetOverlappedRectangle(&stTempArea, &(pstWindow->stArea), &stOverlappedArea) == FALSE) {
    return;
  }

  iScreenWidth = kGetRectangleWidth(&(gs_stWindowManager.stScreenArea));
  iWindowWidth = kGetRectangleWidth(&(pstWindow->stArea));
  iOverlappedWidth = kGetRectangleWidth(&stOverlappedArea);
  iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);

  // Calc Memory address
  pstCurrentVideoMemoryAddress = gs_stWindowManager.pstVideoMemory +
    stOverlappedArea.iY1 * iScreenWidth + stOverlappedArea.iX1;

  pstCurrentWindowBufferAddress = pstWindow->pstWindowBuffer +
    (stOverlappedArea.iY1 - pstWindow->stArea.iY1) * iWindowWidth +
    (stOverlappedArea.iX1 - pstWindow->stArea.iX1);

  // Copy 
  for (i = 0; i < iOverlappedHeight; i++) {
    kMemCpy(pstCurrentVideoMemoryAddress, pstCurrentWindowBufferAddress,
      iOverlappedWidth * sizeof(COLOR));

    pstCurrentVideoMemoryAddress += iScreenWidth;
    pstCurrentWindowBufferAddress += iWindowWidth;
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

BOOL kDrawWindowTitle(QWORD qwWindowID, const char* pcTitle)
{
  WINDOW* pstWindow;
  int iWidth;
  int iHeight;
  int iX;
  int iY;
  RECT stArea;
  RECT stButtonArea;

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


  // Fill Title Area
  kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
    0, 3, iWidth - 1, WINDOW_TITLEBAR_HEIGHT - 1,
    WINDOW_COLOR_TITLEBARBACKGROUND, TRUE);

  // Draw Title Text
  kInternalDrawText(&stArea, pstWindow->pstWindowBuffer,
    6, 3, WINDOW_COLOR_TITLEBARTEXT, WINDOW_COLOR_TITLEBARBACKGROUND,
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
  kRedrawWindowByArea(&stPreviousArea);

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

  kUnlock(&pstWindow->stLock);
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

  kUnlock(&pstWindow->stLock);
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

  kUnlock(&pstWindow->stLock);
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

  kUnlock(&pstWindow->stLock);
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

  kUnlock(&pstWindow->stLock);
  // --- CRITCAL SECTION END ---

  return TRUE;
}