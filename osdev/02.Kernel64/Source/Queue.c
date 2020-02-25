#include "Queue.h"

/*
  Init Queue
*/
void kInitializeQueue(QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize)
{
  pstQueue->iMaxDataCount = iMaxDataCount;
  pstQueue->iDataSize = iDataSize;
  pstQueue->pvQueueArray = pvQueueBuffer;

  pstQueue->iPutIndex = 0;
  pstQueue->iGetIndex = 0;
  pstQueue->bLastOperationPut = FALSE;
}

/*
  return Queue is full
*/
BOOL kIsQueueFull(const QUEUE* pstQueue)
{
  if ((pstQueue->iGetIndex == pstQueue->iPutIndex) &&
    (pstQueue->bLastOperationPut == TRUE))
  {
    return TRUE;
  }
  return FALSE;
}

/*
  return Queue is Empty
*/
BOOL kIsQueueEmpty(const QUEUE* pstQueue)
{
  if ((pstQueue->iGetIndex == pstQueue->iPutIndex) &&
    (pstQueue->bLastOperationPut == FALSE))
  {
    return TRUE;
  }
  return FALSE;
}

/*
  push queue
*/
BOOL kPutQueue(QUEUE* pstQueue, const void* pvData)
{
  if (kIsQueueFull(pstQueue) == TRUE)
    return FALSE;

  kMemCpy((char*)pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iPutIndex),
    pvData, pstQueue->iDataSize);

  pstQueue->iPutIndex = (pstQueue->iPutIndex + 1) % pstQueue->iMaxDataCount;
  pstQueue->bLastOperationPut = TRUE;
  return TRUE;
}

/*
  get queue
*/
BOOL kGetQueue(QUEUE* pstQueue, void* pvData)
{
  if (kIsQueueEmpty(pstQueue) == TRUE)
    return FALSE;

  kMemCpy(pvData, (char*)pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iGetIndex),
    pstQueue->iDataSize);

  pstQueue->iGetIndex = (pstQueue->iGetIndex + 1) % pstQueue->iMaxDataCount;
  pstQueue->bLastOperationPut = FALSE;

  return TRUE;
}

BOOL kFrontQueue(QUEUE* pstQueue, void* pvData)
{
  if (kIsQueueEmpty(pstQueue) == TRUE)
    return FALSE;

  kMemCpy(pvData, (char*)pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iGetIndex),
    pstQueue->iDataSize);

  return TRUE;
}

int kGetQueueSize(QUEUE* pstQueue)
{
  int diff = pstQueue->iGetIndex - pstQueue->iPutIndex;
  return (diff > 0) ? diff : -diff;
}