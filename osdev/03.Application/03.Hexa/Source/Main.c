#include "Main.h"

GAMEINFO g_stGameInfo = { 0, };

// Entry Point
int Main(char* pcArgument)
{
  QWORD qwWindowID;
  EVENT stEvent;
  KEYEVENT *pstKeyEvent;
  QWORD qwLastTickCount;
  char* pcStartMessage = "Please LButton Down To Start...";
  POINT stMouseXY;
  RECT stScreenArea;
  int iX;
  int iY;
  BYTE bBlockKind;

  if (IsGraphicMode() == FALSE)
  {
    printf("This task can run only GUI Mode...\n");
    return -1;
  }

  // Create window
  GetScreenArea(&stScreenArea);
  iX = (GetRectangleWidth(&stScreenArea) - WINDOW_WIDTH) / 2;
  iY = (GetRectangleHeight(&stScreenArea) - WINDOW_HEIGHT) / 2;
  qwWindowID = CreateWindow(iX, iY, WINDOW_WIDTH, WINDOW_HEIGHT,
    WINDOW_FLAGS_DEFAULT, "Hexa");
  if (qwWindowID == WINDOW_INVALIDID)
  {
    printf("Window create fail\n");
    return -1;
  }

  // Init Game Info
  Initialize();

  // Set Random Seed
  srand(GetTickCount());

  // Draw Game Area
  DrawInformation(qwWindowID);
  DrawGameArea(qwWindowID);
  DrawText(qwWindowID, 7, 200, RGB(255, 255, 255), RGB(0, 0, 0),
    pcStartMessage, strlen(pcStartMessage));
  ShowWindow(qwWindowID, TRUE);

  qwLastTickCount = GetTickCount();
  while (1)
  {
    if (ReceiveEventFromWindowQueue(qwWindowID, &stEvent) == TRUE)
    {
      switch (stEvent.qwType)
      {
      case EVENT_MOUSE_LBUTTONDOWN:

        if (g_stGameInfo.bGameStart == FALSE)
        {
          Initialize();

          g_stGameInfo.bGameStart = TRUE;
          break;
        }
        break;

      case EVENT_KEY_DOWN:
        pstKeyEvent = &(stEvent.stKeyEvent);
        if (g_stGameInfo.bGameStart == FALSE)
        {
          break;
        }

        switch (pstKeyEvent->bASCIICode)
        {
        case KEY_LEFT:
          if (IsMovePossible(g_stGameInfo.iBlockX - 1,
            g_stGameInfo.iBlockY) == TRUE)
          {
            g_stGameInfo.iBlockX -= 1;
            DrawGameArea(qwWindowID);
          }
          break;

        case KEY_RIGHT:
          if (IsMovePossible(g_stGameInfo.iBlockX + 1,
            g_stGameInfo.iBlockY) == TRUE)
          {
            g_stGameInfo.iBlockX += 1;
            DrawGameArea(qwWindowID);
          }
          break;

        case KEY_UP:
          bBlockKind = g_stGameInfo.vbBlock[0];
          memcpy(&(g_stGameInfo.vbBlock), &(g_stGameInfo.vbBlock[1]),
            BLOCKCOUNT - 1);
          g_stGameInfo.vbBlock[BLOCKCOUNT - 1] = bBlockKind;

          DrawGameArea(qwWindowID);
          break;

        case KEY_DOWN:
          if (IsMovePossible(g_stGameInfo.iBlockX,
            g_stGameInfo.iBlockY + 1) == TRUE)
          {
            g_stGameInfo.iBlockY += 1;
          }
          DrawGameArea(qwWindowID);
          break;

        case ' ':
          while (IsMovePossible(g_stGameInfo.iBlockX,
            g_stGameInfo.iBlockY + 1) == TRUE)
          {
            g_stGameInfo.iBlockY += 1;
          }
          DrawGameArea(qwWindowID);
          break;
        }

        ShowWindow(qwWindowID, TRUE);
        break;

      case EVENT_WINDOW_CLOSE:
        DeleteWindow(qwWindowID);
        return 0;
        break;
      }
    }

    if ((g_stGameInfo.bGameStart == TRUE) &&
      ((GetTickCount() - qwLastTickCount) >
      (300 - (g_stGameInfo.qwLevel * 10))))
    {
      qwLastTickCount = GetTickCount();

      if (IsMovePossible(g_stGameInfo.iBlockX, g_stGameInfo.iBlockY + 1) ==
        FALSE)
      {
        if (FreezeBlock(g_stGameInfo.iBlockX, g_stGameInfo.iBlockY) ==
          FALSE)
        {
          g_stGameInfo.bGameStart = FALSE;

          DrawText(qwWindowID, 82, 230, RGB(255, 255, 255), RGB(0, 0, 0),
            "Game Over....", 13);
          DrawText(qwWindowID, 7, 250, RGB(255, 255, 255), RGB(0, 0, 0),
            pcStartMessage, strlen(pcStartMessage));
        }

        EraseAllContinuousBlockOnBoard(qwWindowID);

        CreateBlock();
      }
      else
      {
        g_stGameInfo.iBlockY++;

        DrawGameArea(qwWindowID);
      }

      ShowWindow(qwWindowID, TRUE);
    }
    else
    {
      Sleep(0);
    }
  }

  return 0;
}

