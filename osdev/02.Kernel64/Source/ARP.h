#ifndef __ARP_H__
#define __ARP_H__

#include "Types.h"
#include "List.h"
#include "Queue.h"
#include "Frame.h"
#include "Synchronization.h"

#define ARP_HADRWARETYPE_ETHERNET 0x0001

#define ARP_HARDWAREADDRESSLENGTH_ETHERNET  0x06
#define ARP_PROTOCOLADDRESSLENGTH_IPV4      0x04
#define ARP_PROTOCOLTYPE_IPV4     0x0800
#define ARP_OPERATION_REQUEST     0x0001
#define ARP_OPERATION_REPLY      0x0002

#define ARP_TABLE_INDEX_MAX_COUNT 256

#define ARP_TABLE_PA_BROADCAST  0xFFFFFFFF
#define ARP_TABLE_HA_BROADCAST  0xFFFFFFFFFFFF

#define ARP_TABLE_KEY_SIZE      4
#define ARP_TABLE_STATIC_TYPE   0
#define ARP_TABLE_DYNAMIC_TYPE  1


#pragma pack(push, 1)

typedef struct kARPHeader {
  WORD wHardwareType;
  WORD wProtocolType;
  BYTE bHadrwareAddressLength;
  BYTE bProtocolAddressLength;
  WORD wOperation;
  BYTE vbSenderHardwareAddress[6];
  BYTE vbSenderProtocolAddress[4];
  BYTE vbTargetHardwareAddress[6];
  BYTE vbTargetProtocolAddress[4];
} ARP_HEADER;

typedef struct kARPTableEntry {
  LISTLINK stEntryLink;
  QWORD qwHardwareAddress;
  BYTE bType;
  QWORD qwTime;
} ARP_ENTRY;

typedef struct kARPTable {
  LIST vstEntryList[ARP_TABLE_INDEX_MAX_COUNT];
} ARP_TABLE;

typedef struct kARPManager
{
  // 동기화 객체
  MUTEX stLock;

  ARP_TABLE stARPTable;
  
  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  SideOutFunction pfSideOut;
} ARPMANAGER;
#pragma pack(pop)

// function
void kARP_Task(void);
BOOL kARP_Initialize(void);

void kARP_Send(DWORD dwDestinationProtocolAddress, DWORD dwSourceProtocolAddress);

void kARPTable_Put(ARP_ENTRY* pstEntry);
ARP_ENTRY* kARPTable_Get(DWORD dwKey);
void kARPTable_Print(void);
QWORD kARP_GetHardwareAddress(DWORD dwProtocolAddress);

// TODO : 만료된 엔트리 제거
void kARPTable_Clean(void);

BOOL kARP_SideInPoint(FRAME stFrame);
BOOL kARP_PutFrameToFrameQueue(const FRAME* pstFrame);
BOOL kARP_GetFrameFromFrameQueue(FRAME* pstFrame);

#endif // !__ARP_H__
