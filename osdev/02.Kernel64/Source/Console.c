#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"

CONSOLEMANAGER gs_stConsoleManager = { 0, };

/*
  Init Console
*/
void kInitializeConsole(int iX, int iY)
{
  // init Console Manager
  kMemSet(&gs_stConsoleManager, 0, sizeof(gs_stConsoleManager));
  kSetCursor(iX, iY);
}

/*
  Set Cursor
*/
void kSetCursor(int iX, int iY)
{
  int iLinearValue;

  // Cursor Position
  iLinearValue = iY * CONSOLE_WIDTH + iX;

  // select uppercursor
  kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
  // write
  kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);

  // select lowercursor
  kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
  // write
  kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xFF);

  // Console Manager Update
  gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;
}

/*
  get current cursor
*/
void kGetCursor(int *piX, int *piY)
{
  *piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
  *piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}

/*
  printf
*/
void kPrintf(const char* pcFormatString, ...)
{
  va_list ap;
  char vcBuffer[1024];
  int iNextPrintOffset;

  // va argument list to vsprintf()
  va_start(ap, pcFormatString);
  kVSPrintf(vcBuffer, pcFormatString, ap);
  va_end(ap);

  // print format string
  iNextPrintOffset = kConsolePrintString(vcBuffer);

  // cursor update
  kSetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}

/*
  Console Print 
*/
int kConsolePrintString(const char* pcBuffer)
{
  CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
  int i, j;
  int iLength;
  int iPrintOffset;

  // set position to print
  iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;

  iLength = kStrLen(pcBuffer);
  for (i = 0; i < iLength; i++)
  {
    // new line 
    if (pcBuffer[i] == '\n')
    {
      iPrintOffset += (CONSOLE_WIDTH - (iPrintOffset % CONSOLE_WIDTH));
    }
    // tab
    else if (pcBuffer[i] == '\t')
    {
      iPrintOffset += (8 - (iPrintOffset % 8));
    }
    // character
    else
    {
      pstScreen[iPrintOffset].bCharactor = pcBuffer[i];
      pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
      iPrintOffset++;
    }

    // scroll
    if (iPrintOffset >= (CONSOLE_HEIGHT * CONSOLE_WIDTH))
    {
      // all line up one line except first line 
      kMemCpy(CONSOLE_VIDEOMEMORYADDRESS,
        CONSOLE_VIDEOMEMORYADDRESS + CONSOLE_WIDTH * sizeof(CHARACTER),
        (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(CHARACTER));

      // last line set blank
      for (j = (CONSOLE_HEIGHT - 1) * (CONSOLE_WIDTH);
        j < (CONSOLE_HEIGHT * CONSOLE_WIDTH); j++)
      {
        pstScreen[j].bCharactor = ' ';
        pstScreen[j].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
      }

      // position to print set last line
      iPrintOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
    }
  }
  return iPrintOffset;
}

/*
  Console Clear
*/
void kClearScreen(void)
{
  CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
  int i;

  // all Char set blank
  for (i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++)
  {
    pstScreen[i].bCharactor = ' ';
    pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
  }

  // Cursor to 0, 0
  kSetCursor(0, 0);
}

/*
  getch
*/
BYTE kGetCh(void)
{
  KEYDATA stData;
 
  while (1)
  {
    // wait 
    while (kGetKeyFromKeyQueue(&stData) == FALSE)
    {
      kSchedule();
    }

    // if key down
    if (stData.bFlags & KEY_FLAGS_DOWN)
    {
      return stData.bASCIICode;
    }
  }
}

/*
  Print string 
*/
void kPrintStringXY(int iX, int iY, const char* pcString)
{
  CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
  int i;

  pstScreen += (iY * CONSOLE_WIDTH) + iX;
  for (i = 0; pcString[i] != 0; i++)
  {
    pstScreen[i].bCharactor = pcString[i];
    pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
  }
}

