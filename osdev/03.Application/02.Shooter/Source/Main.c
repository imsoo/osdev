#include "Main.h"

GAMEINFO g_stGameInfo = { 0, };

// Entry Point
int Main(char* pcArgument)
{
  QWORD qwWindowID;
  EVENT stEvent;
  QWORD qwLastTickCount;
  char* pcStartMessage = "Please LButton Down To Start...";
  POINT stMouseXY;
  RECT stScreenArea;
  int iX;
  int iY;

  if (IsGraphicMode() == FALSE)
  {
    printf("This task can run only GUI Mode...\n");
    return -1;
  }

  // Create Window
  GetScreenArea(&stScreenArea);
  iX = (GetRectangleWidth(&stScreenArea) - WINDOW_WIDTH) / 2;
  iY = (GetRectangleHeight(&stScreenArea) - WINDOW_HEIGHT) / 2;
  qwWindowID = CreateWindow(iX, iY, WINDOW_WIDTH, WINDOW_HEIGHT,
    WINDOW_FLAGS_DEFAULT, "Bubble Shooter");
  if (qwWindowID == WINDOW_INVALIDID)
  {
    printf("Window create fail\n");
    return -1;
  }

  stMouseXY.iX = WINDOW_WIDTH / 2;
  stMouseXY.iY = WINDOW_HEIGHT / 2;

  // Init Game Info
  if (Initialize() == FALSE)
  {
    DeleteWindow(qwWindowID);
    return -1;
  }

  // Set Random Seed
  srand(GetTickCount());

  // Draw Game Area
  DrawInformation(qwWindowID);
  DrawGameArea(qwWindowID, &stMouseXY);
  DrawText(qwWindowID, 5, 150, RGB(255, 255, 255), RGB(0, 0, 0),
    pcStartMessage, strlen(pcStartMessage));
  ShowWindow(qwWindowID, TRUE);

  while (1)
  {
    if (ReceiveEventFromWindowQueue(qwWindowID, &stEvent) == TRUE)
    {
      switch (stEvent.qwType)
      {
      case EVENT_MOUSE_LBUTTONDOWN:

        if (g_stGameInfo.bGameStart == FALSE) {
          Initialize();
          g_stGameInfo.bGameStart = TRUE;
          break;
        }

        DeleteBubbleUnderMouse(&(stEvent.stMouseEvent.stPoint));
        memcpy(&stMouseXY, &(stEvent.stMouseEvent.stPoint), sizeof(stMouseXY));
        break;

      case EVENT_MOUSE_MOVE:
        memcpy(&stMouseXY, &(stEvent.stMouseEvent.stPoint), sizeof(stMouseXY));
        break;

      case EVENT_WINDOW_CLOSE:
        DeleteWindow(qwWindowID);
        free(g_stGameInfo.pstBubbleBuffer);
        return 0;
        break;

      default:
        break;
      }
    }

    // 50ms period
    if ((g_stGameInfo.bGameStart == TRUE) &&
      ((GetTickCount() - qwLastTickCount) > 50)) {
      qwLastTickCount = GetTickCount();

      // CreateBubble
      if ((rand() & 7) == 1) {
        CreateBubble();
      }

      // Move Bubble
      MoveBubble();

      // Draw 
      DrawGameArea(qwWindowID, &stMouseXY);
      DrawInformation(qwWindowID);

      // if GameOver
      if (g_stGameInfo.iLife <= 0)
      {
        g_stGameInfo.bGameStart = FALSE;

        DrawText(qwWindowID, 80, 130, RGB(255, 255, 255), RGB(0, 0, 0),
          "Game Over...", 13);
        DrawText(qwWindowID, 5, 150, RGB(255, 255, 255), RGB(0, 0, 0),
          pcStartMessage, strlen(pcStartMessage));
      }

      // Show
      ShowWindow(qwWindowID, TRUE);
    }
    else {
      Sleep(0);
    }
  }
  return 0;
}

/*
  Init Game Info
*/
BOOL Initialize(void)
{
  // Allocate bubble buffer
  if (g_stGameInfo.pstBubbleBuffer == NULL) {
    g_stGameInfo.pstBubbleBuffer = malloc(sizeof(BUBBLE) * MAXBUBBLECOUNT);
    if (g_stGameInfo.pstBubbleBuffer == NULL) {
      printf("Memory allocate fail\n");
    }
  }
  memset(g_stGameInfo.pstBubbleBuffer, 0, sizeof(BUBBLE) * MAXBUBBLECOUNT);
  
  g_stGameInfo.iAliveBubbleCount = 0;
  g_stGameInfo.bGameStart = FALSE;
  g_stGameInfo.qwScore = 0;
  g_stGameInfo.iLife = MAXLIFE;

  return TRUE;
}

