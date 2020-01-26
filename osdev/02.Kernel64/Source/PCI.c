#include "PCI.h"
#include "AssemblyUtility.h"

/*
  장치가 Ethernet Controller 인 경우 True를 리턴합니다.
*/
BOOL kPCI_IsEthernetController(BYTE bBus, BYTE bDevice)
{
  WORD wVendor;
  WORD wDevice;
  WORD wClass;
  WORD wHeaderType;

  DWORD dwBAR0, dwBAR1, dwBAR2, dwBAR3, dwBAR4;

  wVendor = kPCI_ConfigReadWord(bBus, bDevice, 0, PCI_VENDORID_OFFSET);
  if (wVendor == PCI_INVALID_VENDORID) {
    return;
  }

  wDevice = kPCI_ConfigReadWord(bBus, bDevice, 0, PCI_DEVICEID_OFFSET);
  wClass = kPCI_ConfigReadWord(bBus, bDevice, 0, PCI_CLASS_OFFSET);
  wHeaderType = kPCI_ConfigReadWord(bBus, bDevice, 0, PCI_CLASS_OFFSET);

  // 장치 유형이 General Device 이며 
  // Ethernet Controller 인지 검사합니다.
  if (((wHeaderType & 0xFF) == PCI_HEADER_GENERALDEVICE) &&
      (wClass == PCI_NETWORK_CLASSCODE))
  { 
    return TRUE;
  }

  return FALSE;
}

/*
  
*/
DWORD kPCI_ConfigReadDWord(BYTE bBus, BYTE bDevice, BYTE bFunc, BYTE bOffset)
{
  DWORD dwAddress;
  DWORD dwData;

  dwAddress = (DWORD)(
    (bBus << 16) |        // Bus Number
    (bDevice << 11) |     // Device Number
    (bFunc << 8) |        // Function Number
    (bOffset & 0xfc) |    // Register Offset (0, 1 비트는 항상 0)
    (DWORD)0x80000000     // Enable Bit
  );

  // Configuration address I/O Port (0xCF8)에 값 쓰기
  kOutPortDWord(PCI_CONFIG_ADDRESS, dwAddress);

  // Configuration Data I/O Port (0xCFC) 에서 값 읽기
  dwData = kInPortDWord(PCI_CONFIG_DATA);

  return dwData;
}

/*

*/
WORD kPCI_ConfigReadWord(BYTE bBus, BYTE bDevice, BYTE bFunc, BYTE bOffset)
{
  DWORD dwData;
  dwData = kPCI_ConfigReadDWord(bBus, bDevice, bFunc, bOffset);

  return (WORD)(dwData >> ((bOffset & 2) * 8)) & 0xFFFF;
}

void kPCI_ConfigWriteWord(BYTE bBus, BYTE bDevice, BYTE bFunc, BYTE bOffset, WORD wValue)
{
  DWORD dwAddress;

  dwAddress = (DWORD)(
    (bBus << 16) |        // Bus Number
    (bDevice << 11) |     // Device Number
    (bFunc << 8) |        // Function Number
    (bOffset & 0xfc) |    // Register Offset (0, 1 비트는 항상 0)
    (DWORD)0x80000000     // Enable Bit
    );

  // Configuration address I/O Port (0xCF8)에 값 쓰기
  kOutPortDWord(PCI_CONFIG_ADDRESS, dwAddress);

  // Configuration Data I/O Port (0xCFC)에 값 쓰기
  kOutPortDWord(PCI_CONFIG_DATA, wValue);
}