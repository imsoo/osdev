#include "OSLibrary.h"
#include "Main.h"

// Entry Point
int Main(char* pcArgument)
{
  QWORD qwWindowID;
  int iX;
  int iY;
  int iWidth;
  int iHeight;
  TEXTINFO stInfo;
  int iMoveLine;
  EVENT stReceivedEvent;
  KEYEVENT* pstKeyEvent;
  WINDOWEVENT* pstWindowEvent;
  DWORD dwFileOffset;
  RECT stScreenArea;

  if (IsGraphicMode() == FALSE)
  {
    printf("This task can run only GUI Mode...\n");
    return -1;
  }

  // Check Text FileName
  if (strlen(pcArgument) == 0)
  {
    printf("ex) exec textviwer.elf abc.txt\n");
    return 0;
  }

  // Allocate Buffer and Read
  if (ReadFileToBuffer(pcArgument, &stInfo) == FALSE)
  {
    printf("%s file is not found\n", pcArgument);
    return 0;
  }

  // Create Window
  GetScreenArea(&stScreenArea);
  iWidth = 500;
  iHeight = 500;
  iX = (GetRectangleWidth(&stScreenArea) - iWidth) / 2;
  iY = (GetRectangleHeight(&stScreenArea) - iHeight) / 2;
  qwWindowID = CreateWindow(iX, iY, iWidth, iHeight, WINDOW_FLAGS_DEFAULT |
    WINDOW_FLAGS_RESIZABLE, "Text Viewer");

  // Calc Line Offset and Set Line Index to 0 (begin)
  CalculateFileOffsetOfLine(iWidth, iHeight, &stInfo);
  stInfo.iCurrentLineIndex = 0;

  // Print Text
  DrawTextBuffer(qwWindowID, &stInfo);

  while (1)
  {
    if (ReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
    {
      Sleep(10);
      continue;
    }

    switch (stReceivedEvent.qwType)
    {
    case EVENT_KEY_DOWN:
      pstKeyEvent = &(stReceivedEvent.stKeyEvent);
      if (pstKeyEvent->bFlags & KEY_FLAGS_DOWN)
      {
        switch (pstKeyEvent->bASCIICode)
        {
          // Page Up, Page Down Move Max Line in Window
        case KEY_PAGEUP:
          iMoveLine = -stInfo.iRowCount;
          break;
        case KEY_PAGEDOWN:
          iMoveLine = stInfo.iRowCount;
          break;
          // Up, Down Move One Line
        case KEY_UP:
          iMoveLine = -1;
          break;
        case KEY_DOWN:
          iMoveLine = 1;
          break;

        default:
          iMoveLine = 0;
          break;
        }

        // Check Invalid Move
        if (stInfo.iCurrentLineIndex + iMoveLine < 0)
        {
          iMoveLine = -stInfo.iCurrentLineIndex;
        }
        else if (stInfo.iCurrentLineIndex + iMoveLine >= stInfo.iMaxLineCount)
        {
          iMoveLine = stInfo.iMaxLineCount - stInfo.iCurrentLineIndex - 1;
        }

        if (iMoveLine == 0)
        {
          break;
        }

        // Move Line and Show
        stInfo.iCurrentLineIndex += iMoveLine;
        DrawTextBuffer(qwWindowID, &stInfo);
      }
      break;

    case EVENT_WINDOW_RESIZE:
      pstWindowEvent = &(stReceivedEvent.stWindowEvent);
      iWidth = GetRectangleWidth(&(pstWindowEvent->stArea));
      iHeight = GetRectangleHeight(&(pstWindowEvent->stArea));

      // Store Current File Offset
      dwFileOffset = stInfo.pdwFileOffsetOfLine[stInfo.iCurrentLineIndex];

      // Re Calc FileOffsetOf Line
      CalculateFileOffsetOfLine(iWidth, iHeight, &stInfo);

      // Set New Current Line Index Using File Offset
      stInfo.iCurrentLineIndex = dwFileOffset / stInfo.iColumnCount;

      // Re Print Text
      DrawTextBuffer(qwWindowID, &stInfo);
      break;

    case EVENT_WINDOW_CLOSE:
      DeleteWindow(qwWindowID);
      free(stInfo.pbFileBuffer);
      free(stInfo.pdwFileOffsetOfLine);
      return 0;
      break;

    default:
      break;
    }
  }

  return 0;
}


/*
  Allocate Buffer and Read From File
*/
BOOL ReadFileToBuffer(const char* pcFileName, TEXTINFO* pstInfo)
{
  DIR* pstDirectory;
  struct dirent* pstEntry;
  DWORD dwFileSize;
  FILE* pstFile;
  DWORD dwReadSize;

  // Find File
  pstDirectory = opendir("/");
  dwFileSize = 0;
  while (1)
  {
    pstEntry = readdir(pstDirectory);
    if (pstEntry == NULL)
    {
      break;
    }

    // Compare filename
    if ((strlen(pstEntry->d_name) == strlen(pcFileName)) &&
      (memcmp(pstEntry->d_name, pcFileName, strlen(pcFileName))
        == 0))
    {
      dwFileSize = pstEntry->dwFileSize;
      break;
    }
  }
  closedir(pstDirectory);

  if (dwFileSize == 0)
  {
    printf("%s file doesn't exist or size is zero\n",
      pcFileName);
    return FALSE;
  }

  // Set Filename
  memcpy(&(pstInfo->vcFileName), pcFileName, sizeof(pstInfo->vcFileName));
  pstInfo->vcFileName[sizeof(pstInfo->vcFileName) - 1] = '\0';

  // Allocate Line Offset buffer
  pstInfo->pdwFileOffsetOfLine = malloc(MAXLINECOUNT * sizeof(DWORD));
  if (pstInfo->pdwFileOffsetOfLine == NULL)
  {
    printf("Memory allocation fail\n");
    return FALSE;
  }

  // Allocate File Buffer
  pstInfo->pbFileBuffer = (BYTE*)malloc(dwFileSize);
  if (pstInfo->pbFileBuffer == NULL)
  {
    printf("Memory %dbytes allocate fail\n", dwFileSize);
    free(pstInfo->pdwFileOffsetOfLine);
    return FALSE;
  }

  // Read From File and Write to File Buffer
  pstFile = fopen(pcFileName, "r");
  if ((pstFile != NULL) &&
    (fread(pstInfo->pbFileBuffer, 1, dwFileSize, pstFile) == dwFileSize))
  {
    fclose(pstFile);
    printf("%s file read success\n", pcFileName);
  }
  else
  {
    printf("%s file read fail\n", pcFileName);
    free(pstInfo->pdwFileOffsetOfLine);
    free(pstInfo->pbFileBuffer);
    fclose(pstFile);
    return FALSE;
  }

  // Set File Size 
  pstInfo->dwFileSize = dwFileSize;
  return TRUE;
}

/*
  Calc File Offset Of Line and Fill File Offset Line Buffer
*/
void CalculateFileOffsetOfLine(int iWidth, int iHeight, TEXTINFO* pstInfo)
{
  DWORD i;
  int iLineIndex;
  int iColumnIndex;

  // Calc Column, Row Character Count
  pstInfo->iColumnCount = (iWidth - MARGIN * 2) / FONT_ENGLISHWIDTH;
  pstInfo->iRowCount = (iHeight - (WINDOW_TITLEBAR_HEIGHT * 2) -
    (MARGIN * 2)) / FONT_ENGLISHHEIGHT;

  // Calc Line Offset
  iLineIndex = 0;
  iColumnIndex = 0;
  pstInfo->pdwFileOffsetOfLine[0] = 0;
  for (i = 0; i < pstInfo->dwFileSize; i++)
  {
    // Line Feed : Pass
    if (pstInfo->pbFileBuffer[i] == '\r')
    {
      continue;
    }
    // Tab : align by Tab Size
    else if (pstInfo->pbFileBuffer[i] == '\t')
    {
      iColumnIndex = iColumnIndex + TABSPACE;
      iColumnIndex -= iColumnIndex % TABSPACE;
    }
    else
    {
      iColumnIndex++;
    }

    // New Line OR Column Index is over Column Count
    if ((iColumnIndex >= pstInfo->iColumnCount) ||
      (pstInfo->pbFileBuffer[i] == '\n'))
    {
      iLineIndex++;
      iColumnIndex = 0;

      // Fill Buffer
      if (i + 1 < pstInfo->dwFileSize)
      {
        pstInfo->pdwFileOffsetOfLine[iLineIndex] = i + 1;
      }

      if (iLineIndex >= MAXLINECOUNT)
      {
        break;
      }
    }
  }

  // Set MaxLine
  pstInfo->iMaxLineCount = iLineIndex;
}

/*
  Draw Text 
*/
BOOL DrawTextBuffer(QWORD qwWindowID, TEXTINFO* pstInfo)
{
  DWORD i;
  DWORD j;
  DWORD dwBaseOffset;
  BYTE bTemp;
  int iXOffset;
  int iYOffset;
  int iLineCountToPrint;
  int iColumnCountToPrint;
  char vcBuffer[100];
  RECT stWindowArea;
  int iLength;
  int iWidth;
  int iColumnIndex;

  iXOffset = MARGIN;
  iYOffset = WINDOW_TITLEBAR_HEIGHT;
  GetWindowArea(qwWindowID, &stWindowArea);

  // Draw File Info
  iWidth = GetRectangleWidth(&stWindowArea);
  DrawRect(qwWindowID, 2, iYOffset, iWidth - 3, WINDOW_TITLEBAR_HEIGHT * 2,
    RGB(55, 215, 47), TRUE);

  sprintf(vcBuffer, "File: %s, Line: %d/%d\n", pstInfo->vcFileName,
    pstInfo->iCurrentLineIndex + 1, pstInfo->iMaxLineCount);
  iLength = strlen(vcBuffer);
  DrawText(qwWindowID, (iWidth - iLength * FONT_ENGLISHWIDTH) / 2,
    WINDOW_TITLEBAR_HEIGHT + 2, RGB(255, 255, 255), RGB(55, 215, 47),
    vcBuffer, strlen(vcBuffer));

  // Draw File Content
  iYOffset = (WINDOW_TITLEBAR_HEIGHT * 2) + MARGIN;
  DrawRect(qwWindowID, iXOffset, iYOffset, iXOffset + FONT_ENGLISHWIDTH *
    pstInfo->iColumnCount, iYOffset + FONT_ENGLISHHEIGHT * pstInfo->iRowCount,
    RGB(255, 255, 255), TRUE);

  // Print Text
  iLineCountToPrint = MIN(pstInfo->iRowCount,
    (pstInfo->iMaxLineCount - pstInfo->iCurrentLineIndex));
  for (j = 0; j < iLineCountToPrint; j++)
  {
    dwBaseOffset = pstInfo->pdwFileOffsetOfLine[pstInfo->iCurrentLineIndex + j];

    iColumnCountToPrint = MIN(pstInfo->iColumnCount,
      (pstInfo->dwFileSize - dwBaseOffset));
    iColumnIndex = 0;
    for (i = 0; (i < iColumnCountToPrint) &&
      (iColumnIndex < pstInfo->iColumnCount); i++)
    {
      bTemp = pstInfo->pbFileBuffer[i + dwBaseOffset];
      // New Line : Break
      if (bTemp == '\n')
      {
        break;
      }
      // Tap : Align Index
      else if (bTemp == '\t')
      {
        iColumnIndex = iColumnIndex + TABSPACE;
        iColumnIndex -= iColumnIndex % TABSPACE;
      }
      // Line Feed : Pass
      else if (bTemp == '\r')
      {
        // Nothing
      }
      else
      {
        DrawText(qwWindowID, iColumnIndex * FONT_ENGLISHWIDTH + iXOffset,
          iYOffset + (j * FONT_ENGLISHHEIGHT), RGB(0, 0, 0),
          RGB(255, 255, 255), &bTemp, 1);
        iColumnIndex++;
      }
    }
  }

  // Update Window
  ShowWindow(qwWindowID, TRUE);

  return TRUE;
}
