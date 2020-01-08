#include "RAMDisk.h"
#include "DynamicMemory.h"
#include "Synchronization.h"
#include "Utility.h"

static RDDMANAGER gs_stRDDManager;

/*
  Init RDD Manager
*/
BOOL kInitializeRDD(DWORD dwTotalSectorCount)
{
  kMemSet(&gs_stRDDManager, 0, sizeof(gs_stRDDManager));

  gs_stRDDManager.pbBuffer = (BYTE*)kAllocateMemory(dwTotalSectorCount * 512);
  if (gs_stRDDManager.pbBuffer == NULL) {
    return FALSE;
  }

  gs_stRDDManager.dwTotalSectorCount = dwTotalSectorCount;
  kInitializeMutex(&(gs_stRDDManager.stMutex));
  return TRUE;
}

/*
  Get RDD Info
*/
BOOL kReadRDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation)
{
  kMemSet(pstHDDInformation, 0, sizeof(HDDINFORMATION));

  pstHDDInformation->dwTotalSectors = gs_stRDDManager.dwTotalSectorCount;
  kMemCpy(pstHDDInformation->vwSerialNumber, "0000-0000", 9);
  kMemCpy(pstHDDInformation->vwModelNumber, "RAM DISK", 8);

  return TRUE;
}

/*
  Read Sector from RDD
*/
int kReadRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer)
{
  int iRealReadCount;

  iRealReadCount = MIN(gs_stRDDManager.dwTotalSectorCount - (dwLBA + iSectorCount), iSectorCount);

  // copy from RDD
  kMemCpy(pcBuffer, gs_stRDDManager.pbBuffer + (dwLBA * 512), iRealReadCount * 512);
  return iRealReadCount;
}

/*
  Write Sector from RDD
*/
int kWriteRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer)
{
  int iRealWriteCount;

  iRealWriteCount = MIN(gs_stRDDManager.dwTotalSectorCount - (dwLBA + iSectorCount), iSectorCount);

  // copy to RDD
  kMemCpy(gs_stRDDManager.pbBuffer + (dwLBA * 512), pcBuffer, iRealWriteCount * 512);
  return iRealWriteCount;
}