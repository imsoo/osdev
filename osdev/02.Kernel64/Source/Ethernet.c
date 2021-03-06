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
  stFrame.eDirection = FRAME_DOWN;
  if (kEthernet_PutFrameToFrameQueue(&stFrame) == FALSE)
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

  while (1)
  {
    if (kEthernet_GetFrameFromFrameQueue(&stFrame) == FALSE) {
      //kSleep(0);
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_UP:
      kDecapuslationFrame(&stFrame, &pstEthernetHeader, sizeof(ETHERNET_HEADER), &pbEthernetPayload);

      // kPrintf("Ethernet | Receive Frame | wType : %x\n", pstEthernetHeader->wType);

      if (ntohs(pstEthernetHeader->wType) == ETHERNET_HEADER_TYPE_ARP) {
        gs_stEthernetManager.pfSideOut(stFrame);
      }
      else if (ntohs(pstEthernetHeader->wType) == ETHERNET_HEADER_TYPE_IP) {
        gs_stEthernetManager.pfUp(stFrame);
      }
      else {
        // kPrintf("Ethernet | Unkown Packet | Type : %d\n", pstEthernetHeader->wType);
        kFreeFrame(&stFrame);
      }
      break;  /* End of case FRAME_UP */
    case FRAME_DOWN:
      // kPrintf("Ethernet | Send Frame | bType : %x\n", stFrame.bType);

      switch (stFrame.bType)
      {
      case FRAME_ARP:
        stEthernetHeader.wType = htons(ETHERNET_HEADER_TYPE_ARP);
        break;  /* End of case FRAME_ARP */

      case FRAME_IP:
        stEthernetHeader.wType = htons(ETHERNET_HEADER_TYPE_IP);
        break; /* End of case FRAME_IP */
      }

      // 프레임 전송
      kNumberToAddressArray(stEthernetHeader.vbDestinationMACAddress, stFrame.qwDestAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
      kEncapuslationFrame(&stFrame, &stEthernetHeader, sizeof(ETHERNET_HEADER), NULL, 0);
      gs_stEthernetManager.pfSend(stFrame.pbCur, stFrame.wLen);

      kFreeFrame(&stFrame);

      break; /* End of case FRAME_DOWN */
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

  // 스핀락 초기화
  kInitializeSpinLock(&(gs_stEthernetManager.stSpinLock));

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
      stFrame.eDirection = FRAME_UP;
      kEthernet_PutFrameToFrameQueue(&stFrame);
    }
    break;
  case HANDLER_UNKNOWN:
    // do nothing
    break;
  }
}

BOOL kEthernet_PutFrameToFrameQueue(const FRAME* pstFrame)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stEthernetManager.stSpinLock));

  // Put into FrameQueue
  bResult = kPutQueue(&(gs_stEthernetManager.stFrameQueue), pstFrame);

  kUnlockForSpinLock(&(gs_stEthernetManager.stSpinLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kEthernet_GetFrameFromFrameQueue(FRAME* pstFrame)
{
  BOOL bResult;
  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stEthernetManager.stSpinLock));

  // Get from FrameQueue
  bResult = kGetQueue(&(gs_stEthernetManager.stFrameQueue), pstFrame);

  kUnlockForSpinLock(&(gs_stEthernetManager.stSpinLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kEthernet_GetMACAddress(BYTE* pbAddress)
{
  return gs_stEthernetManager.pfGetAddress(pbAddress);
}