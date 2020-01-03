#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "Types.h"

#pragma pack (push, 1)

// Queue
typedef struct kQueueManageStruct
{
  // element Size
  int iDataSize;

  // Max element
  int iMaxDataCount;

  // Queue buffer
  void* pvQueueArray;

  // index
  int iPutIndex;
  int iGetIndex;

  // if last operation is put, set TRUE
  BOOL bLastOperationPut;
} QUEUE;

#pragma pack(pop)

// function
void kInitializeQueue(QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize);
BOOL kIsQueueFull(const QUEUE* pstQueue);
BOOL kIsQueueEmpty(const QUEUE* pstQueue);
BOOL kPutQueue(QUEUE* pstQueue, const void* pvData);
BOOL kGetQueue(QUEUE* pstQueue, void* pvData);

#endif  /* __QUEUE_H_ */