/*
  Init Game Info
*/
void Initialize(void)
{
  memset(&g_stGameInfo, 0, sizeof(g_stGameInfo));

  g_stGameInfo.vstBlockColor[1] = RGB(230, 0, 0);
  g_stGameInfo.vstBlockColor[2] = RGB(0, 230, 0);
  g_stGameInfo.vstBlockColor[3] = RGB(230, 0, 230);
  g_stGameInfo.vstBlockColor[4] = RGB(230, 230, 0);
  g_stGameInfo.vstBlockColor[5] = RGB(0, 230, 230);
  g_stGameInfo.vstEdgeColor[1] = RGB(150, 0, 0);
  g_stGameInfo.vstEdgeColor[2] = RGB(0, 150, 0);
  g_stGameInfo.vstEdgeColor[3] = RGB(150, 0, 150);
  g_stGameInfo.vstEdgeColor[4] = RGB(150, 150, 0);
  g_stGameInfo.vstEdgeColor[5] = RGB(0, 150, 150);

  g_stGameInfo.iBlockX = -1;
  g_stGameInfo.iBlockX = -1;
}

/*
  Create Random Color Block
*/
void CreateBlock(void)
{
  int i;

  g_stGameInfo.iBlockX = BOARDWIDTH / 2;
  g_stGameInfo.iBlockY = -BLOCKCOUNT;

  for (i = 0; i < BLOCKCOUNT; i++)
  {
    g_stGameInfo.vbBlock[i] = (rand() % BLOCKKIND) + 1;
  }
}

/*
  Check Is Coord Possible
*/
BOOL IsMovePossible(int iBlockX, int iBlockY)
{
  if ((iBlockX < 0) || (iBlockX >= BOARDWIDTH) ||
    ((iBlockY + BLOCKCOUNT) > BOARDHEIGHT))
  {
    return FALSE;
  }

  if (g_stGameInfo.vvbBoard[iBlockY + BLOCKCOUNT - 1][iBlockX] != EMPTYBLOCK)
  {
    return FALSE;
  }

  return TRUE;
}

/*
  Freeze Block, Set Board Array to FIX (COLOR)
*/
BOOL FreezeBlock(int iBlockX, int iBlockY)
{
  int i;

  if (iBlockY < 0)
  {
    return FALSE;
  }

  for (i = 0; i < BLOCKCOUNT; i++)
  {
    g_stGameInfo.vvbBoard[iBlockY + i][iBlockX] = g_stGameInfo.vbBlock[i];
  }

  g_stGameInfo.iBlockX = -1;
  return TRUE;
}

