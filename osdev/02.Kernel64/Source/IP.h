#ifndef __IP_H__
#define __IP_H__

#include "Types.h"
#include "Queue.h"
#include "List.h"
#include "Frame.h"
#include "Synchronization.h"

#define IP_MAXIMUMTRANSMITUNIT    1500

#define IP_INETERNETHEADERLENGTH_DEFAULT  5

#define IP_FLAGS_OFFSET_LOWER_SHIFT 8
#define IP_FLAGS_MF_SHIFT 13
#define IP_FLAGS_DF_SHIFT 14

#define IP_VERSION_IPV4   0x04
#define IP_VERSION_SHIFT  4

#define IP_PROTOCOL_ICMP  0x01
#define IP_PROTOCOL_TCP   0x06
#define IP_PROTOCOL_UDP   0x11

#define IP_TIMER_DEFAULT_SECOND 15

#pragma pack(push, 1)

typedef struct kIPHeader {
  BYTE bVersionAndIHL;
  BYTE bDSCPAndECN;
  WORD wTotalLength;
  WORD wIdentification;
  WORD wFlagsAndFragmentOffset;
  BYTE bTimeToLive;
  BYTE bProtocol;
  WORD wHeaderChecksum;
  BYTE vbSourceIPAddress[4];
  BYTE vbDestinationIPAddress[4];
} IP_HEADER;

typedef struct kIPReassemblyBuffer
{
  LISTLINK stEntryLink;
  
  QWORD qwBufID_1;
  QWORD qwBUfID_2;

  IP_HEADER stIPHeader;
  BYTE *pbDataBuffer;
  BYTE *pbFragmentBlockBitmap;
  DWORD dwTotalLength;
  QWORD qwTimer;
} IPREASSEMBLYBUFFER;

typedef struct kIPManager
{
  // µø±‚»≠ ∞¥√º
  MUTEX stLock;

  LIST stReassemblyBufferList;

  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  UpFunction pfUpUDP;
  DownFunction pfDown;
  SideOutFunction pfSideOutICMP;

  BYTE vbIPAddress[4];
  BYTE vbGatewayAddress[4];
  WORD wIdentification;
} IPMANAGER;

#pragma pack(pop)

// function
void kIP_Task(void);
BOOL kIP_Initialize(void);

BOOL kIP_UpDirectionPoint(FRAME stFrame);
BOOL kIP_DownDirectionPoint(FRAME stFrame);
BOOL kIP_SideInPoint(FRAME stFrame);

BYTE kIP_Fragmentation(FRAME* stOriginalFrame);
BYTE kIP_Reassembly(FRAME* stFrame);
void kIP_CheckReassemblyBufferList(void);
IPREASSEMBLYBUFFER* kIP_GetReassemblyBuffer(QWORD qwID_1, QWORD qwID_2);
void kIP_ReleaseReassemblyBuffer(IPREASSEMBLYBUFFER* pstBuffer);
BOOL kIP_InitReassemblyBuffer(IPREASSEMBLYBUFFER* pstBuffer);
BOOL kIP_CheckReassemblyBufferBitmap(IPREASSEMBLYBUFFER* pstBuffer);
void kIP_FillReassemblyBufferBitmap(IPREASSEMBLYBUFFER* pstBuffer, DWORD dwFragmentOffset, DWORD dwDataLength);

WORD kIP_CalcChecksum(IP_HEADER* pstHeader);

BOOL kIP_PutFrameToFrameQueue(const FRAME* pstFrame);
BOOL kIP_GetFrameFromFrameQueue(FRAME* pstFrame);
DWORD kIP_GetIPAddress(BYTE* pbAddress);
BOOL kIP_SetIPAddress(BYTE* pbAddress);
BOOL kIP_SetGatewayIPAddress(BYTE* pbAddress);


#endif // !__IP_H__
