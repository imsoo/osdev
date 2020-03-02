#include "DHCP.h"
#include "UDP.h"
#include "ARP.h"
#include "Ethernet.h"
#include "IP.h"
#include "Utility.h"
#include "DNS.h"

static DHCPMANAGER gs_stDHCPManager = { 0, };
static BYTE gs_vbDHCPParamterRequestList[DHCP_PARAMETER_REQUEST_LIST_SIZE] = {
  BOOTP_VENDOR_EXTENSIONS_CODE_SUBNETMASK,
  BOOTP_VENDOR_EXTENSIONS_CODE_ROUTER,
  BOOTP_VENDOR_EXTENSIONS_CODE_DOMAINNAMESERVER,
  BOOTP_VENDOR_EXTENSIONS_CODE_DOMAINNAME
};

static BYTE* gs_vpbDHCPStateString[] = {
  "DHCP_INIT",
  "DHCP_SELECTING",
  "DHCP_REQUESTING",
  "DHCP_BOUND",
  "DHCP_RENEWING",
  "DHCP_REBINDING"
};

void kDHCP_SendMessage(BYTE bMessageType, BYTE bRenewFlag)
{
  FRAME stFrame;
  DHCP_Header stDHCPHeader = { 0, };
  DHCP_Option stDHCPOption = { 0, };
  int iOptionIndex = 0;

  // OP, HTYPE, HLEN, HOPS
  stDHCPHeader.bMessageType = DHCP_MESSAGETYPE_BOOTREQUEST;
  stDHCPHeader.bHardwareType = DHCP_HARDWARETYPE_ETHERNET;
  stDHCPHeader.bHardwareAddressLength = DHCP_HARDWARELEN_ETHERNET;
  stDHCPHeader.bHops = DHCP_HOPS_DEFAULT;

  // XID
  if (bMessageType == DHCP_OPTION_VALUE_DHCPDISCOVER)
    gs_stDHCPManager.dwTransactionID = kRandom() & 0xFFFFFFFF;
  stDHCPHeader.dwTransactionID = htonl(gs_stDHCPManager.dwTransactionID);

  // SECS
  stDHCPHeader.wSecs = 0x0000;

  // Flags
  // 갱신 메시지는 중 RENEWING State에서 전송 메시지는 Unicast
  // 나머지는 BroadCast
  if (bRenewFlag == 1) 
    stDHCPHeader.wFlags = htons(0 << DHCP_FLAGS_BROADCAST_SHIFT);
  else 
    stDHCPHeader.wFlags = htons(1 << DHCP_FLAGS_BROADCAST_SHIFT);

  // CIADDR, YIADDR, SIADDR, GIADDR 모두 0
  // 갱신 메시지의 경우 CIADDR을 현재 IP 주소로 설정함.
  if (bRenewFlag != 0) {
    kMemCpy(stDHCPHeader.vbClientIPAddress, gs_stDHCPManager.vbMyIPAddress, 4);
  }

  // CHADDR 
  kEthernet_GetMACAddress(stDHCPHeader.vbClientHardwareAddress);

  // MAGIC COOKIE
  stDHCPHeader.dwMagicCookie = htonl(DHCP_MAGICCOOKIE);

  // Option
  // DHCP Request
  stDHCPOption.bCode = DHCP_OPTION_CODE_DHCPMESSAGETYPE;
  stDHCPOption.bLength = 1;
  stDHCPOption.bValue = bMessageType;
  iOptionIndex = kDHCP_AddOption(&stDHCPHeader, &stDHCPOption, iOptionIndex);

  // DHCP Parameter Request List
  stDHCPOption.bCode = DHCP_OPTION_CODE_PARAMETERREQUESTLIST;
  stDHCPOption.bLength = DHCP_PARAMETER_REQUEST_LIST_SIZE;
  stDHCPOption.pbValue = gs_vbDHCPParamterRequestList;
  iOptionIndex = kDHCP_AddOption(&stDHCPHeader, &stDHCPOption, iOptionIndex);

  if (bMessageType == DHCP_OPTION_VALUE_DHCPREQUEST ||
    bMessageType == DHCP_OPTION_VALUE_DHCPDECLINE) {
    // Requested IP Address
    stDHCPOption.bCode = DHCP_OPTION_CODE_REQUESTEDIPADDRESS;
    stDHCPOption.bLength = 4;
    stDHCPOption.pbValue = gs_stDHCPManager.vbMyIPAddress;
    iOptionIndex = kDHCP_AddOption(&stDHCPHeader, &stDHCPOption, iOptionIndex);

    // Server Identifier
    stDHCPOption.bCode = DHCP_OPTION_CODE_DHCPSERVERIDENTIFIER;
    stDHCPOption.bLength = 4;
    stDHCPOption.pbValue = gs_stDHCPManager.vbServerIPAddress;
    iOptionIndex = kDHCP_AddOption(&stDHCPHeader, &stDHCPOption, iOptionIndex);
  }

  // Endmark
  *(stDHCPHeader.vbOptions + iOptionIndex) = DHCP_OPTION_CODE_ENDMARK;

  // 캡슐화
  kAllocateFrame(&stFrame);
  kEncapuslationFrame(&stFrame, &stDHCPHeader, sizeof(DHCP_Header), NULL, 0);

  // UDP로 전송
  stFrame.bType = FRAME_DHCP;
  stFrame.dwDestPort = (UDP_PORT_DHCP_CLIENT << 16) | UDP_PORT_DHCP_SERVER;
  stFrame.qwDestAddress = 0x00000000FFFFFFFF;
  stFrame.eDirection = FRAME_DOWN;

  kDHCP_PutFrameToFrameQueue(&stFrame);
}


