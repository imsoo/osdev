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

  // 초기화
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

        // ARP 테이블에 존재하지 않는 경우 
        qwDestinationHardwareAddress = kARP_GetHardwareAddress(stFrame.qwDestAddress);
        if (qwDestinationHardwareAddress == 0) {
          // 큐에 삽입하여 재시도
          kPutQueue(&(gs_stEthernetManager.stFrameQueue), &stFrame);
        }
        // 존재 하는 경우 전송
        else {
          kNumberToAddressArray(stEthernetHeader.vbDestinationMACAddress, qwDestinationHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);

          // 전송
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
  모든 Bus와 Device를 탐색하여
  Ethernet Controller 를 찾습니다.
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

  // 레이어 설정
  gs_stEthernetManager.pfUp = kIP_UpDirectionPoint;
  gs_stEthernetManager.pfSideOut = kARP_SideInPoint;

  // 모든 Bus와 Device를 탐색하여
  // Ethernet Controller 를 찾습니다.
  for (wBus = 0; wBus < PCI_MAX_BUSCOUNT; wBus++) {
    for (bDevice = 0; bDevice < PCI_MAX_DEVICECOUNT; bDevice++) {
      bFound = kPCI_IsEthernetController(wBus, bDevice);

      // Ethernet Controller 찾은 경우 
      if (bFound == TRUE)
      {
        wDeviceID = kPCI_ConfigReadWord(wBus, bDevice, 0, PCI_DEVICEID_OFFSET);
        wVendorID = kPCI_ConfigReadWord(wBus, bDevice, 0, PCI_VENDORID_OFFSET);

        // 드라이버 설정
        if (kEthernet_SetDriver(wVendorID, wDeviceID) == TRUE) {

          // Ethernet Controller의 Base Address Register를 읽어옵니다.
          dwBAR0 = kPCI_ConfigReadDWord(wBus, bDevice, 0, PCI_BAR0_OFFSET);
          dwBAR1 = kPCI_ConfigReadDWord(wBus, bDevice, 0, PCI_BAR1_OFFSET);

          // 0번 비트가 1로 설정된 경우 (I/O 포트 주소를 의미합니다.)
          if ((dwBAR0 & 0x1) == 0x1) {
            qwIOAddress = dwBAR0;
            qwMMIOAddress = dwBAR1;
          }
          // 0으로 설정된 경우 (Memory Map I/O 주소를 의미합니다.)
          else {
            qwIOAddress = dwBAR1;
            qwMMIOAddress = dwBAR0;
          }

          // I/O Address 31 - 2 비트만 사용
          qwIOAddress = qwIOAddress & 0xFFFFFFFC;

          // Memort Mapped I/O Address 31 - 4 비트만 사용
          qwMMIOAddress = qwMMIOAddress & 0xFFFFFFF0;

          // PCI 헤더에서 Command 필드를 읽어옵니다.
          wCommand = kPCI_ConfigReadWord(wBus, bDevice, 0, PCI_COMMAND_OFFSET);

          // Command 필드 내 I/O Space, Bus Master 비트를 설정합니다.
          wCommand |= (1 << 0);   // Bit 0 I/O Space 
          wCommand |= (1 << 2);   // Bit 2 Bus Master
          kPCI_ConfigWriteWord(wBus, bDevice, 0, PCI_COMMAND_OFFSET, wCommand);

          // 드라이버 초기화
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