#include <stdarg.h>
#include "Console.h"
#include "Utility.h"

CONSOLEMANAGER gs_stConsoleManager = { 0, };
static CHARACTER gs_vstScreenBuffer[CONSOLE_WIDTH * CONSOLE_HEIGHT];
static KEYDATA gs_vstKeyQueueBuffer[CONSOLE_GUIKEYQUEUE_MAXCOUNT];
/*
  Init Console
*/
void kInitializeConsole(int iX, int iY)
{
  // init Console Manager
  kMemSet(&gs_stConsoleManager, 0, sizeof(gs_stConsoleManager));

  // Init Screen Buffer
  kMemSet(&gs_vstScreenBuffer, 0, sizeof(gs_vstScreenBuffer));

  // Set Screen Buffer
  if (kIsGraphicMode() == FALSE) {
    gs_stConsoleManager.pstScreenBuffer = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
  }
  else {
    gs_stConsoleManager.pstScreenBuffer = gs_vstScreenBuffer;

    // Init Queue
    kInitializeQueue(&(gs_stConsoleManager.stKeyQueueForGUI),
      gs_vstKeyQueueBuffer, CONSOLE_GUIKEYQUEUE_MAXCOUNT, sizeof(KEYDATA));

    // Init Mutex
    kInitializeMutex(&(gs_stConsoleManager.stLock));
  }

  kSetCursor(iX, iY);
}

/*
  Set Cursor
*/
void kSetCursor(int iX, int iY)
{
  int iLinearValue;
  int i;

  // Cursor Position
  iLinearValue = iY * CONSOLE_WIDTH + iX;

  if (kIsGraphicMode() == FALSE) {
    // select uppercursor
    kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
    // write
    kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);

    // select lowercursor
    kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
    // write
    kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xFF);
  }
  else {
    // Find Cursur and remove
    for (i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
      if ((gs_stConsoleManager.pstScreenBuffer[i].bCharactor == '_') &&
        (gs_stConsoleManager.pstScreenBuffer[i].bAttribute == 0x00)) {
        gs_stConsoleManager.pstScreenBuffer[i].bCharactor = ' ';
        gs_stConsoleManager.pstScreenBuffer[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
        break;
      }
    }

    // Set New cursor
    gs_stConsoleManager.pstScreenBuffer[iLinearValue].bCharactor = '_';
    gs_stConsoleManager.pstScreenBuffer[iLinearValue].bAttribute = 0x00;
  }

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
  CHARACTER* pstScreen;
  int i, j;
  int iLength;
  int iPrintOffset;

  pstScreen = gs_stConsoleManager.pstScreenBuffer;

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
      kMemCpy(pstScreen, pstScreen + CONSOLE_WIDTH,
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
  CHARACTER* pstScreen;
  int i;

  pstScreen = gs_stConsoleManager.pstScreenBuffer;

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
    if (kIsGraphicMode() == FALSE) {
      // wait 
      while (kGetKeyFromKeyQueue(&stData) == FALSE)
      {
        kSchedule();
      }
    }
    else {
      // wait 
      while (kGetKeyFromGUIKeyQueue(&stData) == FALSE)
      {
        if (gs_stConsoleManager.bExit == TRUE) {
          return 0xFF;
        }
        kSchedule();
      }
    }

    // if key down
    if (stData.bFlags & KEY_FLAGS_DOWN)
    {
      return stData.bASCIICode;
    }
  }
}

/*
  getch (non-block);
*/
BYTE kGetChNonBlock(void)
{
  KEYDATA stData;

  if (kIsGraphicMode() == FALSE) {
    // wait 
    if (kGetKeyFromKeyQueue(&stData) == FALSE)
      return 0xFF;
  }
  else {
    // wait 
    if (kGetKeyFromGUIKeyQueue(&stData) == FALSE)
      return 0xFF;
  }

  // if key down
  if (stData.bFlags & KEY_FLAGS_DOWN)
  {
    return stData.bASCIICode;
  }
  return 0xFF;
}

/*
  Print string 
*/
void kPrintStringXY(int iX, int iY, const char* pcString)
{
  CHARACTER* pstScreen;
  int i;

  pstScreen = gs_stConsoleManager.pstScreenBuffer;

  pstScreen += (iY * CONSOLE_WIDTH) + iX;
  for (i = 0; pcString[i] != 0; i++)
  {
    pstScreen[i].bCharactor = pcString[i];
    pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
  }
}

/*
  Get console Manager
*/
CONSOLEMANAGER* kGetConsoleManager(void)
{
  return &gs_stConsoleManager;
}

/*
  Get KeyData From GUI Key Queue
*/
BOOL kGetKeyFromGUIKeyQueue(KEYDATA* pstData)
{
  BOOL bResult;

  if (kIsQueueEmpty(&(gs_stConsoleManager.stKeyQueueForGUI)) == TRUE)
  {
    return FALSE;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stConsoleManager.stLock));

  // Get from KeyQueue
  bResult = kGetQueue(&(gs_stConsoleManager.stKeyQueueForGUI), pstData);

  kUnlock(&(gs_stConsoleManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

/*
  Put KeyData To GUI Key Queue
*/
BOOL kPutKeyToGUIKeyQueue(KEYDATA* pstData)
{
  BOOL bResult;

  if (kIsQueueFull(&(gs_stConsoleManager.stKeyQueueForGUI)) == TRUE)
  {
    return FALSE;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stConsoleManager.stLock));

  // Put into KeyQueue
  bResult = kPutQueue(&(gs_stConsoleManager.stKeyQueueForGUI), pstData);

  kUnlock(&(gs_stConsoleManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

/*
  Set Shell Exit flag
*/
void kSetConsoleShellExitFlag(BOOL bFlag)
{
  gs_stConsoleManager.bExit = bFlag;
}