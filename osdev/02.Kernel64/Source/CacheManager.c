#include "CacheManager.h"
#include "DynamicMemory.h"
#include "FileSystem.h"

static CACHEMANAGER gs_stCacheManager;

/*
  Init CacheManager
*/
BOOL kInitializeCacheManager(void)
{
  int i;

  // Clear CacheManager
  kMemSet(&gs_stCacheManager, 0, sizeof(gs_stCacheManager));

  // Init Access Time
  gs_stCacheManager.vdwAccessTime[CACHE_CLUSTERLINKTABLEAREA] = 0;
  gs_stCacheManager.vdwAccessTime[CACHE_DATAAREA] = 0;

  // Set Max Count 
  gs_stCacheManager.vdwMaxCount[CACHE_CLUSTERLINKTABLEAREA] = CACHE_MAXCLUSTERLINKTABLECOUNT;
  gs_stCacheManager.vdwMaxCount[CACHE_DATAAREA] = CACHE_MAXDATAAREACOUNT;

  // Set Buffer (CLUSTER)
  // Allocate Buffer
  gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] =
    (BYTE*)kAllocateMemory(CACHE_MAXCLUSTERLINKTABLECOUNT * 512);
  if (gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] == NULL) {
    return FALSE;
  }

  // Split and Set Pointer
  for (i = 0; i < CACHE_MAXCLUSTERLINKTABLECOUNT; i++) {
    gs_stCacheManager.vvstCacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].pbBuffer =
      gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] + (i * 512);

    // Set Empty Cache
    gs_stCacheManager.vvstCacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].dwTag = CACHE_INVALIDTAG;
  }

  // Set Buffer (DATA)
  // Allocate Buffer
  gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] =
    (BYTE*)kAllocateMemory(CACHE_MAXDATAAREACOUNT * FILESYSTEM_CLUSTERSIZE);
  if (gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] == NULL) {
    // if Fail, free CLUSTERLINK Memory
    kFreeMemory(gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA]);
    return FALSE;
  }

  // Split and Set Pointer
  for (i = 0; i < CACHE_MAXDATAAREACOUNT; i++) {
    gs_stCacheManager.vvstCacheBuffer[CACHE_DATAAREA][i].pbBuffer =
      gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] + (i * FILESYSTEM_CLUSTERSIZE);

    // Set Empty Cache
    gs_stCacheManager.vvstCacheBuffer[CACHE_DATAAREA][i].dwTag = CACHE_INVALIDTAG;
  }

  return TRUE;
}

/*
  Return Empty Cache in cache Buffer
*/
CACHEBUFFER* kAllocateCacheBuffer(int iCacheTableIndex)
{
  CACHEBUFFER* pstCacheBuffer;
  int i;

  if (iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
    return FALSE;
  }

  kCutDownAccessTime(iCacheTableIndex);

  pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
  for (i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
    // if find empty Cache
    if (pstCacheBuffer[i].dwTag == CACHE_INVALIDTAG) {
      // Set cache is used
      pstCacheBuffer[i].dwTag = CACHE_INVALIDTAG - 1;
      // AccessTime Update
      pstCacheBuffer[i].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;

      return &(pstCacheBuffer[i]);
    }
  }
  return NULL;
}

/*
  Return Cache in cache Buffer using Tag
*/
CACHEBUFFER* kFindCacheBuffer(int iCacheTableIndex, DWORD dwTag)
{
  CACHEBUFFER* pstCacheBuffer;
  int i;

  if (iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
    return FALSE;
  }

  kCutDownAccessTime(iCacheTableIndex);

  pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
  for (i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
    if (pstCacheBuffer[i].dwTag == dwTag) {
      pstCacheBuffer[i].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;

      return &(pstCacheBuffer[i]);
    }
  }

  return NULL;
}

/*
  Cut Down Access Time In Cache Manager
*/
static void kCutDownAccessTime(int iCacheTableIndex)
{
  CACHEBUFFER stTemp;
  CACHEBUFFER* pstCacheBuffer;
  BOOL bSorted;
  int i, j;

  if (iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
    return FALSE;
  }

  if (gs_stCacheManager.vdwAccessTime[iCacheTableIndex] < 0xfffffffe) {
    return;
  }

  // Bubble Sort
  pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
  for (j = 0; j < gs_stCacheManager.vdwMaxCount[iCacheTableIndex] - 1; j++) {
    bSorted = TRUE;
    for (i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex] - 1 - j; i++) {
      if (pstCacheBuffer[i].dwAccessTime > pstCacheBuffer[i + 1].dwAccessTime) {
        bSorted = FALSE;

        // swap
        kMemCpy(&stTemp, &(pstCacheBuffer[i]), sizeof(CACHEBUFFER));
        kMemCpy(&(pstCacheBuffer[i]), &(pstCacheBuffer[i+1]), sizeof(CACHEBUFFER));
        kMemCpy(&(pstCacheBuffer[i+1]), &stTemp, sizeof(CACHEBUFFER));
      }
    }

    if (bSorted == TRUE) {
      break;
    }
  }

  // Cut Down
  for (i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
    pstCacheBuffer[i].dwAccessTime = i;
  }
  gs_stCacheManager.vdwAccessTime[iCacheTableIndex] = i;
}

/*
  Find Buffer to Swap out
*/
CACHEBUFFER* kGetVictimInCacheBuffer(int iCacheTableIndex)
{
  DWORD dwOldTime;
  CACHEBUFFER* pstCacheBuffer;
  int iOldIndex;
  int i;

  if (iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
    return FALSE;
  }

  iOldIndex = -1;
  dwOldTime = 0xFFFFFFFF;

  // Scan Cache Buffer from first index
  pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
  for (i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
    if (pstCacheBuffer[i].dwTag == CACHE_INVALIDTAG) {
      iOldIndex = i;
      break;
    }

    if (pstCacheBuffer[i].dwAccessTime < dwOldTime) {
      dwOldTime = pstCacheBuffer[i].dwAccessTime;
      iOldIndex = i;
    }
  }

  if (iOldIndex == -1) {
    kPrintf("Cache Buffer FInd Error\n");
    return NULL;
  }

  pstCacheBuffer[iOldIndex].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;

  return &(pstCacheBuffer[iOldIndex]);
}

/*
  All Cache Buffer Clear
*/
void kDiscardAllCacheBuffer(int iCacheTableIndex)
{
  CACHEBUFFER* pstCacheBuffer;
  int i;

  pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
  for (i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
    pstCacheBuffer[i].dwTag = CACHE_INVALIDTAG;
  }

  gs_stCacheManager.vdwAccessTime[iCacheTableIndex] = 0;
}

/*
  Get Cache Info for Access directly
*/
BOOL kGetCacheBufferCount(int iCacheTableIndex, CACHEBUFFER** ppstCacheBuffer, int* piMaxCount)
{
  if (iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
    return FALSE;
  }

  *ppstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
  *piMaxCount = gs_stCacheManager.vdwMaxCount[iCacheTableIndex];
  return TRUE;
}

