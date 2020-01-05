#include "Utility.h"
#include "AssemblyUtility.h"

// PIT Count
volatile QWORD g_qwTickCount = 0;
QWORD kGetTickCount(void) {
  return g_qwTickCount;
}

// wait millisecond
void kSleep(QWORD qwMillisecond)
{
  QWORD qwLastTickCount;
  qwLastTickCount = g_qwTickCount;

  while ((g_qwTickCount - qwLastTickCount) <= qwMillisecond) {
    kSchedule();
  }
}

void kMemSet(void* pvDestination, BYTE bData, int iSize)
{
  int i;
  for (i = 0; i < iSize; i++) {
    ((char*)pvDestination)[i] = bData;
  }
}

int kMemCpy(void* pvDestination, const void* pvSource, int iSize)
{
  int i;
  for (i = 0; i < iSize; i++) {
    ((char*)pvDestination)[i] = ((char*)pvSource)[i];
  }
  return iSize;
}


int kMemCmp(const void* pvDestination, const void* pvSource, int iSize)
{
  int i;
  char cTemp;

  for (i = 0; i < iSize; i++) {
    cTemp = ((char*)pvDestination)[i] - ((char*)pvSource)[i];
    if (cTemp != 0) {
      return (int)cTemp;
    }
  }
  return 0;
}

BOOL kSetInterruptFlag(BOOL bEnableInterrupt)
{
  QWORD qwRFLAGS;

  qwRFLAGS = kReadRFLAGS();
  if (bEnableInterrupt == TRUE) {
    kEnableInterrupt();
  }
  else {
    kDisableInterrupt();
  }

  if (qwRFLAGS & 0x0200) {
    return TRUE;
  }
  return FALSE;
}

/*
  strlen
*/
int kStrLen(const char* pcBuffer)
{
  int i;

  for (i = 0; ; i++)
  {
    if (pcBuffer[i] == '\0')
    {
      break;
    }
  }
  return i;
}

// ram size (MB)
static gs_qwTotalRAMMBSize = 0;

/*
  check RAM Size using
  RAM write operation (start 64MB)
*/
void kCheckTotalRAMSize(void)
{
  DWORD* pdwCurrentAddress;
  DWORD dwPreviousValue;

  // 64Mbyte(0x4000000) start
  pdwCurrentAddress = (DWORD*)0x4000000;
  while (1)
  {
    // prev value backup
    dwPreviousValue = *pdwCurrentAddress;
    
    // write check
    *pdwCurrentAddress = 0x12345678;
    if (*pdwCurrentAddress != 0x12345678)
    {
      break;
    }
    *pdwCurrentAddress = dwPreviousValue;
    // next 4 MB 
    pdwCurrentAddress += (0x400000 / 4);
  }
  gs_qwTotalRAMMBSize = (QWORD)pdwCurrentAddress / 0x100000;
}

/*
  Return RAM size
*/
QWORD kGetTotalRAMSize(void)
{
  return gs_qwTotalRAMMBSize;
}

/*
  atoi
*/
long kAToI(const char* pcBuffer, int iRadix)
{
  long lReturn;

  switch (iRadix)
  {
    // hex
  case 16:
    lReturn = kHexStringToQword(pcBuffer);
    break;

    // dec
  case 10:
  default:
    lReturn = kDecimalStringToLong(pcBuffer);
    break;
  }
  return lReturn;
}

/*
  string to hex value
*/
QWORD kHexStringToQword(const char* pcBuffer)
{
  QWORD qwValue = 0;
  int i;

  // convert
  for (i = 0; pcBuffer[i] != '\0'; i++)
  {
    qwValue *= 16;
    if (('A' <= pcBuffer[i]) && (pcBuffer[i] <= 'Z'))
    {
      qwValue += (pcBuffer[i] - 'A') + 10;
    }
    else if (('a' <= pcBuffer[i]) && (pcBuffer[i] <= 'z'))
    {
      qwValue += (pcBuffer[i] - 'a') + 10;
    }
    else
    {
      qwValue += pcBuffer[i] - '0';
    }
  }
  return qwValue;
}

/*
  string to dec
*/
long kDecimalStringToLong(const char* pcBuffer)
{
  long lValue = 0;
  int i;

  // if minus
  if (pcBuffer[0] == '-')
  {
    i = 1;
  }
  else
  {
    i = 0;
  }

  // convert
  for (; pcBuffer[i] != '\0'; i++)
  {
    lValue *= 10;
    lValue += pcBuffer[i] - '0';
  }

  // if minus, add '-'
  if (pcBuffer[0] == '-')
  {
    lValue = -lValue;
  }
  return lValue;
}

/*
  itoa
*/
int kIToA(long lValue, char* pcBuffer, int iRadix)
{
  int iReturn;

  switch (iRadix)
  {
    // 16 hex
  case 16:
    iReturn = kHexToString(lValue, pcBuffer);
    break;

    // 10 dec
  case 10:
  default:
    iReturn = kDecimalToString(lValue, pcBuffer);
    break;
  }

  return iReturn;
}

