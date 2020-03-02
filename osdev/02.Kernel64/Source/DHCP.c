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
  // ���� �޽����� �� RENEWING State���� ���� �޽����� Unicast
  // �������� BroadCast
  if (bRenewFlag == 1) 
    stDHCPHeader.wFlags = htons(0 << DHCP_FLAGS_BROADCAST_SHIFT);
  else 
    stDHCPHeader.wFlags = htons(1 << DHCP_FLAGS_BROADCAST_SHIFT);

  // CIADDR, YIADDR, SIADDR, GIADDR ��� 0
  // ���� �޽����� ��� CIADDR�� ���� IP �ּҷ� ������.
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

  // ĸ��ȭ
  kAllocateFrame(&stFrame);
  kEncapuslationFrame(&stFrame, &stDHCPHeader, sizeof(DHCP_Header), NULL, 0);

  // UDP�� ����
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

  // �ʱ�ȭ
  if (kDHCP_Initialize() == FALSE)
    return;

  while (1)
  {
    switch (gs_stDHCPManager.eState)
    {

    case DHCP_INIT:
      // 1 ~ 10 �� ���
      iRetransmitCount = 1;
      kSleep(kRandom() % 9000 + 1000);
      kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPDISCOVER, 0);
      gs_stDHCPManager.qwTime = kGetTickCount();
      gs_stDHCPManager.eState = DHCP_SELECTING;
      break;

    case DHCP_SELECTING:
      // 5�ʰ� DHCP_OFFER�� �������� ���ϴ� ��� INIT ���·� ����
      if (kGetTickCount() - gs_stDHCPManager.qwTime >= 5000) {
        gs_stDHCPManager.eState = DHCP_INIT;
      }
      break;

    case DHCP_REQUESTING:
      // 60�ʰ� DHCP_ACK�� �������� ���ϴ� ��� INIT ���·� ����
      if (kGetTickCount() - gs_stDHCPManager.qwTime >= 60000) {
        gs_stDHCPManager.eState = DHCP_INIT;
      }
      // 60�� ��� �� 15�ʸ��� DHCP_Request �޽��� ��۽�
      else if (kGetTickCount() - gs_stDHCPManager.qwTime >= 15000 * iRetransmitCount) {
        iRetransmitCount++;
        kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPREQUEST, 0);
      }
      break;

    case DHCP_BOUND:
      // T1 Ÿ�� Ȯ�� : IP ���� 1/2 ���� �ʰ��� 
      // ��û �޽��� ���� �ð� ���
      // ���� ��û �޽��� ���� �� RENEWING ���·� ����
      if (kGetTickCount() >= qwTimer1) {
        iRetransmitCount = 1;
        qwHalfTimer = (qwTimer2 - qwTimer1) / 2;
        gs_stDHCPManager.qwTime = kGetTickCount();
        kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPREQUEST, 1);
        gs_stDHCPManager.eState = DHCP_RENEWING;
      }
      break;

    case DHCP_RENEWING:
      // T2�� T1 ���� �߰� ����, T2 ������ ���� ��û �޽��� ���� 
      if (kGetTickCount() >= qwTimer1 + qwHalfTimer * iRetransmitCount) {
        iRetransmitCount++;
        gs_stDHCPManager.qwTime = kGetTickCount();
        kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPREQUEST, 1);
      }

      // T2 Ÿ�� Ȯ�� : IP ���� 7/8 ���� �ʰ��� 
      // REBINDING ���·� ����
      if (kGetTickCount() >= qwTimer2) {
        iRetransmitCount = 1;
        qwHalfTimer = (gs_stDHCPManager.qwTime - qwTimer2) / 2;
        gs_stDHCPManager.eState = DHCP_REBINDING;
      }
      break;

    case DHCP_REBINDING:
      // T2�� ������� �߰� ����, ���� ������ ���� ��û �޽��� ���� 
      if (kGetTickCount() >= qwTimer2 + qwHalfTimer * iRetransmitCount) {
        iRetransmitCount++;
        gs_stDHCPManager.qwTime = kGetTickCount();
        kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPREQUEST, 2);
      }

      // ���� �ð�Ȯ�� 
      // �ʱ� ���·� ����
      if (kGetTickCount() >= gs_stDHCPManager.qwTime) {
        gs_stDHCPManager.eState = DHCP_INIT;
      }
      break;
    }

    // ť Ȯ��
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
        // ��� DHCP �޽��� Pass
        break;
      case DHCP_SELECTING:
        // Transaction ID Ȯ��
        if (ntohl(pstDHCPHeader->dwTransactionID) == gs_stDHCPManager.dwTransactionID) {
          // DHCP_OFFER �޽��� ���� Ȯ��
          if (kDHCP_CheckDHCPMessage(pstDHCPHeader, DHCP_OPTION_VALUE_DHCPOFFER) == TRUE) {
            // Server Identifier Option ���� DHCP ���� �ּ� ����
            if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_DHCPSERVERIDENTIFIER) != FALSE) {
              kMemCpy(gs_stDHCPManager.vbServerIPAddress, stDHCPOption.pbValue, 4);

              // YIADDR ���� �� IP �ּ� ����
              kMemCpy(gs_stDHCPManager.vbMyIPAddress, pstDHCPHeader->vbYourIPAddress, 4);

              
              // Subnet Mask Option ���� ����� ����ũ ����
              if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_SUBNETMASK) != FALSE) {
                kMemCpy(gs_stDHCPManager.vbSubnetMask, stDHCPOption.pbValue, 4);
              }

              // Router Option ���� ����Ʈ���� �ּ� ����
              if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_ROUTER) != FALSE) {
                kMemCpy(gs_stDHCPManager.vbGateWayIPAddress, stDHCPOption.pbValue, 4);
              }

              // DNS Server Option ���� DNS ���� �ּ� ����
              if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_DOMAINNAMESERVER) != FALSE) {
                kMemCpy(gs_stDHCPManager.vbDNSIPAddress, stDHCPOption.pbValue, 4);
              }

              // DHCP_Request �޽��� ���� �� DHCP_REQUESTING ���·� ����
              kDHCP_SendMessage(DHCP_OPTION_VALUE_DHCPREQUEST, 0);
              gs_stDHCPManager.eState = DHCP_REQUESTING;
            }
          }
        }
        break;
      case DHCP_REQUESTING:
        // Transaction ID Ȯ��
        if (ntohl(pstDHCPHeader->dwTransactionID) == gs_stDHCPManager.dwTransactionID) {
          // DHCP_ACK �޽��� ���� Ȯ��
          if (kDHCP_CheckDHCPMessage(pstDHCPHeader, DHCP_OPTION_VALUE_DHCPACK) == TRUE) {

            // IP Addresss Lease Time ����
            if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_IPADDRESSLEASETIME) != FALSE) {
              // �뿩 �Ⱓ �и�����Ʈ ������ ��ȯ ( * 1000)
              qwDurationOfLease = (kDHCP_GetLeaseTime(stDHCPOption.pbValue) * 1000);

              // Timer 1, Timer 2 ����
              qwTimer1 = gs_stDHCPManager.qwTime + (qwDurationOfLease * (0.5)) + (kRandom() & 0xFF);
              qwTimer2 = gs_stDHCPManager.qwTime + (qwDurationOfLease * (0.875)) + (kRandom() & 0xFF);

              // DHCP �뿩 ���� �ð� ����
              gs_stDHCPManager.qwTime += qwDurationOfLease;

              // ARP ��Ŷ ������ �ð� ���
              qwTempTimer = kGetTickCount();

              // ARP �̿��Ͽ� �Ҵ� ���� IP �ּ� �ߺ� ���� �˻�
              // �ҽ� IP �ּҸ� 0���� ������ ARP ��Ŷ ����
              kARP_Send(kAddressArrayToNumber(gs_stDHCPManager.vbMyIPAddress, 4), 0x00000000);

              // ��� ���
              kSleep(kRandom() % 1000 + 500);

              // ARP ���̺��� �˻��Ͽ� ���� ���� �˻�
              // ���̺��� ��Ʈ�� ��� �ð��� Ȯ��
              pstARPEntry = kARPTable_Get(0x00000000);

              // ��Ʈ�� ��� �ð���  ARP ��Ŷ ������ �ð� ���� ���� ���
              // (ARP ���̺��� ���ŵ��� �ʴ� ���, ������ ���� ���)
              // ������ IP �ּ����� Ȯ��
              if (pstARPEntry->qwTime <= qwTempTimer) {
                // �۽��� �ּҸ� �� IP �ּҷ� ������ ARP ��ε� ĳ��Ʈ ��Ŷ ����
                // �̿����� ARP ���̺� ���� �뵵
                kARP_Send(0xFFFFFFFF, kAddressArrayToNumber(gs_stDHCPManager.vbMyIPAddress, 4));
                // DHCP_BINDING ���·� ����
                gs_stDHCPManager.eState = DHCP_BOUND;

                // IP ��� IP, Gateway �ּ� ����
                kIP_SetIPAddress(gs_stDHCPManager.vbMyIPAddress);
                kIP_SetGatewayIPAddress(gs_stDHCPManager.vbGateWayIPAddress);

                // DNS ��� DNS ���� �ּ� ����
                kDNS_SetDNSAddress(gs_stDHCPManager.vbDNSIPAddress);
              }
              // �ߺ� IP �ּҸ� DHCP ������ ���� �Ҵ���� ���
              else {
                // DHCP Decline �޽��� ����
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
        // Transaction ID Ȯ��
        if (ntohl(pstDHCPHeader->dwTransactionID) == gs_stDHCPManager.dwTransactionID) {
          // DHCP_ACK �޽��� ���� Ȯ��
          if (kDHCP_CheckDHCPMessage(pstDHCPHeader, DHCP_OPTION_VALUE_DHCPACK) == TRUE) {

            // IP Addresss Lease Time ����
            if (kDHCP_FindOption(pstDHCPHeader, &stDHCPOption, DHCP_OPTION_CODE_IPADDRESSLEASETIME) != FALSE) {
              
              // �뿩 �Ⱓ �и�����Ʈ ������ ��ȯ ( * 1000)
              qwDurationOfLease = (kDHCP_GetLeaseTime(stDHCPOption.pbValue) * 1000);

              // Timer 1, Timer 2 ����
              qwTimer1 = gs_stDHCPManager.qwTime + (qwDurationOfLease * (0.5)) + (kRandom() & 0xFF);
              qwTimer2 = gs_stDHCPManager.qwTime + (qwDurationOfLease * (0.875)) + (kRandom() & 0xFF);

              // DHCP �뿩 ���� �ð� ����
              gs_stDHCPManager.qwTime += qwDurationOfLease;

              // �ٽ� BOUND ���·� ����
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
  // ���ؽ� �ʱ�ȭ
  kInitializeMutex(&(gs_stDHCPManager.stLock));

  // Allocate Frame Queue
  gs_stDHCPManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stDHCPManager.pstFrameBuffer == NULL) {
    kPrintf("kDHCP_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // Inint Frame Queue
  kInitializeQueue(&(gs_stDHCPManager.stFrameQueue), gs_stDHCPManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // ���̾� ����
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