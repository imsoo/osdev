#include "Types.h"

void KPrintString(int iX, int iY, const char* pcString);

void Main(void)
{
  KPrintString(0, 3, "C Language Kernel Start...");

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