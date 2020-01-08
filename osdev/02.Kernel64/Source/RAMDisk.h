#ifndef __RAMDISK_H__
#define __RAMDISK_H__

#include "Types.h"
#include "FileSystem.h"

#define RDD_TOTAL_SECTORCOUNT (8 * 1024 * 1024 / 512)

typedef BOOL(*fReadHDDInformation)(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
typedef int(*fReadHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
typedef int(*fWriteHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);

typedef struct kRDDManagerStruct
{
  // Ram Disk Buffer
  BYTE* pbBuffer;

  // Ram Dist Sector Count
  DWORD dwTotalSectorCount;

  // Sync Object
  MUTEX stMutex;
} RDDMANAGER;

// function
BOOL kInitializeRDD(DWORD dwTotalSectorCount);
BOOL kReadRDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
int kReadRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
int kWriteRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);


#endif // !__RAMDISK_H__
