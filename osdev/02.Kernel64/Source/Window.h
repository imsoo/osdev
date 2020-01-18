#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "List.h"
#include "Queue.h"
#include "Keyboard.h"
#include "Synchronization.h"
#include "2DGraphics.h"

// Window
#define WINDOW_BACKGROUNDWINDOWTITLE  "SYS_BACKGROUND"
#define WINDOW_TITLEMAXLENGTH 40
#define WINDOW_TITLEBAR_HEIGHT      21
#define WINDOW_XBUTTON_SIZE         19

// Window Color
#define WINDOW_COLOR_FRAME                  RGB( 50, 50, 50 )
#define WINDOW_COLOR_BACKGROUND             RGB( 30, 30, 30 )
#define WINDOW_COLOR_FRAMETEXT              RGB(255, 255, 255)
#define WINDOW_COLOR_WHITE                  RGB(255, 255, 255)

#define WINDOW_COLOR_TITLEBARABACKGROUND    RGB( 45, 45, 48 )
#define WINDOW_COLOR_TITLEBARACTIVATETEXT   RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARINACTIVATETEXT RGB(100, 100, 100)


#define WINDOW_COLOR_TITLEBARBRIGHT1        RGB( 45, 45, 48 )
#define WINDOW_COLOR_TITLEBARBRIGHT2        RGB( 45, 45, 48 )
#define WINDOW_COLOR_TITLEBARUNDERLINE      RGB( 50, 50, 50 )

#define WINDOW_COLOR_BUTTONACTIVATEBACKGROUND   RGB( 75, 75, 75 )
#define WINDOW_COLOR_BUTTONINACTIVATEBACKGROUND RGB( 50, 50, 50 )

#define WINDOW_COLOR_BUTTONBRIGHT           RGB( 45, 45, 48 )
#define WINDOW_COLOR_BUTTONDARK             RGB( 45, 45, 48 )
#define WINDOW_COLOR_SYSTEMBACKGROUND       RGB( 232, 255, 232 )
#define WINDOW_COLOR_XBUTTONLINECOLOR       RGB(255, 255, 255)

#define WINDOW_MAXCOUNT 2048
#define GETWINDOWOFFSET(X)  ((X) & 0xFFFFFFFF)
#define WINDOW_INVALIDID            0xFFFFFFFFFFFFFFFF

#define WINDOW_FLAGS_SHOW           0x00000001
#define WINDOW_FLAGS_DRAWFRAME      0x00000002
#define WINDOW_FLAGS_DRAWTITLE      0x00000004
#define WINDOW_FLAGS_DEFAULT        ( WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | \
                                      WINDOW_FLAGS_DRAWTITLE )

// Mouse 
#define MOUSE_CURSOR_WIDTH                  20
#define MOUSE_CURSOR_HEIGHT                 20

// Mouse Color
#define MOUSE_CURSOR_OUTERLINE              RGB(0, 0, 0 )
#define MOUSE_CURSOR_OUTER                  RGB(255, 255, 255)
#define MOUSE_CURSOR_INNER                  RGB(255, 255, 255)

// Event
#define EVENTQUEUE_WINDOWMAXCOUNT           100
#define EVENTQUEUE_WNIDOWMANAGERMAXCOUNT    WINDOW_MAXCOUNT

// Mouse Event
#define EVENT_UNKNOWN                                   0
#define EVENT_MOUSE_MOVE                                1
#define EVENT_MOUSE_LBUTTONDOWN                         2
#define EVENT_MOUSE_LBUTTONUP                           3
#define EVENT_MOUSE_RBUTTONDOWN                         4
#define EVENT_MOUSE_RBUTTONUP                           5
#define EVENT_MOUSE_MBUTTONDOWN                         6
#define EVENT_MOUSE_MBUTTONUP                           7

// Window Event
#define EVENT_WINDOW_SELECT                             8
#define EVENT_WINDOW_DESELECT                           9
#define EVENT_WINDOW_MOVE                               10
#define EVENT_WINDOW_RESIZE                             11
#define EVENT_WINDOW_CLOSE                              12

// key Event
#define EVENT_KEY_DOWN                                  13
#define EVENT_KEY_UP                                    14