/*
  Check Horizontal Continuous Block
*/
BOOL MarkContinuousHorizonBlockOnBoard(void)
{
  int iMatchCount;
  BYTE bBlockKind;
  int i;
  int j;
  int k;
  BOOL bMarked;

  bMarked = FALSE;

  for (j = 0; j < BOARDHEIGHT; j++)
  {
    iMatchCount = 0;
    bBlockKind = 0xFF;

    for (i = 0; i < BOARDWIDTH; i++)
    {
      if ((iMatchCount == 0) &&
        (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK))
      {
        bBlockKind = g_stGameInfo.vvbBoard[j][i];
        iMatchCount++;
      }
      else
      {
        if (g_stGameInfo.vvbBoard[j][i] == bBlockKind)
        {
          iMatchCount++;
          if (iMatchCount == BLOCKCOUNT)
          {
            for (k = 0; k < iMatchCount; k++)
            {
              g_stGameInfo.vvbEraseBlock[j][i - k] = ERASEBLOCK;
            }

            bMarked = TRUE;
          }
          else if (iMatchCount > BLOCKCOUNT)
          {
            g_stGameInfo.vvbEraseBlock[j][i] = ERASEBLOCK;
          }
        }
        else
        {
          if (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK)
          {
            iMatchCount = 1;
            bBlockKind = g_stGameInfo.vvbBoard[j][i];
          }
          else
          {
            iMatchCount = 0;
            bBlockKind = 0xFF;
          }
        }
      }
    }
  }

  return bMarked;
}

/*
  Check Vertical Continuous Block
*/
BOOL MarkContinuousVerticalBlockOnBoard(void)
{
  int iMatchCount;
  BYTE bBlockKind;
  int i;
  int j;
  int k;
  BOOL bMarked;

  bMarked = FALSE;
  for (i = 0; i < BOARDWIDTH; i++)
  {
    iMatchCount = 0;
    bBlockKind = 0xFF;

    for (j = 0; j < BOARDHEIGHT; j++)
    {
      if ((iMatchCount == 0) &&
        (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK))
      {
        bBlockKind = g_stGameInfo.vvbBoard[j][i];
        iMatchCount++;
      }
      else
      {
        if (g_stGameInfo.vvbBoard[j][i] == bBlockKind)
        {
          iMatchCount++;
          if (iMatchCount == BLOCKCOUNT)
          {
            for (k = 0; k < iMatchCount; k++)
            {
              g_stGameInfo.vvbEraseBlock[j - k][i] = ERASEBLOCK;
            }

            bMarked = TRUE;
          }
          else if (iMatchCount > BLOCKCOUNT)
          {
            g_stGameInfo.vvbEraseBlock[j][i] = ERASEBLOCK;
          }
        }
        else
        {
          if (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK)
          {
            iMatchCount = 1;
            bBlockKind = g_stGameInfo.vvbBoard[j][i];
          }
          else
          {
            iMatchCount = 0;
            bBlockKind = 0xFF;
          }
        }
      }
    }
  }

  return bMarked;
}

