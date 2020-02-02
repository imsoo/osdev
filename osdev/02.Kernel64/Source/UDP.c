#include "UDP.h"
#include "DHCP.h"
#include "IP.h"
#include "Utility.h"

static UDPMANAGER gs_stUDPManager = { 0, };

void kUDP_Task(void)
{
  FRAME stFrame;
  IPv4Pseudo_Header stPseudoHeader = { 0, };
  UDP_HEADER stUDPHeader = { 0, };
  UDP_HEADER *pstUDPHeader;
  BYTE* pbUDPPayload;


  // 기본 UDP 헤더
  stPseudoHeader.bZeroes = 0x00;
  stPseudoHeader.bProtocol = IP_PROTOCOL_UDP;

  // 초기화
  if (kUDP_Initialize() == FALSE)
    return;

  while (1)
  {
    // 큐 확인
    if (kUDP_GetFrameFromFrameQueue(&stFrame) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_OUT:
      kPrintf("UDP | Receive UDP Datagram \n");

      kDecapuslationFrame(&stFrame, &pstUDPHeader, sizeof(UDP_HEADER), &pbUDPPayload);

      switch (ntohs(pstUDPHeader->wDestinationPort))
      {
      case UDP_PORT_DHCP_CLIENT:
        gs_stUDPManager.pfUpDHCP(stFrame);
        break;
      default:
        kFreeFrame(&stFrame);
        break;
      }

      break;  /* End of case FRAME_OUT: */
    case FRAME_IN:
      kPrintf("UDP | Send UDP Datagram \n");
      stUDPHeader.wDestinationPort = htons(stFrame.dwDestPort);
      stUDPHeader.wSourcePort = htons(stFrame.dwDestPort >> 16);
      stUDPHeader.wLength = htons(sizeof(UDP_HEADER) + stFrame.wLen);

      kNumberToAddressArray(stPseudoHeader.vbDestinationIPAddress, stFrame.qwDestAddress, 4);
      kNumberToAddressArray(stPseudoHeader.vbSourceIPAddress, stFrame.qwDestAddress >> 32, 4);
      stPseudoHeader.wUDPLength = htons(stUDPHeader.wLength);

      // 체크섬 계산
      stUDPHeader.wChecksum = 0x0000;

      // 캡슐화
      kEncapuslationFrame(&stFrame, &stUDPHeader, sizeof(UDP_HEADER), NULL, 0);

      stFrame.bType = FRAME_UDP;

      // IP로 전송
      gs_stUDPManager.pfDownIP(stFrame);
      break;  /* End of case FRAME_IN: */
    }
  }
}

BOOL kUDP_Initialize(void)
{
  // 뮤텍스 초기화
  kInitializeMutex(&(gs_stUDPManager.stLock));

  // Allocate Frame Queue
  gs_stUDPManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stUDPManager.pstFrameBuffer == NULL) {
    kPrintf("kUDP_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // Inint Frame Queue
  kInitializeQueue(&(gs_stUDPManager.stFrameQueue), gs_stUDPManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // 레이어 설정
  gs_stUDPManager.pfDownIP = kIP_DownDirectionPoint;
  gs_stUDPManager.pfUpDHCP = kDHCP_UpDirectionPoint;

  return TRUE;
}

BOOL kUDP_DownDirectionPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_IN;
  if (kUDP_PutFrameToFrameQueue(&stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL kUDP_UpDirectionPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_OUT;
  if (kUDP_PutFrameToFrameQueue(&stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL kUDP_PutFrameToFrameQueue(const FRAME* pstFrame)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stUDPManager.stLock));

  // Put into FrameQueue
  bResult = kPutQueue(&(gs_stUDPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stUDPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kUDP_GetFrameFromFrameQueue(FRAME* pstFrame)
{
  BOOL bResult;
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stUDPManager.stLock));

  // Get from FrameQueue
  bResult = kGetQueue(&(gs_stUDPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stUDPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}