/*
  Create bubble in bubble buffer
*/
BOOL CreateBubble(void)
{
  BUBBLE* pstTarget;
  int i;

  if (g_stGameInfo.iAliveBubbleCount >= MAXBUBBLECOUNT) {
    return FALSE;
  }

  for (i = 0; i < MAXBUBBLECOUNT; i++) {
    // Find Died bubble
    if (g_stGameInfo.pstBubbleBuffer[i].bAlive == FALSE) {
      pstTarget = &(g_stGameInfo.pstBubbleBuffer[i]);

      pstTarget->bAlive = TRUE;
      pstTarget->qwSpeed = (rand() % 8) + DEFAULTSPEED;

      pstTarget->qwX = rand() % (WINDOW_WIDTH - 2 * RADIUS) + RADIUS;
      pstTarget->qwY = INFORMATION_HEIGHT + WINDOW_TITLEBAR_HEIGHT + RADIUS + 1;

      pstTarget->stColor = RGB(rand() % 256, rand() % 256, rand() % 256);

      g_stGameInfo.iAliveBubbleCount++;
      return TRUE;
    }
  }
  return FALSE;
}

/*
  Move Bubble
*/
void MoveBubble(void)
{
  BUBBLE* pstTarget;
  int i;

  for (i = 0; i < MAXBUBBLECOUNT; i++) {
    // Find Alive bubble
    if (g_stGameInfo.pstBubbleBuffer[i].bAlive == TRUE) {
      pstTarget = &(g_stGameInfo.pstBubbleBuffer[i]);
      
      // Move
      pstTarget->qwY += pstTarget->qwSpeed;

      // if player miss bubble
      if ((pstTarget->qwY + RADIUS) >= WINDOW_HEIGHT) {
        pstTarget->bAlive = FALSE;

        g_stGameInfo.iAliveBubbleCount--;
        if (g_stGameInfo.iLife > 0) {
          g_stGameInfo.iLife--;
        }
      }
    }
  }
}

void DeleteBubbleUnderMouse(POINT* pstMouseXY)
{
  BUBBLE* pstTarget;
  int i;
  QWORD qwDistance;

  for (i = MAXBUBBLECOUNT - 1; i >= 0; i--) {
    if (g_stGameInfo.pstBubbleBuffer[i].bAlive == TRUE) {
      pstTarget = &(g_stGameInfo.pstBubbleBuffer[i]);

      qwDistance = ((pstMouseXY->iX - pstTarget->qwX) *
        (pstMouseXY->iX - pstTarget->qwX)) +
        ((pstMouseXY->iY - pstTarget->qwY) *
        (pstMouseXY->iY - pstTarget->qwY));

      // if Mouse Point in bubble
      if (qwDistance < (RADIUS * RADIUS)) {
        pstTarget->bAlive = FALSE;

        g_stGameInfo.iAliveBubbleCount--;
        g_stGameInfo.qwScore++;
        break;
      }
    }
  }
}

/*
  Draw Information
*/
void DrawInformation(QWORD qwWindowID)
{
  char vcBuffer[200];
  int iLength;

  // Draw Info Area
  DrawRect(qwWindowID, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_WIDTH - 2,
    WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT, RGB(55, 215, 47), TRUE);

  sprintf(vcBuffer, "Life: %d, Score: %d\n", g_stGameInfo.iLife,
    g_stGameInfo.qwScore);
  iLength = strlen(vcBuffer);

  // Fill Info
  DrawText(qwWindowID, (WINDOW_WIDTH - iLength * FONT_ENGLISHWIDTH) / 2,
    WINDOW_TITLEBAR_HEIGHT + 2, RGB(255, 255, 255), RGB(55, 215, 47),
    vcBuffer, strlen(vcBuffer));
}

/*
  Draw Game Area
*/
void DrawGameArea(QWORD qwWindowID, POINT* pstMouseXY)
{
  BUBBLE* pstTarget;
  int i;

  // Fill Game Area (Make Clean)
  DrawRect(qwWindowID, 0, WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT,
    WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1, RGB(0, 0, 0), TRUE);

  // Draw Bubble
  for (i = 0; i < MAXBUBBLECOUNT; i++)
  {
    if (g_stGameInfo.pstBubbleBuffer[i].bAlive == TRUE)
    {
      pstTarget = &(g_stGameInfo.pstBubbleBuffer[i]);

      DrawCircle(qwWindowID, pstTarget->qwX, pstTarget->qwY, RADIUS,
        pstTarget->stColor, TRUE);
      DrawCircle(qwWindowID, pstTarget->qwX, pstTarget->qwY, RADIUS,
        ~pstTarget->stColor, FALSE);
    }
  }

  // Draw Line of shoot ('+' symbol) on Mouse 
  if (pstMouseXY->iY < (WINDOW_TITLEBAR_HEIGHT + RADIUS))
  {
    pstMouseXY->iY = WINDOW_TITLEBAR_HEIGHT + RADIUS;
  }
  DrawLine(qwWindowID, pstMouseXY->iX, pstMouseXY->iY - RADIUS,
    pstMouseXY->iX, pstMouseXY->iY + RADIUS, RGB(255, 0, 0));
  DrawLine(qwWindowID, pstMouseXY->iX - RADIUS, pstMouseXY->iY,
    pstMouseXY->iX + RADIUS, pstMouseXY->iY, RGB(255, 0, 0));


  // Draw Game Area Frame
  DrawRect(qwWindowID, 0, WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT,
    WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1, RGB(0, 255, 0), FALSE);
}