/*
  Check Diagonal Continuous Block
*/
BOOL MarkContinuousDiagonalBlockInBoard(void)
{
  int iMatchCount;
  BYTE bBlockKind;
  int i;
  int j;
  int k;
  int l;
  BOOL bMarked;

  bMarked = FALSE;

  // Left Upper -> Right Lower
  for (i = 0; i < BOARDWIDTH; i++)
  {
    for (j = 0; j < BOARDHEIGHT; j++)
    {
      iMatchCount = 0;
      bBlockKind = 0xFF;

      for (k = 0; ((i + k) < BOARDWIDTH) && ((j + k) < BOARDHEIGHT); k++)
      {
        if ((iMatchCount == 0) &&
          (g_stGameInfo.vvbBoard[j + k][i + k] != EMPTYBLOCK))
        {
          bBlockKind = g_stGameInfo.vvbBoard[j + k][i + k];
          iMatchCount++;
        }
        else
        {
          if (g_stGameInfo.vvbBoard[j + k][i + k] == bBlockKind)
          {
            iMatchCount++;
            if (iMatchCount == BLOCKCOUNT)
            {
              for (l = 0; l < iMatchCount; l++)
              {
                g_stGameInfo.vvbEraseBlock[j + k - l][i + k - l] =
                  ERASEBLOCK;
              }
              bMarked = TRUE;
            }
            else if (iMatchCount > BLOCKCOUNT)
            {
              g_stGameInfo.vvbEraseBlock[j + k][i + k] = ERASEBLOCK;
            }
          }
          else
          {
            if (g_stGameInfo.vvbBoard[j + k][i + k] != EMPTYBLOCK)
            {
              iMatchCount = 1;
              bBlockKind = g_stGameInfo.vvbBoard[j + k][i + k];
            }
            else
            {
              iMatchCount = 0;
              bBlockKind = 0xFF;
            }
          }
        }
      }
    }
  }

  // Left Lower -> Right Upper
  for (i = 0; i < BOARDWIDTH; i++)
  {
    for (j = 0; j < BOARDHEIGHT; j++)
    {
      iMatchCount = 0;
      bBlockKind = 0xFF;

      for (k = 0; ((i + k) < BOARDWIDTH) && ((j - k) >= 0); k++)
      {
        if ((iMatchCount == 0) &&
          (g_stGameInfo.vvbBoard[j - k][i + k] != EMPTYBLOCK))
        {
          bBlockKind = g_stGameInfo.vvbBoard[j - k][i + k];
          iMatchCount++;
        }
        else
        {
          if (g_stGameInfo.vvbBoard[j - k][i + k] == bBlockKind)
          {
            iMatchCount++;
            if (iMatchCount == BLOCKCOUNT)
            {
              for (l = 0; l < iMatchCount; l++)
              {
                g_stGameInfo.vvbEraseBlock[j - k + l][i + k - l] =
                  ERASEBLOCK;
              }
              bMarked = TRUE;
            }
            else if (iMatchCount > BLOCKCOUNT)
            {
              g_stGameInfo.vvbEraseBlock[j - k][i + k] = ERASEBLOCK;
            }
          }
          else
          {
            if (g_stGameInfo.vvbBoard[j - k][i + k] != EMPTYBLOCK)
            {
              iMatchCount = 1;
              bBlockKind = g_stGameInfo.vvbBoard[j - k][i + k];
            }
            else
            {
              iMatchCount = 0;
              bBlockKind = 0xFF;
            }
          }
        }
      }
    }
  }

  return bMarked;
}

/*
  Erase Market Block (After MarkContinuousBlockInBoard)
*/
void EraseMarkedBlock(void)
{
  int i;
  int j;

  for (j = 0; j < BOARDHEIGHT; j++)
  {
    for (i = 0; i < BOARDWIDTH; i++)
    {
      if (g_stGameInfo.vvbEraseBlock[j][i] == ERASEBLOCK)
      {
        g_stGameInfo.vvbBoard[j][i] = EMPTYBLOCK;

        g_stGameInfo.qwScore++;
      }
    }
  }
}

/*
  Compact Empty Space
*/
void CompactBlockOnBoard(void)
{
  int i;
  int j;
  int iEmptyPosition;

  for (i = 0; i < BOARDWIDTH; i++)
  {
    iEmptyPosition = -1;

    for (j = BOARDHEIGHT - 1; j >= 0; j--)
    {
      if ((iEmptyPosition == -1) &&
        (g_stGameInfo.vvbBoard[j][i] == EMPTYBLOCK))
      {
        iEmptyPosition = j;
      }
      else if ((iEmptyPosition != -1) &&
        (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK))
      {
        g_stGameInfo.vvbBoard[iEmptyPosition][i] =
          g_stGameInfo.vvbBoard[j][i];

        iEmptyPosition--;

        g_stGameInfo.vvbBoard[j][i] = EMPTYBLOCK;
      }
    }
  }
}


/*
  Check Continousblock -> Erase -> Compact
*/
void EraseAllContinuousBlockOnBoard(QWORD qwWindowID)
{
  BOOL bMarked;

  memset(g_stGameInfo.vvbEraseBlock, 0, sizeof(g_stGameInfo.vvbEraseBlock));

  while (1)
  {
    Sleep(300);

    bMarked = FALSE;

    // Check Continuous
    bMarked |= MarkContinuousHorizonBlockOnBoard();
    bMarked |= MarkContinuousVerticalBlockOnBoard();
    bMarked |= MarkContinuousDiagonalBlockInBoard();

    if (bMarked == FALSE)
    {
      break;
    }

    EraseMarkedBlock();

    CompactBlockOnBoard();

    g_stGameInfo.qwLevel = (g_stGameInfo.qwScore / 30) + 1;

    DrawGameArea(qwWindowID);
    DrawInformation(qwWindowID);

    ShowWindow(qwWindowID, TRUE);
  }
}

