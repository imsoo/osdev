#include "ApplicationPanelTask.h"
#include "Window.h"
#include "GUITask.h"
#include "Task.h"
#include "RTC.h" // for Clock

static APPLICATIONPANELDATA gs_stApplicationPanelData;

APPLICATIONENTRY gs_vstApplicationTable[] =
{
  { "Base GUI Task", kBaseGUITask },
  { "Hello World GUI Task",  kHelloWorldGUITask },
  {"System Monitor", kSystemMonitorTask },
};

void kApplicationPanelGUITask(void)
{
  EVENT stReceivedEvent;
  BOOL bApplicationPanelEventResult;
  BOOL bApplicationListEventResult;

  // Check GUI Enabled
  if (kIsGraphicMode() == FALSE) {
    kPrintf("This task can run only GUI Mode...\n");
    return;
  }

  // create Window
  if ((kCreateApplicationPanelWindow() == FALSE) ||
      (kCreateApplicationListWindow() == FALSE)) {
    return;
  }

  // Event Loop
  while (1) {
    bApplicationPanelEventResult = kProcessApplicationPanelWindowEvent();
    bApplicationListEventResult = kProcessApplicationListWindowEvent();

    if ((bApplicationListEventResult == FALSE) ||
        (bApplicationPanelEventResult == FALSE)) {
      kSleep(0);
    }
  }
}

/*
  Create Application Panel Window
*/
static BOOL kCreateApplicationPanelWindow(void)
{
  WINDOWMANAGER* pstWindowManager;
  QWORD qwWindowID;

  pstWindowManager = kGetWindowManager();

  // Create Application Panel Window
  qwWindowID = kCreateWindow(0, 0,
    pstWindowManager->stScreenArea.iX2 + 1, APPLICATIONPANEL_HEIGHT,
    NULL, APPLICATIONPANEL_TITLE);
  if (qwWindowID == WINDOW_INVALIDID) {
    return FALSE;
  }

  // Draw Application Panel
  kDrawRect(qwWindowID, 0, 0, pstWindowManager->stScreenArea.iX2,
    APPLICATIONPANEL_HEIGHT - 1, APPLICATIONPANEL_COLOR_OUTERLINE, FALSE);
  kDrawRect(qwWindowID, 1, 1, pstWindowManager->stScreenArea.iX2 - 1,
    APPLICATIONPANEL_HEIGHT - 2, APPLICATIONPANEL_COLOR_MIDDLELINE, FALSE);
  kDrawRect(qwWindowID, 2, 2, pstWindowManager->stScreenArea.iX2 - 2,
    APPLICATIONPANEL_HEIGHT - 3, APPLICATIONPANEL_COLOR_INNERLINE, FALSE);
  kDrawRect(qwWindowID, 3, 3, pstWindowManager->stScreenArea.iX2 - 3,
    APPLICATIONPANEL_HEIGHT - 4, APPLICATIONPANEL_COLOR_BACKGROUND, TRUE);

  // Draw Application button
  kSetRectangleData(5, 5, 120, 25, &(gs_stApplicationPanelData.stButtonArea));
  kDrawButton(qwWindowID, &(gs_stApplicationPanelData.stButtonArea),
    APPLICATIONPANEL_COLOR_ACTIVE, "Application", RGB(255, 255, 255));

  // Draw Clock 
  kDrawDigitalClock(qwWindowID);

  // Show Window
  kShowWindow(qwWindowID, TRUE);

  gs_stApplicationPanelData.qwApplicationPanelID = qwWindowID;

  return TRUE;
}

