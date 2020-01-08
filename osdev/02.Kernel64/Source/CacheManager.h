#ifndef __CACHEMANAGER_H__
#define __CACHEMANAGER_H__

#include "Types.h"

#define CACHE_MAXCLUSTERLINKTABLECOUNT 16
#define CACHE_MAXDATAAREACOUNT 32

#define CACHE_MAXCACHETABLEINDEX  2
#define CACHE_CLUSTERLINKTABLEAREA 0
#define CACHE_DATAAREA 1

#define CACHE_INVALIDTAG  0xFFFFFFFF
typedef struct kCacheBufferStruct 
{
  // ID (INDEX)
  DWORD dwTag;

  // Recent AccessedTime
  DWORD dwAccessTime;

  // Is Changed
  BOOL bChanged;

  // Data Buffer
  BYTE* pbBuffer;
} CACHEBUFFER;

typedef struct kCacheManagerStruct
{
  // AccessedTime Array
  DWORD vdwAccessTime[CACHE_MAXCACHETABLEINDEX];

  // Cache Buffer Pointer Array
  BYTE* vpbBuffer[CACHE_MAXCACHETABLEINDEX];

  // Cache
  CACHEBUFFER vvstCacheBuffer[CACHE_MAXCACHETABLEINDEX][CACHE_MAXDATAAREACOUNT];

  // Cache Buffer Max Size
  DWORD vdwMaxCount[CACHE_MAXCACHETABLEINDEX];
} CACHEMANAGER;


// function
BOOL kInitializeCacheManager(void);
CACHEBUFFER* kAllocateCacheBuffer(int iCacheTableIndex);
CACHEBUFFER* kFindCacheBuffer(int iCacheTableIndex, DWORD dwTag);
static void kCutDownAccessTime(int iCacheTableIndex);
CACHEBUFFER* kGetVictimInCacheBuffer(int iCacheTableIndex);
void kDiscardAllCacheBuffer(int iCacheTableIndex);
BOOL kGetCacheBufferCount(int iCacheTableIndex, CACHEBUFFER** ppstCacheBuffer, int* piMaxCount);


#endif // !__CACHEMANAGER_H__
