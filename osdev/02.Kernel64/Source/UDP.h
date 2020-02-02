#ifndef __UDP_H__
#define __UDP_H__

#include "Synchronization.h"
#include "Frame.h"
#include "Queue.h"

#define UDP_PORT_DHCP_SERVER 67
#define UDP_PORT_DHCP_CLIENT 68

#pragma pack(push, 1)

typedef struct kUDPManager
{
  // µø±‚»≠ ∞¥√º
  MUTEX stLock;

  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  UpFunction pfUpDHCP;
  DownFunction pfDownIP;

} UDPMANAGER;

typedef struct kUDPHeader {
  WORD wSourcePort;
  WORD wDestinationPort;
  WORD wLength;
  WORD wChecksum;
} UDP_HEADER;

typedef struct kIPv4PseudoHeader {
  BYTE vbSourceIPAddress[4];
  BYTE vbDestinationIPAddress[4];
  BYTE bZeroes;
  BYTE bProtocol;
  WORD wUDPLength;
} IPv4Pseudo_Header;

#pragma pack(pop)

void kUDP_Task(void);
BOOL kUDP_Initialize(void);

WORD kUDP_CalcChecksum(IPv4Pseudo_Header* pstIPHeader, UDP_HEADER* pstUDPHeader, void* pvData, WORD wDataLen);

BOOL kUDP_DownDirectionPoint(FRAME stFrame);
BOOL kUDP_UpDirectionPoint(FRAME stFrame);

BOOL kUDP_PutFrameToFrameQueue(const FRAME* pstFrame);
BOOL kUDP_GetFrameFromFrameQueue(FRAME* pstFrame);

#endif /* __UDP_H__ */