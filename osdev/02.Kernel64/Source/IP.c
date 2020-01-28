#include "IP.h"
#include "Utility.h"
#include "Ethernet.h"

static IPMANAGER gs_stIPManager = { 0, };

void kIP_Task(void)
{
  int i;
  FRAME stFrame;
  IP_HEADER stIPHeader, *pstIPHeader;
  // 초기화
  if (kIP_Initialize() == FALSE)
    return;


  stIPHeader.bVersionAndIHL = (IP_VERSION_IPV4 << IP_VERSION_SHIFT) | 0x05;
  stIPHeader.bDSCPAndECN = 0x00;
  stIPHeader.wIdentification = htons(0x2000);
  stIPHeader.wFlagsAndFragmentOffset = htons(0x0000);
  stIPHeader.bTimeToLive = 128;
  stIPHeader.wHeaderChecksum = 0x00;
  kMemCpy(stIPHeader.vbSourceIPAddress, gs_stIPManager.vbIPAddress, 4);
  kMemCpy(stIPHeader.vbDestinationIPAddress, gs_stIPManager.vbGatewayAddress, 4);


  while (1)
  {
    if (kGetQueue(&(gs_stIPManager.stFrameQueue), &stFrame) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_OUT:
      pstIPHeader = stFrame.pbCur;
      kPrintf("IP | Receive IP Datagram %x\n", pstIPHeader->bProtocol);

      switch (pstIPHeader->bProtocol)
      {
      case IP_PROTOCOL_ICMP:
        gs_stIPManager.pfSideOut(stFrame);
        break;
      case IP_PROTOCOL_TCP:
      case IP_PROTOCOL_UDP:
        gs_stIPManager.pfUp(stFrame);
        break;
      default:
        break;
      }
      break;
    case FRAME_IN:
      kPrintf("IP | Send IP Datagram \n");

      switch (stFrame.bType)
      {
      case FRAME_ICMP:
        // IP 헤더 추가
        stIPHeader.wIdentification = htons(gs_stIPManager.wIdentification++);
        stIPHeader.bProtocol = IP_PROTOCOL_ICMP;
        stFrame.wLen += sizeof(IP_HEADER);
        stIPHeader.wTotalLength = htons(stFrame.wLen);
        stFrame.pbCur = stFrame.pbBuf + FRAME_MAX_SIZE - stFrame.wLen;
        kNumberToAddressArray(stIPHeader.vbDestinationIPAddress, stFrame.qwDestAddress, 4);
        //stIPHeader.wHeaderChecksum = kIP_CalcChecksum(&stIPHeader);
        kMemCpy(stFrame.pbCur, &stIPHeader, sizeof(IP_HEADER));

        stFrame.bType = FRAME_IP;
        stFrame.qwDestAddress = kAddressArrayToNumber(gs_stIPManager.vbGatewayAddress, 4);
        gs_stIPManager.pfDown(stFrame);
        break;

      default:
        break;
      }

      break;
    }
  }
}

BOOL kIP_UpDirectionPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_OUT;
  if (kPutQueue(&(gs_stIPManager.stFrameQueue), &stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL kIP_SideInPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_IN;
  if (kPutQueue(&(gs_stIPManager.stFrameQueue), &stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

static BOOL kIP_Initialize(void)
{
  // Allocate Frame Queue
  gs_stIPManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stIPManager.pstFrameBuffer == NULL) {
    kPrintf("kIP_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // Inint Frame Queue
  kInitializeQueue(&(gs_stIPManager.stFrameQueue), gs_stIPManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // 레이어 설정
  gs_stIPManager.pfDown = kEthernet_DownDirectionPoint;
  gs_stIPManager.pfUp = kStub_UpDirectionPoint;

  // 임시
  // QEMU Virtual Network Device : 10.0.2.15
  gs_stIPManager.vbIPAddress[0] = 10;
  gs_stIPManager.vbIPAddress[1] = 0;
  gs_stIPManager.vbIPAddress[2] = 2;
  gs_stIPManager.vbIPAddress[3] = 15;

  // QEMU GateWay : 10.0.2.3
  gs_stIPManager.vbGatewayAddress[0] = 10;
  gs_stIPManager.vbGatewayAddress[1] = 0;
  gs_stIPManager.vbGatewayAddress[2] = 2;
  gs_stIPManager.vbGatewayAddress[3] = 2;

  return TRUE;
}

WORD kIP_CalcChecksum(IP_HEADER* pstHeader)
{
  int i;
  DWORD dwSum = 0;
  WORD* pwP = pstHeader;

  for (int i = 0; i < sizeof(IP_HEADER) / 2; i++)
  {
    dwSum += ntohs(pwP[i]);
  }

  if (dwSum > 0xFFFF) {
    dwSum = ((dwSum >> 16) + (dwSum & 0xFFFF));
  }

  return ~(dwSum & 0xFFFF) & 0xFFFF;
}