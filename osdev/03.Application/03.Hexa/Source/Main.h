#ifndef __MAIN_H__
#define __MAIN_H__

#include "OSLibrary.h"

#define BOARDWIDTH      8
#define BOARDHEIGHT     12

#define BLOCKSIZE       32
#define BLOCKCOUNT      3
#define EMPTYBLOCK      0
#define ERASEBLOCK      0xFF
#define BLOCKKIND       5

#define WINDOW_WIDTH        ( BOARDWIDTH * BLOCKSIZE )
#define WINDOW_HEIGHT       ( WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT + \
                              BOARDHEIGHT * BLOCKSIZE )

#define INFORMATION_HEIGHT  20

typedef struct GameInfoStruct
{
  // Block Color Info
  COLOR vstBlockColor[BLOCKKIND + 1];
  COLOR vstEdgeColor[BLOCKKIND + 1];

  // Coor
  int iBlockX;
  int iBlockY;

  // Fixed Block Coord 
  BYTE vvbBoard[BOARDHEIGHT][BOARDWIDTH];

  // Block Coord To Erase
  BYTE vvbEraseBlock[BOARDHEIGHT][BOARDWIDTH];

  // Current Moving Block
  BYTE vbBlock[BLOCKCOUNT];

  // GameStart Flag
  BOOL bGameStart;

  // Score
  QWORD qwScore;

  // Level
  QWORD qwLevel;
} GAMEINFO;


// function
void Initialize(void);
void CreateBlock(void);
BOOL IsMovePossible(int iBlockX, int iBlockY);
BOOL FreezeBlock(int iBlockX, int iBlockY);
void EraseAllContinuousBlockOnBoard(QWORD qwWindowID);

BOOL MarkContinuousVerticalBlockOnBoard(void);
BOOL MarkContinuousHorizonBlockOnBoard(void);
BOOL MarkContinuousDiagonalBlockInBoard(void);
void EraseMarkedBlock(void);
void CompactBlockOnBoard(void);

void DrawInformation(QWORD qwWindowID);
void DrawGameArea(QWORD qwWindowID);

#endif /* __MAIN_H__ */