/*
  Draw Digital Clock
*/
static void kDrawDigitalClock(QWORD qwApplicationPanelID)
{
  RECT stWindowArea;
  RECT stUpdateArea;
  static BYTE s_bPreviousHour, s_bPreviousMinute, s_bPreviousSecond;
  BYTE bHour, bMinute, bSecond;
  char vcBuffer[10] = "00:00 AM";

  // Get Time using RTC
  kReadRTCTime(&bHour, &bMinute, &bSecond);

  // if Time is Change, Update Clock
  if ((s_bPreviousHour == bHour) && (s_bPreviousMinute == bMinute) &&
    (s_bPreviousSecond == bSecond))
  {
    return;
  }

  s_bPreviousHour = bHour;
  s_bPreviousMinute = bMinute;
  s_bPreviousSecond = bSecond;

  // Hour, PM or AM
  if (bHour >= 12)
  {
    if (bHour > 12)
    {
      bHour -= 12;
    }
    vcBuffer[6] = 'P';
  }
  vcBuffer[0] = '0' + bHour / 10;
  vcBuffer[1] = '0' + bHour % 10;

  // Minute
  vcBuffer[3] = '0' + bMinute / 10;
  vcBuffer[4] = '0' + bMinute % 10;

  // Second, The colon blinks every Second.
  if ((bSecond % 2) == 1)
  {
    vcBuffer[2] = ' ';
  }
  else
  {
    vcBuffer[2] = ':';
  }

  kGetWindowArea(qwApplicationPanelID, &stWindowArea);

  // Draw Clock Area
  kSetRectangleData(stWindowArea.iX2 - APPLICATIONPANEL_CLOCKWIDTH - 13, 5,
    stWindowArea.iX2 - 5, 25, &stUpdateArea);
  kDrawRect(qwApplicationPanelID, stUpdateArea.iX1, stUpdateArea.iY1,
    stUpdateArea.iX2, stUpdateArea.iY2, APPLICATIONPANEL_COLOR_INNERLINE,
    FALSE);

  // Draw Clock
  kDrawText(qwApplicationPanelID, stUpdateArea.iX1 + 4, stUpdateArea.iY1 + 3,
    RGB(255, 255, 255), APPLICATIONPANEL_COLOR_BACKGROUND, vcBuffer,
    kStrLen(vcBuffer));

  // Update Clock Area
  kUpdateScreenByWindowArea(qwApplicationPanelID, &stUpdateArea);
}

/*
  Process Application Panel Window Event
*/
static BOOL kProcessApplicationPanelWindowEvent(void)
{
  EVENT stReceivedEvent;
  MOUSEEVENT* pstMouseEvent;
  BOOL bProcessResult;
  QWORD qwApplicationPanelID;
  QWORD qwApplicationListID;

  // Get Window ID
  qwApplicationPanelID = gs_stApplicationPanelData.qwApplicationPanelID;
  qwApplicationListID = gs_stApplicationPanelData.qwApplicationListID;
  bProcessResult = FALSE;

  while (1) {
    // Draw Clock;
    kDrawDigitalClock(gs_stApplicationPanelData.qwApplicationPanelID);

    if (kReceiveEventFromWindowQueue(qwApplicationPanelID, &stReceivedEvent) == FALSE) {
      break;
    }

    bProcessResult = TRUE;

    switch (stReceivedEvent.qwType)
    {
    case EVENT_MOUSE_LBUTTONDOWN:
      pstMouseEvent = &(stReceivedEvent.stMouseEvent);

      // if Application Button is clicked
      if (kIsInRectangle(&(gs_stApplicationPanelData.stButtonArea),
        pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY) == FALSE) {
        break;
      }

      // if List Window is Not visible 
      if (gs_stApplicationPanelData.bApplicationWindowVisible == FALSE) {

        // Chnage unactive Button to active Button
        kDrawButton(qwApplicationPanelID, &(gs_stApplicationPanelData.stButtonArea),
          APPLICATIONPANEL_COLOR_BACKGROUND, "Application",
          RGB(255, 255, 255));
        kUpdateScreenByWindowArea(qwApplicationPanelID,
          &(gs_stApplicationPanelData.stButtonArea));

        // Show Application List Window
        if (gs_stApplicationPanelData.iPreviousMouseOverIndex != -1)
        {
          kDrawApplicationListItem(
            gs_stApplicationPanelData.iPreviousMouseOverIndex, FALSE);
          gs_stApplicationPanelData.iPreviousMouseOverIndex = -1;
        }
        kMoveWindowToTop(gs_stApplicationPanelData.qwApplicationListID);
        kShowWindow(gs_stApplicationPanelData.qwApplicationListID, TRUE);
        
        // Set List Window Visible flag
        gs_stApplicationPanelData.bApplicationWindowVisible = TRUE;
      }
      else {
        // Chnage active Button to unactive Button
        kDrawButton(qwApplicationPanelID,
          &(gs_stApplicationPanelData.stButtonArea),
          APPLICATIONPANEL_COLOR_ACTIVE, "Application",
          RGB(255, 255, 255));
        kUpdateScreenByWindowArea(qwApplicationPanelID,
          &(gs_stApplicationPanelData.stButtonArea));

        // Hide Application List Window
        kShowWindow(qwApplicationListID, FALSE);

        // Set List Window Visible flag
        gs_stApplicationPanelData.bApplicationWindowVisible = FALSE;
      }
      break;

    default:
      break;
    }
  }

  return bProcessResult;
}

