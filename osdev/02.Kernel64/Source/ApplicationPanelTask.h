#ifndef __APLICATIONPANELTASK_H__
#define __APLICATIONPANELTASK_H__

#include "Types.h"
#include "2DGraphics.h"
#include "Font.h"

// Application Panel
#define APPLICATIONPANEL_HEIGHT 31
#define APPLICATIONPANEL_TITLE  "SYS_APPLICATIONPANEL"
#define APPLICATIONPANEL_COLOR_OUTERLINE     RGB( 30, 30, 30 )
#define APPLICATIONPANEL_COLOR_MIDDLELINE    RGB( 30, 30, 30 )
#define APPLICATIONPANEL_COLOR_INNERLINE     RGB( 30, 30, 30 )
#define APPLICATIONPANEL_COLOR_BACKGROUND    RGB( 50, 50, 50 )
#define APPLICATIONPANEL_COLOR_INACTIVE      RGB( 30, 30, 30 )
#define APPLICATIONPANEL_COLOR_ACTIVE        RGB( 50, 50, 50 )

#define APPLICATIONPANEL_CLOCKWIDTH         ( 8 * FONT_ENGLISHWIDTH )

// Applicationo List
#define APPLICATIONPANEL_LISTITEMHEIGHT     ( FONT_ENGLISHHEIGHT + 4 )
#define APPLICATIONPANEL_LISTTITLE          "SYS_APPLICATIONLIST"

typedef struct kApplicationPanelDataStruct
{
  // Application Panel Window ID
  QWORD qwApplicationPanelID;

  // Application List Window ID
  QWORD qwApplicationListID;

  // Application Panel Button Area 
  RECT stButtonArea;

  // Application List Width
  int iApplicationListWidth;

  // Application List Index which mouse located in before
  int iPreviousMouseOverIndex;

  // Is Application window showing
  BOOL bApplicationWindowVisible
} APPLICATIONPANELDATA;

typedef struct kApplicationEntryStruct
{
  // GUI Task Name
  char* pcApplicationName;

  // Task Entry Point
  void* pvEntryPoint;
} APPLICATIONENTRY;

// function
void kApplicationPanelGUITask(void);

// Panel
static BOOL kCreateApplicationPanelWindow(void);
static void kDrawDigitalClock(QWORD qwApplicationPanelID);
static BOOL kProcessApplicationPanelWindowEvent(void);

// Application List
static BOOL kCreateApplicationListWindow(void);
static void kDrawApplicationListItem(int iIndex, BOOL bSelected);
static int kGetMouseOverItemIndex(int iMouseY);
static BOOL kProcessApplicationListWindowEvent(void);

#endif // !__APLICATIONPANELTASK_H__