BYTE kDHCP_AddOption(DHCP_Header* pstHeader, DHCP_Option* pstOption, WORD wIndex)
{
  kMemCpy(pstHeader->vbOptions + wIndex, pstOption, 2);

  if (pstOption->bLength == 1) {
    kMemCpy(pstHeader->vbOptions + wIndex + 2, &(pstOption->bValue), pstOption->bLength);
  }
  else {
    kMemCpy(pstHeader->vbOptions + wIndex + 2, pstOption->pbValue, pstOption->bLength);
  }

  return wIndex + pstOption->bLength + 2;
}

BOOL kDHCP_FindOption(DHCP_Header* pstHeader, DHCP_Option* pstOption, BYTE bCode)
{
  DHCP_Option* pstDHCPOption;
  pstDHCPOption = pstHeader->vbOptions;

  while (pstDHCPOption->bCode != DHCP_OPTION_CODE_ENDMARK) {
    if (pstDHCPOption->bCode == bCode) {
      if (pstDHCPOption->bLength != 1) {
        pstOption->pbValue = (BYTE*)pstDHCPOption + 2;
      }
      return TRUE;
    }
    pstDHCPOption = (BYTE*)pstDHCPOption + (pstDHCPOption->bLength + 2);
  }
  return FALSE;
}

BOOL kDHCP_CheckDHCPMessage(DHCP_Header* pstHeader, BYTE bValue)
{
  DHCP_Option* pstDHCPOption;
  pstDHCPOption = pstHeader->vbOptions;

  if (pstDHCPOption->bCode == DHCP_OPTION_CODE_DHCPMESSAGETYPE) {
    if (pstDHCPOption->bValue == bValue) {
      return TRUE;
    }
  }
  return FALSE;
}

DWORD kDHCP_GetLeaseTime(BYTE* pbTime)
{
  return ((pbTime[0] << 24) + (pbTime[1] << 16) + (pbTime[2] << 8) + pbTime[3]);
}

void kDHCP_ShowState(void)
{
  kPrintf("DHCP State : %s [%d]", gs_vpbDHCPStateString[gs_stDHCPManager.eState], gs_stDHCPManager.eState);
  kPrintf("\n\tIP Address : ");
  kPrintIPAddress(gs_stDHCPManager.vbMyIPAddress);

  kPrintf("\n\tSubnet Mask : ");
  kPrintIPAddress(gs_stDHCPManager.vbSubnetMask);

  kPrintf("\n\tDHCP Server Address : ");
  kPrintIPAddress(gs_stDHCPManager.vbServerIPAddress);

  kPrintf("\n\tGateway Address : ");
  kPrintIPAddress(gs_stDHCPManager.vbGateWayIPAddress);

  kPrintf("\n\tDNS Address : ");
  kPrintIPAddress(gs_stDHCPManager.vbDNSIPAddress);

  kPrintf("\n");
}