/*
  Create Application List Window
*/
static BOOL kCreateApplicationListWindow(void)
{
  int i;
  int iCount;
  int iMaxNameLength;
  int iNameLength;
  QWORD qwWindowID;
  int iX;
  int iY;
  int iWindowWidth;

  // Find longest Task Name in Application Table
  iMaxNameLength = 0;
  iCount = sizeof(gs_vstApplicationTable) / sizeof(APPLICATIONENTRY);
  for (i = 0; i < iCount; i++)
  {
    iNameLength = kStrLen(gs_vstApplicationTable[i].pcApplicationName);
    if (iMaxNameLength < iNameLength)
    {
      iMaxNameLength = iNameLength;
    }
  }

  // Calc Area Info 
  iWindowWidth = iMaxNameLength * FONT_ENGLISHWIDTH + 20;
  iX = gs_stApplicationPanelData.stButtonArea.iX1;
  iY = gs_stApplicationPanelData.stButtonArea.iY2 + 5;

  // Create List Window
  qwWindowID = kCreateWindow(iX, iY, iWindowWidth,
    iCount * APPLICATIONPANEL_LISTITEMHEIGHT + 1, NULL,
    APPLICATIONPANEL_LISTTITLE);
  if (qwWindowID == WINDOW_INVALIDID)
  {
    return FALSE;
  }

  // Set Panel Info
  gs_stApplicationPanelData.iApplicationListWidth = iWindowWidth;
  gs_stApplicationPanelData.bApplicationWindowVisible = FALSE;
  gs_stApplicationPanelData.qwApplicationListID = qwWindowID;
  gs_stApplicationPanelData.iPreviousMouseOverIndex = -1;

  // Draw Each Item
  for (i = 0; i < iCount; i++)
  {
    kDrawApplicationListItem(i, FALSE);
  }

  kMoveWindow(qwWindowID, gs_stApplicationPanelData.stButtonArea.iX1,
    gs_stApplicationPanelData.stButtonArea.iY2 + 5);

  return TRUE;
}

/*
  Draw Application List Item 
*/
static void kDrawApplicationListItem(int iIndex, BOOL bSelected)
{
  QWORD qwWindowID;
  int iWindowWidth;
  COLOR stColor;
  RECT stItemArea;

  // Get Area Info
  qwWindowID = gs_stApplicationPanelData.qwApplicationListID;
  iWindowWidth = gs_stApplicationPanelData.iApplicationListWidth;

  // if Item be selected, change Color
  if (bSelected == TRUE)
  {
    stColor = APPLICATIONPANEL_COLOR_ACTIVE;
  }
  else
  {
    stColor = APPLICATIONPANEL_COLOR_INACTIVE;
  }

  // Draw item Area
  kSetRectangleData(0, iIndex * APPLICATIONPANEL_LISTITEMHEIGHT,
    iWindowWidth - 1, (iIndex + 1) * APPLICATIONPANEL_LISTITEMHEIGHT,
    &stItemArea);
  kDrawRect(qwWindowID, stItemArea.iX1, stItemArea.iY1, stItemArea.iX2,
    stItemArea.iY2, APPLICATIONPANEL_COLOR_INNERLINE, FALSE);
  kDrawRect(qwWindowID, stItemArea.iX1 + 1, stItemArea.iY1 + 1,
    stItemArea.iX2 - 1, stItemArea.iY2 - 1, stColor, TRUE);

  // Draw GUI Task Name
  kDrawText(qwWindowID, stItemArea.iX1 + 10, stItemArea.iY1 + 2,
    RGB(255, 255, 255), stColor,
    gs_vstApplicationTable[iIndex].pcApplicationName,
    kStrLen(gs_vstApplicationTable[iIndex].pcApplicationName));

  kUpdateScreenByWindowArea(qwWindowID, &stItemArea);
}

