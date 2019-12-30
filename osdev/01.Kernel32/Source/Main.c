#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void kPrintString(int iX, int iY, const char* pcString);
BOOL kInitializeKernel64Area(void);
BOOL kIsMemoryEnough(void);
void kCopyKernel64ImageTo2Mbyte(void);

void Main(void)
{
  DWORD i;
  DWORD dwEAX, dwEBX, dwECX, dwEDX;
  char vcVendorString[13] = { 0, };

  kPrintString(0, 3, "C Language Kernel Start.....................[Pass]");

  // check memory space is enough 
  kPrintString(0, 4, "Minimum Memory Size Check...................[    ]");
  if (kIsMemoryEnough() == FALSE) {
    kPrintString(45, 4, "Fail");
    kPrintString(0, 5, "Not Enough Memory... Requires over 64Mbyte memory...");
    while (1);
  } else {
    kPrintString(45, 4, "Pass");
  }

  // IA-32e Kerneal Area Init
  kPrintString(0, 5, "IA-32e Kernel Area Initialize...............[    ]");
  if (kInitializeKernel64Area() == FALSE) {
    kPrintString(45, 5, "Fail");
    kPrintString(0, 6, "IA-32e Kernel Area Initialize Fail");
    while (1);
  } else {
    kPrintString(45, 5, "Pass");
  }

  // create page tables for IA-32e mode kernel
  kPrintString(0, 6, "IA-32e Page Tables Initialize...............[    ]");
  kInitializePageTables();
  kPrintString(45, 6, "Pass");

  // read cpuid
  kReadCPUID(0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX);
  *(DWORD*)vcVendorString = dwEBX;
  *((DWORD*)vcVendorString + 1) = dwEDX;
  *((DWORD*)vcVendorString + 2) = dwECX;

  kPrintString(0, 7, "Processor Vendor String.....................[            ]");
  kPrintString(45, 7, vcVendorString);

  // check 64 bit support
  kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
  kPrintString(0, 8, "64bit Mode Support Check....................[    ]");
  
  if (dwEDX & (1 << 29)) {
    kPrintString(45, 8, "Pass");
  }
  else {
    kPrintString(45, 8, "Fail");
    kPrintString(0, 9, "This processor does not support 64bit mode...");
    while (1);
  }

  // copy IA-32e kerenl to 0x200000 (2MB)
  kPrintString(0, 9, "Copy IA-32e Kernel To 2M Address............[    ]");
  kCopyKernel64ImageTo2Mbyte();
  kPrintString(45, 9, "Pass");


  // switch to IA-32e mode
  kPrintString(0, 10, "Switch To IA-32e Mode");
  kSwitchAndExecute64bitKernel();

  while (1);
}

void kPrintString(int iX, int iY, const char* pcString)
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
BOOL kInitializeKernel64Area(void)
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
BOOL kIsMemoryEnough(void)
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

/*
  Copy IA-32e kernel to 0x200000
*/
void kCopyKernel64ImageTo2Mbyte(void)
{
  WORD wKernel32SectorCount, wTotalKernelSectorCount;
  DWORD* pdwSourceAddress, * pdwDestinationAddress;
  int i;

  // 0x7C05 (total kernel sector count), 0x7C07 (kernel32 sector count)
  wTotalKernelSectorCount = *((WORD*)0x7C05);
  wKernel32SectorCount = *((WORD*)0x7C07);

  pdwSourceAddress = (DWORD*)(0x10000 + (wKernel32SectorCount * 512));
  pdwDestinationAddress = (DWORD*)0x200000;

  // copy IA-32e mode kernel 
  for (i = 0; i < 512 * (wTotalKernelSectorCount - wKernel32SectorCount) / 4; i++) {
    *pdwDestinationAddress = *pdwSourceAddress;
    pdwDestinationAddress++;
    pdwSourceAddress++;
  }
}