void kDHCP_Task(void)
{ 
  FRAME stFrame;
  DHCP_Header* pstDHCPHeader;
  DHCP_Option stDHCPOption;
  int iRetransmitCount = 0;
  int i;
  ARP_ENTRY* pstARPEntry;
  QWORD qwTimer1, qwTimer2, qwHalfTimer, qwDurationOfLease, qwTempTimer;

  // 초기화
  if (kDHCP_Initialize() == FALSE)
    return;

  while (1)
  {
    switch (gs_stDHCPManager.eState)
    {

    case DHCP_INIT:
      // 1 ~ 10 초 대기
      iRetransmitCount = 1;
      kSleep(kRandom() % 9000 + 1000);
      kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPDISCOVER, 0);
      gs_stDHCPManager.qwTime = kGetTickCount();
      gs_stDHCPManager.eState = DHCP_SELECTING;
      break;

    case DHCP_SELECTING:
      // 5초간 DHCP_OFFER를 수신하지 못하는 경우 INIT 상태로 변경
      if (kGetTickCount() - gs_stDHCPManager.qwTime >= 5000) {
        gs_stDHCPManager.eState = DHCP_INIT;
      }
      break;

    case DHCP_REQUESTING:
      // 60초간 DHCP_ACK를 수신하지 못하는 경우 INIT 상태로 변경
      if (kGetTickCount() - gs_stDHCPManager.qwTime >= 60000) {
        gs_stDHCPManager.eState = DHCP_INIT;
      }
      // 60초 대기 중 15초마다 DHCP_Request 메시지 재송신
      else if (kGetTickCount() - gs_stDHCPManager.qwTime >= 15000 * iRetransmitCount) {
        iRetransmitCount++;
        kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPREQUEST, 0);
      }
      break;

    case DHCP_BOUND:
      // T1 타임 확인 : IP 만료 1/2 지점 초과시 
      // 요청 메시지 전송 시간 기록
      // 갱신 요청 메시지 전송 후 RENEWING 상태로 변경
      if (kGetTickCount() >= qwTimer1) {
        iRetransmitCount = 1;
        qwHalfTimer = (qwTimer2 - qwTimer1) / 2;
        gs_stDHCPManager.qwTime = kGetTickCount();
        kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPREQUEST, 1);
        gs_stDHCPManager.eState = DHCP_RENEWING;
      }
      break;

    case DHCP_RENEWING:
      // T2와 T1 사이 중간 지점, T2 지점에 갱신 요청 메시지 전송 
      if (kGetTickCount() >= qwTimer1 + qwHalfTimer * iRetransmitCount) {
        iRetransmitCount++;
        gs_stDHCPManager.qwTime = kGetTickCount();
        kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPREQUEST, 1);
      }

      // T2 타임 확인 : IP 만료 7/8 지점 초과시 
      // REBINDING 상태로 변경
      if (kGetTickCount() >= qwTimer2) {
        iRetransmitCount = 1;
        qwHalfTimer = (gs_stDHCPManager.qwTime - qwTimer2) / 2;
        gs_stDHCPManager.eState = DHCP_REBINDING;
      }
      break;

    case DHCP_REBINDING:
      // T2와 만료까지 중간 지점, 만료 지점에 갱신 요청 메시지 전송 
      if (kGetTickCount() >= qwTimer2 + qwHalfTimer * iRetransmitCount) {
        iRetransmitCount++;
        gs_stDHCPManager.qwTime = kGetTickCount();
        kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPREQUEST, 2);
      }

      // 만료 시간확인 
      // 초기 상태로 변경
      if (kGetTickCount() >= gs_stDHCPManager.qwTime) {
        gs_stDHCPManager.eState = DHCP_INIT;
      }
      break;
    }

    // 큐 확인
    if (kDHCP_GetFrameFromFrameQueue(&stFrame) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_UP:
      // kPrintf("DHCP | Receive DHCP Packet \n");
      kDecapuslationFrame(&stFrame, &pstDHCPHeader, sizeof(DHCP_Header), NULL);

      switch (gs_stDHCPManager.eState)
      {
      case DHCP_INIT:
        // 모든 DHCP 메시지 Pass
        break;
      case DHCP_SELECTING:
        // Transaction ID 확인
        if (ntohl(pstDHCPHeader->dwTransactionID) == gs_stDHCPManager.dwTransactionID) {
          // DHCP_OFFER 메시지 인지 확인
          if (kDHCP_CheckDHCPMessage(pstDHCPHeader, DHCP_OPTION_VALUE_DHCPOFFER) == TRUE) {
            // Server Identifier Option 에서 DHCP 서버 주소 추출
            if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_DHCPSERVERIDENTIFIER) != FALSE) {
              kMemCpy(gs_stDHCPManager.vbServerIPAddress, stDHCPOption.pbValue, 4);

              // YIADDR 에서 내 IP 주소 추출
              kMemCpy(gs_stDHCPManager.vbMyIPAddress, pstDHCPHeader->vbYourIPAddress, 4);

              
              // Subnet Mask Option 에서 서브넷 마스크 추출
              if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_SUBNETMASK) != FALSE) {
                kMemCpy(gs_stDHCPManager.vbSubnetMask, stDHCPOption.pbValue, 4);
              }

              // Router Option 에서 게이트웨이 주소 추출
              if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_ROUTER) != FALSE) {
                kMemCpy(gs_stDHCPManager.vbGateWayIPAddress, stDHCPOption.pbValue, 4);
              }

              // DNS Server Option 에서 DNS 서버 주소 추출
              if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_DOMAINNAMESERVER) != FALSE) {
                kMemCpy(gs_stDHCPManager.vbDNSIPAddress, stDHCPOption.pbValue, 4);
              }

              // DHCP_Request 메시지 전송 후 DHCP_REQUESTING 상태로 변경
              kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPREQUEST, 0);
              gs_stDHCPManager.eState = DHCP_REQUESTING;
            }
          }
        }
        break;
      case DHCP_REQUESTING:
        // Transaction ID 확인
        if (ntohl(pstDHCPHeader->dwTransactionID) == gs_stDHCPManager.dwTransactionID) {
          // DHCP_ACK 메시지 인지 확인
          if (kDHCP_CheckDHCPMessage(pstDHCPHeader, DHCP_OPTION_VALUE_DHCPACK) == TRUE) {

            // IP Addresss Lease Time 추출
            if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_IPADDRESSLEASETIME) != FALSE) {
              // 대여 기간 밀리세컨트 단위로 변환 ( * 1000)
              qwDurationOfLease = (kDHCP_GetLeaseTime(stDHCPOption.pbValue) * 1000);

              // Timer 1, Timer 2 설정
              qwTimer1 = gs_stDHCPManager.qwTime + (qwDurationOfLease * (0.5)) + (kRandom() & 0xFF);
              qwTimer2 = gs_stDHCPManager.qwTime + (qwDurationOfLease * (0.875)) + (kRandom() & 0xFF);

              // DHCP 대여 만료 시간 설정
              gs_stDHCPManager.qwTime += qwDurationOfLease;

              // ARP 패킷 전송전 시간 기록
              qwTempTimer = kGetTickCount();

              // ARP 이용하여 할당 받은 IP 주소 중복 여부 검사
              // 소스 IP 주소를 0으로 설정한 ARP 패킷 전송
              kARP_Send(kAddressArrayToNumber(gs_stDHCPManager.vbMyIPAddress, 4), 0x00000000);

              // 잠깐 대기
              kSleep(kRandom() % 1000 + 500);

              // ARP 테이블을 검색하여 응답 여부 검사
              // 테이블의 엔트리 등록 시간을 확인
              pstARPEntry = kARPTable_Get(0x00000000);

              // 엔트리 등록 시간이  ARP 패킷 전송전 시간 보다 작은 경우
              // (ARP 테이블이 갱신되지 않는 경우, 응답이 없는 경우)
              // 유일한 IP 주소임을 확인
              if (pstARPEntry->qwTime <= qwTempTimer) {
                // 송신자 주소를 내 IP 주소로 설정한 ARP 브로드 캐스트 패킷 전송
                // 이웃들의 ARP 테이블 갱신 용도
                kARP_Send(0xFFFFFFFF, kAddressArrayToNumber(gs_stDHCPManager.vbMyIPAddress, 4));
                // DHCP_BINDING 상태로 변경
                gs_stDHCPManager.eState = DHCP_BOUND;

                // IP 모듈 IP, Gateway 주소 설정
                kIP_SetIPAddress(gs_stDHCPManager.vbMyIPAddress);
                kIP_SetGatewayIPAddress(gs_stDHCPManager.vbGateWayIPAddress);

                // DNS 모듈 DNS 서버 주소 설정
                kDNS_SetDNSAddress(gs_stDHCPManager.vbDNSIPAddress);
              }
              // 중복 IP 주소를 DHCP 서버로 부터 할당받은 경우
              else {
                // DHCP Decline 메시지 전송
                kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPDECLINE, 0);
                gs_stDHCPManager.eState = DHCP_INIT;
              }
            }
          }
        }
        break;
      case DHCP_BOUND:
        break;

      case DHCP_RENEWING:
      case DHCP_REBINDING:
        // Transaction ID 확인
        if (ntohl(pstDHCPHeader->dwTransactionID) == gs_stDHCPManager.dwTransactionID) {
          // DHCP_ACK 메시지 인지 확인
          if (kDHCP_CheckDHCPMessage(pstDHCPHeader, DHCP_OPTION_VALUE_DHCPACK) == TRUE) {

            // IP Addresss Lease Time 추출
            if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_IPADDRESSLEASETIME) != FALSE) {
              
              // 대여 기간 밀리세컨트 단위로 변환 ( * 1000)
              qwDurationOfLease = (kDHCP_GetLeaseTime(stDHCPOption.pbValue) * 1000);

              // Timer 1, Timer 2 설정
              qwTimer1 = gs_stDHCPManager.qwTime + (qwDurationOfLease * (0.5)) + (kRandom() & 0xFF);
              qwTimer2 = gs_stDHCPManager.qwTime + (qwDurationOfLease * (0.875)) + (kRandom() & 0xFF);

              // DHCP 대여 만료 시간 설정
              gs_stDHCPManager.qwTime += qwDurationOfLease;

              // 다시 BOUND 상태로 변경
              gs_stDHCPManager.eState = DHCP_BOUND;
            }
          }
        }
        break;
      default:
        break;
      }

      break;  /* End of case FRAME_UP: */
    case FRAME_DOWN:
      // kPrintf("DHCP | Send DHCP Packet\n");
      gs_stDHCPManager.pfDownUDP(stFrame);
      break;  /* End of case FRAME_DOWN: */
    }
  }
}

BOOL kDHCP_Initialize(void)
{
  // 뮤텍스 초기화
  kInitializeMutex(&(gs_stDHCPManager.stLock));

  // Allocate Frame Queue
  gs_stDHCPManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stDHCPManager.pstFrameBuffer == NULL) {
    kPrintf("kDHCP_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // Inint Frame Queue
  kInitializeQueue(&(gs_stDHCPManager.stFrameQueue), gs_stDHCPManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // 레이어 설정
  gs_stDHCPManager.pfDownUDP = kUDP_DownDirectionPoint;
  gs_stDHCPManager.pfUp = kStub_UpDirectionPoint;

  return TRUE;
}

BOOL kDHCP_UpDirectionPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_UP;
  if (kDHCP_PutFrameToFrameQueue(&stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL kDHCP_PutFrameToFrameQueue(const FRAME* pstFrame)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDHCPManager.stLock));

  // Put into FrameQueue
  bResult = kPutQueue(&(gs_stDHCPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stDHCPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kDHCP_GetFrameFromFrameQueue(FRAME* pstFrame)
{
  BOOL bResult;
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDHCPManager.stLock));

  // Get from FrameQueue
  bResult = kGetQueue(&(gs_stDHCPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stDHCPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}