/*
  Get Item Index Using Mouse Position
*/
static int kGetMouseOverItemIndex(int iMouseY)
{
  int iCount;
  int iItemIndex;

  iCount = sizeof(gs_vstApplicationTable) / sizeof(APPLICATIONENTRY);

  iItemIndex = iMouseY / APPLICATIONPANEL_LISTITEMHEIGHT;

  if ((iItemIndex < 0) || (iItemIndex >= iCount)) {
    return -1;
  }

  return iItemIndex;
}

/*
  Process Application List Window Event
*/
static BOOL kProcessApplicationListWindowEvent(void)
{
  EVENT stReceivedEvent;
  MOUSEEVENT* pstMouseEvent;
  BOOL bProcessResult;
  QWORD qwApplicationPanelID;
  QWORD qwApplicationListID;
  int iMouseOverIndex;
  EVENT stEvent;

  qwApplicationPanelID = gs_stApplicationPanelData.qwApplicationPanelID;
  qwApplicationListID = gs_stApplicationPanelData.qwApplicationListID;
  bProcessResult = FALSE;

  while (1) {
    if (kReceiveEventFromWindowQueue(qwApplicationListID, &stReceivedEvent) == FALSE) {
      break;
    }

    bProcessResult = TRUE;

    switch (stReceivedEvent.qwType)
    {
    case EVENT_MOUSE_MOVE:
      pstMouseEvent = &(stReceivedEvent.stMouseEvent);

      // Check Mouse is over Item
      iMouseOverIndex = kGetMouseOverItemIndex(pstMouseEvent->stPoint.iY);
      if ((iMouseOverIndex == gs_stApplicationPanelData.iPreviousMouseOverIndex) ||
        (iMouseOverIndex == -1)) {
        break;
      }

      // Set Previous Item FALSE (deactivate)
      if (gs_stApplicationPanelData.iPreviousMouseOverIndex != -1) {
        kDrawApplicationListItem(gs_stApplicationPanelData.iPreviousMouseOverIndex, FALSE);
      }

      // Set Current Itme TRUE (activate)
      kDrawApplicationListItem(iMouseOverIndex, TRUE);
      gs_stApplicationPanelData.iPreviousMouseOverIndex = iMouseOverIndex;

      break;

    case EVENT_MOUSE_LBUTTONDOWN:
      pstMouseEvent = &(stReceivedEvent.stMouseEvent);
      
      // Check Mouse is over Item
      iMouseOverIndex = kGetMouseOverItemIndex(pstMouseEvent->stPoint.iY);
      if (iMouseOverIndex == -1) {
        break;
      }

      // Create GUI Task
      kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, 
        (QWORD)gs_vstApplicationTable[iMouseOverIndex].pvEntryPoint, TASK_LOADBALANCINGID);

      // Send Mouse L Button Click Event to Panel Window
      // for Closing List Window
      kSetMouseEvent(qwApplicationPanelID, EVENT_MOUSE_LBUTTONDOWN,
        gs_stApplicationPanelData.stButtonArea.iX1 + 1,
        gs_stApplicationPanelData.stButtonArea.iY1 + 1,
        NULL, &stEvent);
      kSendEventToWindow(qwApplicationPanelID, &stEvent);
      break;
    default:
      break;
    }
  }

  return bProcessResult;
}