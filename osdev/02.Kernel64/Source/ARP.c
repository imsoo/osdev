#include "ARP.h"
#include "DynamicMemory.h"
#include "Utility.h"
#include "Ethernet.h"
#include "Console.h"

static ARPMANAGER gs_stARPManager = { 0, };

void kARP_Task(void)
{
  FRAME stFrame;
  ARP_HEADER stARPHeader, *pstARPHeader;
  ARP_ENTRY *pstEntry;

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

      // IPv4 ���� �˻�
      if (ntohs(pstARPHeader->wProtocolType) != ARP_PROTOCOLTYPE_IPV4)
        break;

      // ���̺� ����
      pstEntry = kARPTable_Get(kAddressArrayToNumber(pstARPHeader->vbSenderProtocolAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4));

      // ��Ʈ���� �̹� �����ϴ� ��� �ϵ���� �ּ� ����
      if (pstEntry != NULL) {
        pstEntry->qwHardwareAddress = kAddressArrayToNumber(pstARPHeader->vbSenderHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
      }
      // ���ο� ������ ���
      else {
        // ������ �������� �ּҸ� ��
        if (kMemCmp(pstARPHeader->vbTargetProtocolAddress, gs_stARPManager.vbIPAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4) == 0) {
          kPrintf("ARP | New Entry\n");
          // �� ���̺� ��Ʈ�� ����
          pstEntry = (ARP_ENTRY*)kAllocateMemory(sizeof(ARP_ENTRY));
          if (pstEntry == NULL) {
            return FALSE;
          }
          pstEntry->stEntryLink.qwID = kAddressArrayToNumber(pstARPHeader->vbSenderProtocolAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4);
          pstEntry->qwHardwareAddress = kAddressArrayToNumber(pstARPHeader->vbSenderHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
          pstEntry->bType = ARP_TABLE_DYNAMIC_TYPE;
          kARPTable_Put(pstEntry);

          // ARP ��û�� ��� ���� ��Ŷ ����
          if (ntohs(pstARPHeader->wOperation) == ARP_OPERATION_REQUEST) {
            // ������ �ּҸ� �ٿ��� �ּҷ� ����
            kMemCpy(pstARPHeader->vbTargetHardwareAddress, pstARPHeader->vbSenderHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
            kMemCpy(pstARPHeader->vbTargetProtocolAddress, pstARPHeader->vbSenderProtocolAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4);

            // Operation ����
            pstARPHeader->wOperation = ARP_OPERATION_REPLY;

            // �ٿ��� �ּ� ����
            kMemCpy(pstARPHeader->vbSenderHardwareAddress, gs_stARPManager.vbMACAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
            kMemCpy(pstARPHeader->vbSenderProtocolAddress, gs_stARPManager.vbIPAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4);

            stFrame.qwDestAddress = kAddressArrayToNumber(pstARPHeader->vbTargetHardwareAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
            stFrame.eDirection = FRAME_OUT;
            kARP_PutFrameToFrameQueue(&stFrame);
          }
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

  // �ӽ�
  gs_stARPManager.vbMACAddress[0] = 0x52;
  gs_stARPManager.vbMACAddress[1] = 0x54;
  gs_stARPManager.vbMACAddress[2] = 0x00;
  gs_stARPManager.vbMACAddress[3] = 0x12;
  gs_stARPManager.vbMACAddress[4] = 0x34;
  gs_stARPManager.vbMACAddress[5] = 0x56;
  
  
  // QEMU Virtual Network Device : 10.0.2.15
  gs_stARPManager.vbIPAddress[0] = 10;
  gs_stARPManager.vbIPAddress[1] = 0;
  gs_stARPManager.vbIPAddress[2] = 2;
  gs_stARPManager.vbIPAddress[3] = 15;

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

  // ���̺� �ʱ� ��Ʈ�� ����
  pstEntry->stEntryLink.qwID = ARP_TABLE_PA_BROADCAST;
  pstEntry->qwHardwareAddress = ARP_TABLE_HA_BROADCAST;
  pstEntry->bType = ARP_TABLE_STATIC_TYPE;
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

  // ARP ���̺� �˻�
  pstEntry = kARPTable_Get(dwProtocolAddress);
  if (pstEntry != NULL) {
    return pstEntry->qwHardwareAddress;
  }

  // �������� �ʴ� ��� ARP Request ����
  kARP_Send(dwProtocolAddress);
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

        for (j = 0; j < ARP_PROTOCOLADDRESSLENGTH_IPV4; j++) {
          if (j != 0)
            kPrintf(".");
          kPrintf("%d", vbProtocolAddress[j]);
        }
        kPrintf(" | ");

        for (j = 0; j < ARP_HARDWAREADDRESSLENGTH_ETHERNET; j++) {
          if (j != 0)
            kPrintf("-");
          kPrintf("%x", vbHardwareAddress[j]);
        }
        kPrintf(" | ");

        kPrintf("%s\n", s_vpbTypeString[pstEntry->bType]);
        pstEntry = kGetNextFromList(&stList, pstEntry);
      } while (pstEntry != NULL);
    }
  }

}

void kARP_Send(DWORD dwDestinationProtocolAddress)
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
  kMemCpy(stARPPacket.vbSenderHardwareAddress, gs_stARPManager.vbMACAddress, ARP_HARDWAREADDRESSLENGTH_ETHERNET);
  for (i = 0; i < 6; i++) {
    stARPPacket.vbTargetHardwareAddress[i] = 0xFF;
  }

  // �������� �ּ� ����
  // QEMU Virtual Network Device : 10.0.2.15
  kMemCpy(stARPPacket.vbSenderProtocolAddress, gs_stARPManager.vbIPAddress, ARP_PROTOCOLADDRESSLENGTH_IPV4);
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