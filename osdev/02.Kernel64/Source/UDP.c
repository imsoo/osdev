#ifndef __UDP_H__
#define __UDP_H__

#include "Frame.h"
#include "Queue.h"

#pragma pack(push, 1)

typedef struct kUDPManager
{
  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  UpFunction pfUp;
} UDPMANAGER;

typedef struct kUDPHeader {
  DWORD dwSourcePort;
  DWORD dwDestinationPort;
  DWORD dwLength;
  DWORD dwChecksum;
} UDP_HEADER;

#pragma pack(pop)

#endif /* __UDP_H__ */