// Screen Update Event
#define EVENT_WINDOWMANAGER_UPDATESCREENBYID            15
#define EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA    16
#define EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA    17

#define WINDOW_OVERLAPPEDAREALOGMAXCOUNT                20

// Event
typedef struct kMouseEventStruct
{
  // Window ID
  QWORD qwWindowID;

  // Point & Button Status
  POINT stPoint;
  BYTE bButtonStatus;
} MOUSEEVENT;

typedef struct kKeyEventStruct
{
  // Window ID
  QWORD qwWindowID;

  // Key
  BYTE bASCIICode;
  BYTE bScanCode;
  BYTE bFlags;
} KEYEVENT;

typedef struct kWindowEventStruct
{
  // Window ID
  QWORD qwWindowID;

  // Window Info
  RECT stArea;
} WINDOWEVENT;

typedef struct kEventStruct
{
  // Event Type
  QWORD qwType;

  union 
  {
    MOUSEEVENT stMouseEvent;
    KEYEVENT stKeyEvent;
    WINDOWEVENT stWindowEvent;
    QWORD vqwData[3];
  };
} EVENT;

typedef struct kWindowStruct
{
  // Next Window & Window ID
  LISTLINK stLink;

  // Mutex
  MUTEX stLock;

  // Window Area Info
  RECT stArea;

  // Window Buffer address
  COLOR* pstWindowBuffer;

  // Task ID
  QWORD qwTaskID;

  // Window Flags
  DWORD dwFlags;

  // Window Event Queue
  QUEUE stEventQueue;
  EVENT* pstEventBuffer;

  // Window Title 
  char vcWindowTitle[WINDOW_TITLEMAXLENGTH + 1];
} WINDOW;


typedef struct kwindowPoolManagerStruct
{
  // Mutex
  MUTEX stLock;

  // Pool Start Address
  WINDOW* pstStartAddress;

  // Max Window Count
  int iMaxCount;

  // Allocated Window Count
  int iUseCount;

  // Total Allocated Count
  int iAllocatedCount;
} WINDOWPOOLMANAGER;

typedef struct kWindowManagerStruct
{
  // Mutex
  MUTEX stLock;

  // Window List
  LIST stWindowList;

  // Mouse Position
  int iMouseX;
  int iMouseY;

  // Screen Info
  RECT stScreenArea;

  // Video Memory Address
  COLOR* pstVideoMemory;

  // Background Window ID
  QWORD qwBackgroundWindowID;

  // Window Event Queue
  QUEUE stEventQueue;
  EVENT* pstEventBuffer;

  // Mouse Previous Button Status
  BYTE bPreviousButtonStatus;

  // Moving Window info
  QWORD qwMovingWindowID;
  BOOL bWindowMoveMode;

  // Bitmap for Screen Drawing
  BYTE* pbDrawBitmap;

} WINDOWMANAGER;

typedef struct kDrawBitmapStruct
{
  // Update Area Info
  RECT stArea;

  // Bitmap 
  BYTE* pbBitmap
} DRAWBITMAP;

// Window Pool
static void kInitializeWindowPool(void);
static WINDOW* kAllocateWindow(void);
static void kFreeWindow(QWORD qwID);

// Window Manager
void kInitializeGUISystem(void);
WINDOWMANAGER* kGetWindowManager(void);
QWORD kGetBackgroundWindowID(void);
void kGetScreenArea(RECT* pstScreenArea);

// Window
QWORD kCreateWindow(int iX, int iY, int iWidth, int iHeight, DWORD dwFlags,
  const char* pcTitle);
BOOL kDeleteWindow(QWORD qwWindowID);
BOOL kDeleteAllWindowInTaskID(QWORD qwTaskID);
WINDOW* kGetWindow(QWORD qwWindowID);
WINDOW* kGetWindowWithWindowLock(QWORD qwWindowID);
BOOL kShowWindow(QWORD qwWindowID, BOOL bShow);
BOOL kRedrawWindowByArea(const RECT* pstArea, QWORD qwDrawWindowID);
static void kCopyWindowBufferToFrameBuffer(const WINDOW* pstWindow, DRAWBITMAP* pstDrawBitmap);

