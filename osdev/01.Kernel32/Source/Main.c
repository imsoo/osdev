#include "Types.h"

void KPrintString(int iX, int iY, const char* pcString);
BOOL KInitializeKernel64Area(void);
BOOL KIsMemoryEnough(void);

void Main(void)
{
  DWORD i;

  KPrintString(0, 3, "C Language Kernel Start................[Pass]");

  // check memory space is enough 
  KPrintString(0, 4, "Minimum Memory Size Check..............[    ]");
  if (KIsMemoryEnough() == FALSE)
  {
    KPrintString(40, 4, "Fail");
    KPrintString(0, 5, "Not Enough Memory... Requires over 64Mbyte memory...");
    while (1);
  }
  else 
  {
    KPrintString(40, 4, "Pass");
  }
  // IA-32e Kerneal Area Init
  KPrintString(0, 5, "IA-32e Kernel Area Initialize..........[    ]");
  if (KInitializeKernel64Area() == FALSE)
  {
    KPrintString(40, 5, "Fail");
    KPrintString(0, 6, "IA-32e Kernel Area Initialize Fail");
    while (1);
  }
  else 
  {
    KPrintString(40, 5, "Pass");
  }

  while (1);
}

void KPrintString(int iX, int iY, const char* pcString)
{
  CHARACTER* pstScreen = (CHARACTER*)0xB8000;
  int i;

  pstScreen += (iY * 80) + iX;
  for (i = 0; pcString[i] != 0; i++)
  {
    pstScreen[i].bCharactor = pcString[i];
  }
}

/*
IA-32e kernel Area (0x100000 ~ 0x600000) Init (set up 0)
*/
BOOL KInitializeKernel64Area(void)
{
  DWORD* pdwCurrentAddress;

  // addr 0x100000 (1MB)
  pdwCurrentAddress = (DWORD*)0x100000;

  while ((DWORD)pdwCurrentAddress < 0x600000)
  {
    *pdwCurrentAddress = 0x00;

    // if writing is not working
    if (*pdwCurrentAddress != 0)
    {
      return FALSE;
    }

    pdwCurrentAddress++;
  }
  return TRUE;
}

/*
check that Machine has enough memory to load Kernel
*/
BOOL KIsMemoryEnough(void)
{
  DWORD* pdwCurrentAddress;
  // begin 0x100000
  pdwCurrentAddress = (DWORD*)0x100000;

  while ((DWORD)pdwCurrentAddress < 0x4000000)
  {
    *pdwCurrentAddress = 0x12345678;

    // if writing is not working 
    if (*pdwCurrentAddress != 0x12345678)
    {
      return FALSE;
    }

    // step 1MB
    pdwCurrentAddress += (0x100000 / 4);
  }
  return TRUE;
}