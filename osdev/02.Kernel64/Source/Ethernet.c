#include "Ethernet.h"
#include "E1000.h"
#include "PCI.h"
#include "DynamicMemory.h"
#include "AssemblyUtility.h"
#include "Utility.h"
#include "ARP.h"
#include "Console.h" // for temp

static ETHERNETMANAGER gs_stEthernetManager = { 0, };

static BYTE gs_vbBroadCast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


void kEthernet_TS(void)
{
  int i;
  ETHERNET_HEADER stEthernetFrame;
  ARP_HEADER stARPPacket;

  stARPPacket.wHardwareType = htons(0x0001);
  stARPPacket.wProtocolType = htons(0x0800);
  stARPPacket.bHadrwareAddressLength = 0x6;
  stARPPacket.bProtocolAddressLength = 0x4;
  stARPPacket.wOperation = htons(0x0001);

  for (i = 0; i < 6; i++) {
    gs_stEthernetManager.pfGetAddress(stARPPacket.vbSenderHardwareAddress);
    stARPPacket.vbTargetHardwareAddress[i] = 0xFF;
  }

  // QEMU Virtual Network Device : 10.0.2.15
  stARPPacket.vbSenderProtocolAddress[0] = 10;
  stARPPacket.vbSenderProtocolAddress[1] = 0;
  stARPPacket.vbSenderProtocolAddress[2] = 2;
  stARPPacket.vbSenderProtocolAddress[3] = 15;

  // QEMU DNS : 10.0.2.3
  stARPPacket.vbTargetProtocolAddress[0] = 10;
  stARPPacket.vbTargetProtocolAddress[1] = 0;
  stARPPacket.vbTargetProtocolAddress[2] = 2;
  stARPPacket.vbTargetProtocolAddress[3] = 3;

  // Ethernet Frame
  kMemCpy(stEthernetFrame.vbDestinationMACAddress, gs_vbBroadCast, 6);
  kMemCpy(stEthernetFrame.vbSourceMACAddress, stARPPacket.vbSenderHardwareAddress, 6);
  stEthernetFrame.wType = htons(0x0806);
  kMemCpy(stEthernetFrame.vbPayload, &stARPPacket, sizeof(stARPPacket));

  // Send
  gs_stEthernetManager.pfSend(&stEthernetFrame, 28 + 14);
}

/*
  ��� Bus�� Device�� Ž���Ͽ�
  Ethernet Controller �� ã���ϴ�.
*/
BOOL kEthernet_Initialize(void)
{
  WORD wBus;
  BYTE bDevice;
  BOOL bFound = FALSE;

  WORD wVendorID, wDeviceID, wCommand;
  DWORD dwBAR0, dwBAR1;

  QWORD qwIOAddress;
  QWORD qwMMIOAddress;

  // ��� Bus�� Device�� Ž���Ͽ�
  // Ethernet Controller �� ã���ϴ�.
  for (wBus = 0; wBus < PCI_MAX_BUSCOUNT; wBus++) {
    for (bDevice = 0; bDevice < PCI_MAX_DEVICECOUNT; bDevice++) {
      bFound = kPCI_IsEthernetController(wBus, bDevice);

      // Ethernet Controller ã�� ��� 
      if (bFound == TRUE)
      {
        wDeviceID = kPCI_ConfigReadWord(wBus, bDevice, 0, PCI_DEVICEID_OFFSET);
        wVendorID = kPCI_ConfigReadWord(wBus, bDevice, 0, PCI_VENDORID_OFFSET);

        // ����̹� ����
        if (kEthernet_SetDriver(wVendorID, wDeviceID) == TRUE) {

          // Ethernet Controller�� Base Address Register�� �о�ɴϴ�.
          dwBAR0 = kPCI_ConfigReadDWord(wBus, bDevice, 0, PCI_BAR0_OFFSET);
          dwBAR1 = kPCI_ConfigReadDWord(wBus, bDevice, 0, PCI_BAR1_OFFSET);

          // 0�� ��Ʈ�� 1�� ������ ��� (I/O ��Ʈ �ּҸ� �ǹ��մϴ�.)
          if ((dwBAR0 & 0x1) == 0x1) {
            qwIOAddress = dwBAR0;
            qwMMIOAddress = dwBAR1;
          }
          // 0���� ������ ��� (Memory Map I/O �ּҸ� �ǹ��մϴ�.)
          else {
            qwIOAddress = dwBAR1;
            qwMMIOAddress = dwBAR0;
          }

          // I/O Address 31 - 2 ��Ʈ�� ���
          qwIOAddress = qwIOAddress & 0xFFFFFFFC;

          // Memort Mapped I/O Address 31 - 4 ��Ʈ�� ���
          qwMMIOAddress = qwMMIOAddress & 0xFFFFFFF0;

          // PCI ������� Command �ʵ带 �о�ɴϴ�.
          wCommand = kPCI_ConfigReadWord(wBus, bDevice, 0, PCI_COMMAND_OFFSET);

          // Command �ʵ� �� I/O Space, Bus Master ��Ʈ�� �����մϴ�.
          wCommand |= (1 << 0);   // Bit 0 I/O Space 
          wCommand |= (1 << 2);   // Bit 2 Bus Master
          kPCI_ConfigWriteWord(wBus, bDevice, 0, PCI_COMMAND_OFFSET, wCommand);

          // ����̹� �ʱ�ȭ
          if (gs_stEthernetManager.pfInit(qwMMIOAddress, qwIOAddress) == TRUE) {
            return TRUE;
          }
        }
      }
    }
  }

  return FALSE;
}

BOOL kEthernet_SetDriver(WORD wVendor, WORD wDevice)
{
  switch (wVendor)
  {
  case VEN_INTEL:
    if ((wDevice == DEV_E1000_DEV) ||
      (wDevice == E1000_I217) ||
      (wDevice == E1000_82577LM))
    {
      gs_stEthernetManager.pfInit = kE1000_Initialize;
      gs_stEthernetManager.pfSend = kE1000_Send;
      gs_stEthernetManager.pfHandler = kE1000_Handler;
      gs_stEthernetManager.pfGetAddress = kE1000_ReadMACAddress;
    }
    return TRUE;
  default:
    kPrintf("Could not find ethernet driver...\n");
    return FALSE;
  }
}

void kEthernet_Handler(void)
{
  gs_stEthernetManager.pfHandler();
}