QWORD kFindWindowByPoint(int iX, int iY);
QWORD kFindWindowByTitle(const char* pcTitle);
QWORD kGetTopWindowID(void);
BOOL kMoveWindowToTop(QWORD qwWindowID);
BOOL kMoveWindow(QWORD qwWindowID, int iX, int iY);
BOOL kIsInTitleBar(QWORD qwWindowID, int iX, int iY);
BOOL kIsInCloseButton(QWORD qwWindowID, int iX, int iY);
static BOOL kUpdateWindowTitle(QWORD qwWindowID, BOOL bSelectedTitle);

// Convert Point (Screen <-> Window)
BOOL kGetWindowArea(QWORD qwWindowID, RECT* pstArea);
BOOL kConvertPointScreenToClient(QWORD qwWindowID, const POINT* pstXY,
  POINT* pstXYInWindow);
BOOL kConvertPointClientToScreen(QWORD qwWindowID, const POINT* pstXY,
  POINT* pstXYInScreen);
BOOL kConvertRectScreenToClient(QWORD qwWindowID, const RECT* pstArea,
  RECT* pstAreaInWindow);
BOOL kConvertRectClientToScreen(QWORD qwWindowID, const RECT* pstArea,
  RECT* pstAreaInScreen);

// Event
BOOL kSendEventToWindow(QWORD qwWindowID, const EVENT* pstEvent);
BOOL kReceiveEventFromWindowQueue(QWORD qwWindowID, EVENT* pstEvent);
BOOL kSendEventToWindowManager(const EVENT* pstEvent);
BOOL kReceiveEventFromWindowManagerQueue(EVENT* pstEvent);

BOOL kSetMouseEvent(QWORD qwWindowID, QWORD qwEventType, int iMouseX, int iMouseY,
  BYTE bButtonStatus, EVENT* pstEvent);
BOOL kSetWindowEvent(QWORD qwWindowID, QWORD qwEventType, EVENT* pstEvent);
void kSetKeyEvent(QWORD qwWindow, const KEYDATA* pstKeyData, EVENT* pstEvent);

// Window Update
BOOL kUpdateScreenByID(QWORD qwWindowID);
BOOL kUpdateScreenByWindowArea(QWORD qwWindowID, const RECT* pstArea);
BOOL kUpdateScreenByScreenArea(const RECT* pstArea);

// Draw
BOOL kDrawWindowFrame(QWORD qwWindowID);
BOOL kDrawWindowBackground(QWORD qwWindowID);
BOOL kDrawWindowTitle(QWORD qwWindowID, const char* pcTitle, BOOL bSelectedTitle);
BOOL kDrawButton(QWORD qwWindowID, RECT* pstButtonArea, COLOR stBackgroundColor,
  const char* pcText, COLOR stTextColor);

static void kDrawCursor(int iX, int iY);
void kMoveCursor(int iX, int iY);
void kGetCursorPosition(int* piX, int* piY);

BOOL kDrawPixel(QWORD qwWindowID, int iX, int iY, COLOR stColor);
BOOL kDrawLine(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor);
BOOL kDrawRect(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2,
  COLOR stColor, BOOL bFill);
BOOL kDrawCircle(QWORD qwWindowID, int iX, int iY, int iRadius, COLOR stColor,
  BOOL bFill);
BOOL kDrawText(QWORD qwWindowID, int iX, int iY, COLOR stTextColor,
  COLOR stBackgroundColor, const char* pcString, int iLength);

// Draw Bitmap
BOOL kCreateDrawBitmap(const RECT* pstArea, DRAWBITMAP* pstDrawBitmap);
inline BOOL kGetStartPositionInDrawBitmap(const DRAWBITMAP* pstDrawBitmap,
  int iX, int iY, int* piByteOffset, int* piBitOffset);
static BOOL kFillDrawBitmap(DRAWBITMAP* pstDrawBitmap, RECT* pstArea, BOOL bFill);
inline BOOL kIsDrawBitmapAllOff(const DRAWBITMAP* pstDrawBitmap);


#endif // !__WINDOW_H__
