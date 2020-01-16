#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "List.h"
#include "Synchronization.h"
#include "2DGraphics.h"

// Window
#define WINDOW_BACKGROUNDWINDOWTITLE  "SYS_BACKGROUND"
#define WINDOW_TITLEMAXLENGTH 40
#define WINDOW_TITLEBAR_HEIGHT      21
#define WINDOW_XBUTTON_SIZE         19

// Window Color
#define WINDOW_COLOR_FRAME                  RGB( 109, 218, 22 )
#define WINDOW_COLOR_BACKGROUND             RGB( 255, 255, 255 )
#define WINDOW_COLOR_TITLEBARTEXT           RGB( 255, 255, 255 )
#define WINDOW_COLOR_TITLEBARBACKGROUND     RGB( 79, 204, 11 )
#define WINDOW_COLOR_TITLEBARBRIGHT1        RGB( 183, 249, 171 )
#define WINDOW_COLOR_TITLEBARBRIGHT2        RGB( 150, 210, 140 )
#define WINDOW_COLOR_TITLEBARUNDERLINE      RGB( 46, 59, 30 )
#define WINDOW_COLOR_BUTTONBRIGHT           RGB( 229, 229, 229 )
#define WINDOW_COLOR_BUTTONDARK             RGB( 86, 86, 86 )
#define WINDOW_COLOR_SYSTEMBACKGROUND       RGB( 232, 255, 232 )
#define WINDOW_COLOR_XBUTTONLINECOLOR       RGB( 71, 199, 21 )

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
#define MOUSE_CURSOR_OUTER                  RGB( 79, 204, 11 )
#define MOUSE_CURSOR_INNER                  RGB( 232, 255, 232 )

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
} WINDOWMANAGER;

// Window Pool
static void kInitializeWindowPool(void);
static WINDOW* kAllocateWindow(void);
static void kFreeWindow(QWORD qwID);

// Window Manager
void kInitializeGUISystem(void);

// Window
QWORD kCreateWindow(int iX, int iY, int iWidth, int iHeight, DWORD dwFlags,
  const char* pcTitle);
BOOL kDeleteWindow(QWORD qwWindowID);
BOOL kDeleteAllWindowInTaskID(QWORD qwTaskID);
WINDOW* kGetWindow(QWORD qwWindowID);
WINDOW* kGetWindowWithWindowLock(QWORD qwWindowID);
BOOL kShowWindow(QWORD qwWindowID, BOOL bShow);
BOOL kRedrawWindowByArea(const RECT* pstArea);
static void kCopyWindowBufferToFrameBuffer(const WINDOW* pstWindow, const RECT* pstCopyArea);

// Draw
BOOL kDrawWindowFrame(QWORD qwWindowID);
BOOL kDrawWindowBackground(QWORD qwWindowID);
BOOL kDrawWindowTitle(QWORD qwWindowID, const char* pcTitle);
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


#endif // !__WINDOW_H__
