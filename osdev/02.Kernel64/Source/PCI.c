#include "PCI.h"
#include "AssemblyUtility.h"

/*
  ��ġ�� Ethernet Controller �� ��� True�� �����մϴ�.
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

  // ��ġ ������ General Device �̸� 
  // Ethernet Controller ���� �˻��մϴ�.
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
    (bOffset & 0xfc) |    // Register Offset (0, 1 ��Ʈ�� �׻� 0)
    (DWORD)0x80000000     // Enable Bit
  );

  // Configuration address I/O Port (0xCF8)�� �� ����
  kOutPortDWord(PCI_CONFIG_ADDRESS, dwAddress);

  // Configuration Data I/O Port (0xCFC) ���� �� �б�
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
    (bOffset & 0xfc) |    // Register Offset (0, 1 ��Ʈ�� �׻� 0)
    (DWORD)0x80000000     // Enable Bit
    );

  // Configuration address I/O Port (0xCF8)�� �� ����
  kOutPortDWord(PCI_CONFIG_ADDRESS, dwAddress);

  // Configuration Data I/O Port (0xCFC)�� �� ����
  kOutPortDWord(PCI_CONFIG_DATA, wValue);
}