#include "Types.h"
#include "Window.h"
#include "WindowManagerTask.h"
#include "VBE.h"
#include "Mouse.h"
#include "Task.h"

/*
  Window Manager Task
*/
void kStartWindowManager(void)
{
  VBEMODEINFOBLOCK* pstVBEMode;
  int iRelativeX, iRelativeY;
  int iMouseX, iMouseY;
  BYTE bButton;
  QWORD qwWindowID;
  TCB* pstTask;
  char vcTempTitle[WINDOW_TITLEMAXLENGTH];
  int iWindowCount = 0;

  // Get Window Manager Task's TCB
  pstTask = kGetRunningTask(kGetAPICID());

  // Init GUI
  kInitializeGUISystem();

  // Draw Mouse
  kGetCursorPosition(&iMouseX, &iMouseY);
  kMoveCursor(iMouseX, iMouseY);

  while (1)
  {
    // Wait Mouse
    if (kGetMouseDataFromMouseQueue(&bButton, &iRelativeX, &iRelativeY) ==
      FALSE)
    {
      kSleep(0);
      continue;
    }

    // Calc Current Mouse Position
    kGetCursorPosition(&iMouseX, &iMouseY);
    iMouseX += iRelativeX;
    iMouseY += iRelativeY;

    // L Button Click
    if (bButton & MOUSE_LBUTTONDOWN)
    {
      kSPrintf(vcTempTitle, "Window %d", iWindowCount++);
      qwWindowID = kCreateWindow(iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2,
        400, 200, WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLE, vcTempTitle);

      kDrawText(qwWindowID, 10, WINDOW_TITLEBAR_HEIGHT + 10, RGB(0, 0, 0),
        WINDOW_COLOR_BACKGROUND, "Window", 7);
      kDrawText(qwWindowID, 10, WINDOW_TITLEBAR_HEIGHT + 30, RGB(0, 0, 0),
        WINDOW_COLOR_BACKGROUND, "Hello, World", 13);
      kShowWindow(qwWindowID, TRUE);
    }
    // R Button
    else if (bButton & MOUSE_RBUTTONDOWN)
    {
      // Delete All Window
      kDeleteAllWindowInTaskID(pstTask->stLink.qwID);
      iWindowCount = 0;
    }

    kMoveCursor(iMouseX, iMouseY);
  }
}

