#ifndef __WINDOWMANAGER_H__
#define __WINDOWMANAGER_H__

#include "Types.h"
#include "2DGraphics.h"

#define WINDOWMANAGER_DATAACCUMULATECOUNT    20

// Resize Window
#define WINDOWMANAGER_RESIZEMARKERSIZE      20
#define WINDOWMANAGER_COLOR_RESIZEMARKER    RGB( 0, 204, 112 )
#define WINDOWMANAGER_THICK_RESIZEMARKER    4

// function
void kStartWindowManager(void);
BOOL kProcessMouseData(void);
BOOL kProcessKeyData(void);
BOOL kProcessEventQueueData(void);

void kDrawResizeMarker(const RECT* pstArea, BOOL bShowMarker);

#endif /*__WINDOWMANAGER_H__*/