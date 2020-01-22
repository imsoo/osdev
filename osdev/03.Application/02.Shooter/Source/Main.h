#ifndef __MAIN_H__
#define __MAIN_H__

#include "OSLibrary.h"

#define MAXBUBBLECOUNT      50
#define RADIUS              16
#define MAXLIFE             20
#define DEFAULTSPEED        3

#define WINDOW_WIDTH        250
#define WINDOW_HEIGHT       350
#define INFORMATION_HEIGHT  20

typedef struct BubbleStruct
{
  // Coor
  QWORD qwX;
  QWORD qwY;

  // Falling Speed
  QWORD qwSpeed;

  // Bubble Color
  COLOR stColor;

  // Bubble is Showing
  BOOL bAlive
} BUBBLE;

typedef struct GameInfoStruct
{
  // Bubble Buffer
  BUBBLE* pstBubbleBuffer;

  // Bubble Count in GameBoard
  int iAliveBubbleCount;

  // Life
  int iLife;

  // Score
  QWORD qwScore;

  // GameStart Flag
  BOOL bGameStart;
} GAMEINFO;

// function
BOOL Initialize(void);
BOOL CreateBubble(void);
void MoveBubble(void);
void DeleteBubbleUnderMouse(POINT* pstMouseXY);

void DrawInformation(QWORD qwWindowID);
void DrawGameArea(QWORD qwWindowID, POINT* pstMouseXY);

#endif // !__MAIN_H__
