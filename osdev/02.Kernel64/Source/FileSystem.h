#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

#define FILESYSTEM_SIGNATURE  0x7E38CF10

// Cluster 4KB
#define FILESYSTEM_SECTORPERCLUSTER 8
#define FILESYSTEM_LASTCLUSTER 0xFFFFFFFF
#define FILESYSTEM_FREECLUSTER 0x00
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT ((FILESYSTEM_SECTORPERCLUSTER * 512) / (sizeof(DIRECTORYENTRY)))
#define FILESYSTEM_CLUSTERSIZE (FILESYSTEM_SECTORPERCLUSTER * 512)

#define FILESYSTEM_MAXFILENAMELENGTH 24

typedef BOOL(*fReadHDDInformation)(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
typedef int(*fReadHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
typedef int(*fWriteHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);

#pragma pack(push, 1)

typedef struct kPartitionStruct {
  // Booting Enable Flag
  BYTE bBootableFlag;

  // Partition Start Address, Not Used
  BYTE vbStartingCHSAddress[3];

  // Partition Type
  BYTE bPartitionType;

  // Partition End Address, Not used
  BYTE vbEndingCHSAddress[3];

  // Partition Start LBA Address 
  DWORD dwStartingLBAAddress;

  // Partition Sector
  DWORD dwSizeInSeector;
} PARTITION;

typedef struct kMBRStruct
{
  // Boot loader code
  BYTE vbBootCode[430];

  // File system signature
  DWORD dwSignature;

  // Reserved Sector Count
  DWORD dwReservedSectorCount;

  // Cluster Link Table Sector Count
  DWORD dwClusterLinkSectorCount;

  // Total Cluster Count;
  DWORD dwTotalClusterCount;

  // Partion Table
  PARTITION vstPartition[4];

  // Boot Loader signature 0x55, 0xAA
  BYTE vbBootLoaderSignature[2];
} MBR;

typedef struct kDirectoryEntryStruct
{
  // filename
  char vcFileName[FILESYSTEM_MAXFILENAMELENGTH];

  // file size
  DWORD dwFileSize;

  // Cluster Index
  DWORD dwStartClusterIndex;
} DIRECTORYENTRY;

#pragma pack(pop)

typedef struct kFileSystemManagerStruct
{
  // Init Success Flag
  BOOL bMounted;

  // Res Sector Count
  DWORD dwReservedSectorCount;

  // Cluster Link table start addr and Size
  DWORD dwClusterLinkAreaStartAddress;
  DWORD dwClusterLinkAreaSize;

  // Data Area start addr
  DWORD dwDataAreaStartAddress;

  // Total Cluster
  DWORD dwTotalClusterCount;

  // Last Allocated Cluster Info 
  DWORD dwLastAllocatedClusterLinkSectorOffset;

  // FileSystem Mutex
  MUTEX stMutex;
} FILESYSTEMMANAGER;


// function
BOOL kInitializeFileSystem(void);
BOOL kFormat(void);
BOOL kMount(void);
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation);

BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer);
BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer);
DWORD kFindFreeCluster(void);
BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData);
BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* pdwData);
int kFindFreeDirectoryEntry(void);
BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry);
void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager);

#endif // !__FILESYSTEM_H__