/*
  Draw Info
*/
void DrawInformation(QWORD qwWindowID)
{
  char vcBuffer[200];
  int iLength;

  DrawRect(qwWindowID, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_WIDTH - 2,
    WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT, RGB(55, 215, 47), TRUE);

  sprintf(vcBuffer, "Level: %d, Score: %d\n", g_stGameInfo.qwLevel,
    g_stGameInfo.qwScore);
  iLength = strlen(vcBuffer);

  DrawText(qwWindowID, (WINDOW_WIDTH - iLength * FONT_ENGLISHWIDTH) / 2,
    WINDOW_TITLEBAR_HEIGHT + 2, RGB(255, 255, 255), RGB(55, 215, 47),
    vcBuffer, strlen(vcBuffer));
}

/*
  Draw Game Area
*/
void DrawGameArea(QWORD qwWindowID)
{
  COLOR stColor;
  int i;
  int j;
  int iY;

  iY = WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT;

  DrawRect(qwWindowID, 0, iY, BLOCKSIZE * BOARDWIDTH, iY + BLOCKSIZE * BOARDHEIGHT,
    RGB(0, 0, 0), TRUE);

  for (j = 0; j < BOARDHEIGHT; j++)
  {
    for (i = 0; i < BOARDWIDTH; i++)
    {
      if (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK)
      {
        stColor = g_stGameInfo.vstBlockColor[g_stGameInfo.vvbBoard[j][i]];
        DrawRect(qwWindowID, i * BLOCKSIZE, iY + (j * BLOCKSIZE),
          (i + 1) * BLOCKSIZE, iY + ((j + 1) * BLOCKSIZE),
          stColor, TRUE);

        stColor = g_stGameInfo.vstEdgeColor[g_stGameInfo.vvbBoard[j][i]];
        DrawRect(qwWindowID, i * BLOCKSIZE, iY + (j * BLOCKSIZE),
          (i + 1) * BLOCKSIZE, iY + ((j + 1) * BLOCKSIZE),
          stColor, FALSE);
        stColor = g_stGameInfo.vstEdgeColor[g_stGameInfo.vvbBoard[j][i]];
        DrawRect(qwWindowID, i * BLOCKSIZE + 1, iY + (j * BLOCKSIZE) + 1,
          (i + 1) * BLOCKSIZE - 1, iY + ((j + 1) * BLOCKSIZE) - 1,
          stColor, FALSE);
      }
    }
  }

  if (g_stGameInfo.iBlockX != -1)
  {
    for (i = 0; i < BLOCKCOUNT; i++)
    {
      if (WINDOW_TITLEBAR_HEIGHT <
        (iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE)))
      {
        stColor = g_stGameInfo.vstBlockColor[g_stGameInfo.vbBlock[i]];
        DrawRect(qwWindowID, g_stGameInfo.iBlockX * BLOCKSIZE,
          iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE),
          (g_stGameInfo.iBlockX + 1) * BLOCKSIZE,
          iY + ((g_stGameInfo.iBlockY + i + 1) * BLOCKSIZE),
          stColor, TRUE);

        stColor = g_stGameInfo.vstEdgeColor[g_stGameInfo.vbBlock[i]];
        DrawRect(qwWindowID, g_stGameInfo.iBlockX * BLOCKSIZE,
          iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE),
          (g_stGameInfo.iBlockX + 1) * BLOCKSIZE,
          iY + ((g_stGameInfo.iBlockY + i + 1) * BLOCKSIZE),
          stColor, FALSE);
        DrawRect(qwWindowID, g_stGameInfo.iBlockX * BLOCKSIZE + 1,
          iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE) + 1,
          (g_stGameInfo.iBlockX + 1) * BLOCKSIZE - 1,
          iY + ((g_stGameInfo.iBlockY + i + 1) * BLOCKSIZE) - 1,
          stColor, FALSE);
      }
    }
  }

  DrawRect(qwWindowID, 0, iY, BLOCKSIZE * BOARDWIDTH - 1,
    iY + BLOCKSIZE * BOARDHEIGHT - 1, RGB(0, 255, 0), FALSE);
}