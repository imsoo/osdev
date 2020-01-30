#include "Ethernet.h"
#include "E1000.h"
#include "PCI.h"
#include "DynamicMemory.h"
#include "AssemblyUtility.h"
#include "Utility.h"
#include "IP.h"
#include "ARP.h"
#include "Console.h" // for temp

static ETHERNETMANAGER gs_stEthernetManager = { 0, };
static BYTE gs_vbBroadCast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

BOOL kEthernet_DownDirectionPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_IN;
  if (kPutQueue(&(gs_stEthernetManager.stFrameQueue), &stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

void kEthernet_Task(void)
{
  FRAME stFrame;
  ETHERNET_HEADER stEthernetHeader, *pstEthernetHeader;
  BYTE* pbEthernetPayload;
  QWORD qwDestinationHardwareAddress = 0;

  // �ʱ�ȭ
  if (kEthernet_Initialize() == FALSE)
    return;

  // Ethernet Frame
  gs_stEthernetManager.pfGetAddress(stEthernetHeader.vbSourceMACAddress);

  kMemCpy(stEthernetHeader.vbDestinationMACAddress, gs_vbBroadCast, 6);

  while (1)
  {
    if (kGetQueue(&(gs_stEthernetManager.stFrameQueue), &stFrame) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_OUT:
      kDecapuslationFrame(&stFrame, &pstEthernetHeader, sizeof(ETHERNET_HEADER), &pbEthernetPayload);

      kPrintf("Ethernet | Receive Frame | wType : %x\n", pstEthernetHeader->wType);


      if (ntohs(pstEthernetHeader->wType) == ETHERNET_HEADER_TYPE_ARP) {
        gs_stEthernetManager.pfSideOut(stFrame);
      }
      else if (ntohs(pstEthernetHeader->wType) == ETHERNET_HEADER_TYPE_IP) {
        gs_stEthernetManager.pfUp(stFrame);
      }
      else {
        kPrintf("Ethernet | Unkown Packet | Type : %d\n", pstEthernetHeader->wType);
      }
      break;
    case FRAME_IN:
      kPrintf("Ethernet | Send Frame | bType : %x\n", stFrame.bType);

      switch (stFrame.bType)
      {
      case FRAME_ARP:
        stEthernetHeader.wType = htons(ETHERNET_HEADER_TYPE_ARP);

        kNumberToAddressArray(stEthernetHeader.vbDestinationMACAddress, stFrame.qwDestAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
        kEncapuslationFrame(&stFrame, &stEthernetHeader, sizeof(ETHERNET_HEADER), NULL, 0);
        gs_stEthernetManager.pfSend(stFrame.pbCur, stFrame.wLen);

        kFreeFrame(&stFrame);
        break;
      case FRAME_IP:
        stEthernetHeader.wType = htons(ETHERNET_HEADER_TYPE_IP);

        // ARP ���̺� �������� �ʴ� ��� 
        qwDestinationHardwareAddress = kARP_GetHardwareAddress(stFrame.qwDestAddress);
        if (qwDestinationHardwareAddress == 0) {
          // ť�� �����Ͽ� ��õ�
          kPutQueue(&(gs_stEthernetManager.stFrameQueue), &stFrame);
        }
        // ���� �ϴ� ��� ����
        else {
          kNumberToAddressArray(stEthernetHeader.vbDestinationMACAddress, qwDestinationHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);

          // ����
          kEncapuslationFrame(&stFrame, &stEthernetHeader, sizeof(ETHERNET_HEADER), NULL, 0);
          gs_stEthernetManager.pfSend(stFrame.pbCur, stFrame.wLen);

          kFreeFrame(&stFrame);
        }

        break;
      default:
        break;
      }

      break;
    }
  }
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

  // Allocate Frame Queue
  gs_stEthernetManager.pstFrameBuffer= (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stEthernetManager.pstFrameBuffer == NULL) {
    kPrintf("kEthernet_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // Inint Frame Queue
  kInitializeQueue(&(gs_stEthernetManager.stFrameQueue), gs_stEthernetManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // ���̾� ����
  gs_stEthernetManager.pfUp = kIP_UpDirectionPoint;
  gs_stEthernetManager.pfSideOut = kARP_SideInPoint;

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
      gs_stEthernetManager.pfRecevie = kE1000_Receive;
      gs_stEthernetManager.pfHandler = kE1000_Handler;
      gs_stEthernetManager.pfGetAddress = kE1000_ReadMACAddress;
      gs_stEthernetManager.pfLinkUp = kE1000_SetLinkUp;
    }
    return TRUE;
  default:
    kPrintf("Could not find ethernet driver...\n");
    return FALSE;
  }
}

void kEthernet_Handler(void)
{
  HANDLERSTATUS eStatus;
  FRAME stFrame;

  eStatus = gs_stEthernetManager.pfHandler();
  
  switch (eStatus)
  {
  case HANDLER_LSC:
    gs_stEthernetManager.pfLinkUp();
    break;
  case HANDLER_RXDMT:
    // do nothing
    break;
  case HANDLER_RXT0:
    if (kAllocateFrame(&stFrame) == TRUE) {
      gs_stEthernetManager.pfRecevie(&stFrame);
      stFrame.eDirection = FRAME_OUT;
      kPutQueue(&(gs_stEthernetManager.stFrameQueue), &stFrame);
    }
    break;
  case HANDLER_UNKNOWN:
    // do nothing
    break;
  }
}