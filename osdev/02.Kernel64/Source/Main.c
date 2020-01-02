#include "Types.h"
#include "Keyboard.h"

// function define
void kPrintString(int iX, int iY, const char* pcString);

// C Kernel 
void Main(void)
{
  char vcTemp[2] = { 0, };
  BYTE bFlags;
  BYTE bTemp;
  int i = 0;

  kPrintString(0, 10, "Switch To IA-32e Mode Success...");
  kPrintString(0, 11, "IA-32e C Language Kernel Start..............[Pass]");
  kPrintString(0, 12, "Keyboard Activate...........................[    ]");

  // key board enable
  if (kActivateKeyboard() == TRUE) {
    kPrintString(45, 12, "Pass");
    kChangeKeyboardLED(FALSE, FALSE, FALSE);
  } 
  else {
    kPrintString(45, 12, "Fail");
    while (1);
  } 

  while (1) {
    if (kIsOutputBufferFull() == TRUE) {
      bTemp = kGetKeyboardScanCode();

      if (kConvertScanCodeToASCIICode(bTemp, &(vcTemp[0]), &bFlags) == TRUE) {
        if (bFlags & KEY_FLAGS_DOWN) {
          kPrintString(i++, 13, vcTemp);
        }
      }
    }
  }
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