/*
  hex to string
*/
int kHexToString(QWORD qwValue, char* pcBuffer)
{
  QWORD i;
  QWORD qwCurrentValue;

  // if 0
  if (qwValue == 0)
  {
    pcBuffer[0] = '0';
    pcBuffer[1] = '\0';
    return 1;
  }

  // convert 16^0, 16^1, 16^2
  for (i = 0; qwValue > 0; i++)
  {
    qwCurrentValue = qwValue % 16;
    if (qwCurrentValue >= 10)
    {
      pcBuffer[i] = 'A' + (qwCurrentValue - 10);
    }
    else
    {
      pcBuffer[i] = '0' + qwCurrentValue;
    }

    qwValue = qwValue / 16;
  }
  pcBuffer[i] = '\0';

  // reverse
  kReverseString(pcBuffer);
  return i;
}

/*
  Dec to String
*/
int kDecimalToString(long lValue, char* pcBuffer)
{
  long i;

  // if 0
  if (lValue == 0)
  {
    pcBuffer[0] = '0';
    pcBuffer[1] = '\0';
    return 1;
  }

  // if minus add '-'
  if (lValue < 0)
  {
    i = 1;
    pcBuffer[0] = '-';
    lValue = -lValue;
  }
  else
  {
    i = 0;
  }

  // convert 10^0, 10^1, 10^2
  for (; lValue > 0; i++)
  {
    pcBuffer[i] = '0' + lValue % 10;
    lValue = lValue / 10;
  }
  pcBuffer[i] = '\0';


  // reverse
  // if minus, begin index 1
  if (pcBuffer[0] == '-')
  {
    kReverseString(&(pcBuffer[1]));
  }
  else
  {
    kReverseString(pcBuffer);
  }

  return i;
}

/*
  reverse string
*/
void kReverseString(char* pcBuffer)
{
  int iLength;
  int i;
  char cTemp;

  // reverse
  iLength = kStrLen(pcBuffer);
  for (i = 0; i < iLength / 2; i++)
  {
    cTemp = pcBuffer[i];
    pcBuffer[i] = pcBuffer[iLength - 1 - i];
    pcBuffer[iLength - 1 - i] = cTemp;
  }
}

/*
  sprintf
*/
int kSPrintf(char* pcBuffer, const char* pcFormatString, ...)
{
  va_list ap;
  int iReturn;

  // va argument list to vsprintf()
  va_start(ap, pcFormatString);
  iReturn = kVSPrintf(pcBuffer, pcFormatString, ap);
  va_end(ap);

  return iReturn;
}

/*
  vsprintf
*/
int kVSPrintf(char* pcBuffer, const char* pcFormatString, va_list ap)
{
  QWORD i, j;
  int iBufferIndex = 0;
  int iFormatLength, iCopyLength;
  char* pcCopyString;
  QWORD qwValue;
  int iValue;

  // read format string and return result string
  iFormatLength = kStrLen(pcFormatString);
  for (i = 0; i < iFormatLength; i++)
  {
    // if char '%' format specifier
    if (pcFormatString[i] == '%')
    {
      // read next char
      i++;
      switch (pcFormatString[i])
      {
        // %s
      case 's':
        // char *
        pcCopyString = (char*)(va_arg(ap, char*));
        iCopyLength = kStrLen(pcCopyString);
        
        // copy string and move strlen
        kMemCpy(pcBuffer + iBufferIndex, pcCopyString, iCopyLength);
        iBufferIndex += iCopyLength;
        break;

        // %c
      case 'c':
        // copy char and move 1
        pcBuffer[iBufferIndex] = (char)(va_arg(ap, int));
        iBufferIndex++;
        break;

        // %d
      case 'd':
      case 'i':
        // itoa and move strlen
        iValue = (int)(va_arg(ap, int));
        iBufferIndex += kIToA(iValue, pcBuffer + iBufferIndex, 10);
        break;

        // 4byte %x
      case 'x':
      case 'X':
        // itoa and move strlen
        qwValue = (DWORD)(va_arg(ap, DWORD)) & 0xFFFFFFFF;
        iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
        break;

        // 8byte %q
      case 'q':
      case 'Q':
      case 'p':
        // itoa and move strlen
        qwValue = (QWORD)(va_arg(ap, QWORD));
        iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
        break;


      default:
        // copy char and move 1
        pcBuffer[iBufferIndex] = pcFormatString[i];
        iBufferIndex++;
        break;
      }
    }
    // character
    else
    {
      // copy char and move 1
      pcBuffer[iBufferIndex] = pcFormatString[i];
      iBufferIndex++;
    }
  }

  // set NULL terminate and return result string
  pcBuffer[iBufferIndex] = '\0';
  return iBufferIndex;
}