#include "ARP.h"
#include "Ethernet.h"
#include "IP.h"
#include "DynamicMemory.h"
#include "Utility.h"
#include "Console.h"

static ARPMANAGER gs_stARPManager = { 0, };

void kARP_Task(void)
{
  FRAME stFrame;
  ARP_HEADER stARPHeader, *pstARPHeader;
  ARP_ENTRY *pstEntry;
  BYTE vbIPAddress[4];
  BYTE bMergeFlag;

  // �ʱ�ȭ
  if (kARP_Initialize() == FALSE)
    return;

  while (1)
  {
    if (kARP_GetFrameFromFrameQueue(&stFrame) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_OUT:
      kPrintf("ARP | Send ARP Packet\n");

      gs_stARPManager.pfSideOut(stFrame);
      break;
    case FRAME_IN:
      kPrintf("ARP | Receive ARP Packet\n");

      kDecapuslationFrame(&stFrame, &pstARPHeader, sizeof(ARPMANAGER), NULL);

      // �̴��� ���� �˻�
      if (ntohs(pstARPHeader->wHardwareType) != ARP_HADRWARETYPE_ETHERNET)
        break;

      // IPv4 ���� �˻�
      if (ntohs(pstARPHeader->wProtocolType) != ARP_PROTOCOLTYPE_IPV4)
        break;

      bMergeFlag = FALSE;

      // ���̺� ����
      pstEntry = kARPTable_Get(kAddressArrayToNumber(pstARPHeader->vbSenderProtocolAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4));
      // ��Ʈ���� �̹� �����ϴ� ��� �ϵ���� �ּ� ����
      if (pstEntry != NULL) {
        pstEntry->qwHardwareAddress = kAddressArrayToNumber(pstARPHeader->vbSenderHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
        pstEntry->qwTime = kGetTickCount();
        bMergeFlag = TRUE;
      }

      // ������ �������� �ּҸ� ��
      kIP_GetIPAddress(vbIPAddress);
      if (kMemCmp(pstARPHeader->vbTargetProtocolAddress, vbIPAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4) == 0) {

        // �� ���̺� ��Ʈ�� ����
        if (bMergeFlag == FALSE) {
          kPrintf("ARP | New Entry\n");
          pstEntry = (ARP_ENTRY*)kAllocateMemory(sizeof(ARP_ENTRY));
          if (pstEntry == NULL) {
            return FALSE;
          }
          pstEntry->stEntryLink.qwID = kAddressArrayToNumber(pstARPHeader->vbSenderProtocolAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4);
          pstEntry->qwHardwareAddress = kAddressArrayToNumber(pstARPHeader->vbSenderHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
          pstEntry->bType = ARP_TABLE_DYNAMIC_TYPE;
          pstEntry->qwTime = kGetTickCount();

          kARPTable_Put(pstEntry);
        }

        // ARP ��û�� ��� ���� ��Ŷ ����
        if (ntohs(pstARPHeader->wOperation) == ARP_OPERATION_REQUEST) {
          // ������ �ּҸ� �ٿ��� �ּҷ� ����
          kMemCpy(pstARPHeader->vbTargetHardwareAddress, pstARPHeader->vbSenderHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
          kMemCpy(pstARPHeader->vbTargetProtocolAddress, pstARPHeader->vbSenderProtocolAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4);

          // Operation ����
          pstARPHeader->wOperation = ARP_OPERATION_REPLY;

          // �ٿ��� �ּ� ����
          kEthernet_GetMACAddress(pstARPHeader->vbSenderHardwareAddress);
          kIP_GetIPAddress(pstARPHeader->vbSenderProtocolAddress);

          stFrame.qwDestAddress = kAddressArrayToNumber(pstARPHeader->vbTargetHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
          stFrame.eDirection = FRAME_OUT;
          kARP_PutFrameToFrameQueue(&stFrame);
        }
      }

      break;
    }
  }
}

BOOL kARP_Initialize(void)
{
  int i;
  ARP_ENTRY* pstEntry;

  // ���ؽ� �ʱ�ȭ
  kInitializeMutex(&(gs_stARPManager.stLock));

  // Allocate Frame Queue
  gs_stARPManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stARPManager.pstFrameBuffer == NULL) {
    kPrintf("kARP_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // Inint Frame Queue
  kInitializeQueue(&(gs_stARPManager.stFrameQueue), gs_stARPManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // ���̾� ����
  gs_stARPManager.pfSideOut = kEthernet_DownDirectionPoint;

  // ���̺� �ʱ�ȭ
  for (i = 0; i < ARP_TABLE_INDEX_MAX_COUNT; i++) {
    kInitializeList(&(gs_stARPManager.stARPTable.vstEntryList[i]));
  }

  // TODO : �����Ҵ��� �ƴ� POOL ���·� ����
  pstEntry = (ARP_ENTRY*)kAllocateMemory(sizeof(ARP_ENTRY));
  if (pstEntry == NULL) {
    return FALSE;
  }
  // ���̺� �ʱ� ��Ʈ�� ���� (BroadCast)
  pstEntry->stEntryLink.qwID = ARP_TABLE_PA_BROADCAST;
  pstEntry->qwHardwareAddress = ARP_TABLE_HA_BROADCAST;
  pstEntry->bType = ARP_TABLE_STATIC_TYPE;
  pstEntry->qwTime = kGetTickCount();
  kARPTable_Put(pstEntry);

  // TODO : �����Ҵ��� �ƴ� POOL ���·� ����
  pstEntry = (ARP_ENTRY*)kAllocateMemory(sizeof(ARP_ENTRY));
  if (pstEntry == NULL) {
    return FALSE;
  }
  // ���̺� �ʱ� ��Ʈ�� ���� (DHCP Ȯ�ο�)
  pstEntry->stEntryLink.qwID = 0x0;
  pstEntry->qwHardwareAddress = 0x0;
  pstEntry->bType = ARP_TABLE_STATIC_TYPE;
  pstEntry->qwTime = kGetTickCount();
  kARPTable_Put(pstEntry);

  return TRUE;
}

void kARPTable_Put(ARP_ENTRY* pstEntry)
{
  BYTE dwIndex = HASH(pstEntry->stEntryLink.qwID, ARP_TABLE_KEY_SIZE);

  kAddListToHeader(&(gs_stARPManager.stARPTable.vstEntryList[dwIndex]), &(pstEntry->stEntryLink));
}

ARP_ENTRY* kARPTable_Get(DWORD dwKey)
{
  ARP_ENTRY* pstEntry;
  BYTE dwIndex = HASH(dwKey, ARP_TABLE_KEY_SIZE);

  pstEntry = kFindList(&(gs_stARPManager.stARPTable.vstEntryList[dwIndex]), dwKey);
  if (pstEntry != NULL) {
    return pstEntry;
  }
  return NULL;
}

QWORD kARP_GetHardwareAddress(DWORD dwProtocolAddress)
{
  ARP_ENTRY* pstEntry;
  BYTE vbIPAddress[4];
  static DWORD dwPreviousProtocolAddress;
  static QWORD qwPreviousTime;

  // ARP ���̺� �˻�
  pstEntry = kARPTable_Get(dwProtocolAddress);
  if (pstEntry != NULL) {
    return pstEntry->qwHardwareAddress;
  }

  // ���̺� �������� �ʾ� ������ 
  // ARP Request Broadcast ��Ŷ�� ������ ��� 500ms ���� ������ ���� ����.
  if ((dwPreviousProtocolAddress != dwProtocolAddress) || (kGetTickCount() - qwPreviousTime >= 500)) {
    qwPreviousTime = kGetTickCount();
    dwPreviousProtocolAddress = dwProtocolAddress;
    // �������� �ʴ� ��� ARP Request ����
    kIP_GetIPAddress(vbIPAddress);
    kARP_Send(dwProtocolAddress, kAddressArrayToNumber(vbIPAddress, 4));
  }
  return 0;
}

BOOL kARP_SideInPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_IN;
  if (kARP_PutFrameToFrameQueue(&stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

void kARPTable_Print(void)
{
  int i, j;
  LIST stList;
  ARP_ENTRY* pstEntry;
  BYTE vbProtocolAddress[ARP_PROTOCOLADDRESSLENGTH_IPV4];
  BYTE vbHardwareAddress[ARP_HARDWAREADDRESSLENGTH_ETHERNET];
  static BYTE* s_vpbTypeString[2] = { "Static", "Dynamic" };

  kPrintf("Protocol Address | Hardware Address | Type\n");
  for (i = 0; i < ARP_TABLE_INDEX_MAX_COUNT; i++) {
    stList = gs_stARPManager.stARPTable.vstEntryList[i];

    if (kGetListCount(&stList) > 0) {
      pstEntry = kGetHeaderFromList(&stList);

      do
      {
        kNumberToAddressArray(vbProtocolAddress, pstEntry->stEntryLink.qwID, ARP_PROTOCOLADDRESSLENGTH_IPV4);
        kNumberToAddressArray(vbHardwareAddress, pstEntry->qwHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);

        kPrintIPAddress(vbProtocolAddress);
        kPrintf(" | ");

        kPrintMACAddress(vbHardwareAddress);
        kPrintf(" | ");

        kPrintf("%s\n", s_vpbTypeString[pstEntry->bType]);
        pstEntry = kGetNextFromList(&stList, pstEntry);
      } while (pstEntry != NULL);
    }
  }

}

void kARP_Send(DWORD dwDestinationProtocolAddress, DWORD dwSourceProtocolAddress)
{
  int i;
  ARP_HEADER stARPPacket;
  FRAME stFrame;

  stARPPacket.wHardwareType = htons(ARP_HADRWARETYPE_ETHERNET);
  stARPPacket.wProtocolType = htons(ARP_PROTOCOLTYPE_IPV4);
  stARPPacket.bHadrwareAddressLength = ARP_HARDWAREADDRESSLENGTH_ETHERNET;
  stARPPacket.bProtocolAddressLength = ARP_PROTOCOLADDRESSLENGTH_IPV4;
  stARPPacket.wOperation = htons(ARP_OPERATION_REQUEST);

  // �ϵ���� �ּ� ����
  kEthernet_GetMACAddress(stARPPacket.vbSenderHardwareAddress);
  kNumberToAddressArray(stARPPacket.vbTargetHardwareAddress, 0xFFFFFFFFFFFFFFFF, 6);

  // �������� �ּ� ����
  kNumberToAddressArray(stARPPacket.vbSenderProtocolAddress, dwSourceProtocolAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4);
  kNumberToAddressArray(stARPPacket.vbTargetProtocolAddress, dwDestinationProtocolAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4);

  kAllocateFrame(&stFrame);
  stFrame.bType = FRAME_ARP;
  stFrame.qwDestAddress = kAddressArrayToNumber(stARPPacket.vbTargetHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
  stFrame.eDirection = FRAME_OUT;

  kEncapuslationFrame(&stFrame, &stARPPacket, sizeof(ARP_HEADER), NULL, 0);

  kARP_PutFrameToFrameQueue(&stFrame);
}

BOOL kARP_PutFrameToFrameQueue(const FRAME* pstFrame)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stARPManager.stLock));

  // Put into FrameQueue
  bResult = kPutQueue(&(gs_stARPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stARPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kARP_GetFrameFromFrameQueue(FRAME* pstFrame)
{
  BOOL bResult;
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stARPManager.stLock));

  // Get from FrameQueue
  bResult = kGetQueue(&(gs_stARPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stARPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}