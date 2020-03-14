#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"
#include "CacheManager.h"
#include "Task.h"

#define FILESYSTEM_SIGNATURE  0x7E38CF10

// Cluster 4KB
#define FILESYSTEM_SECTORPERCLUSTER 8
#define FILESYSTEM_LASTCLUSTER 0xFFFFFFFF
#define FILESYSTEM_FREECLUSTER 0x00
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT ((FILESYSTEM_SECTORPERCLUSTER * 512) / (sizeof(DIRECTORYENTRY)))
#define FILESYSTEM_CLUSTERSIZE (FILESYSTEM_SECTORPERCLUSTER * 512)

#define FILESYSTEM_MAXFILENAMELENGTH 24

// High Level 
#define FILESYSTEM_HANDLE_MAXCOUNT (TASK_MAXCOUNT * 3)

#define FILESYSTEM_TYPE_FREE  0
#define FILESYSTEM_TYPE_FILE  1
#define FILESYSTEM_TYPE_DIRECTORY 2

#define FILESYSTEM_SEEK_SET 0
#define FILESYSTEM_SEEK_CUR 1
#define FILESYSTEM_SEEK_END 2

#define SEEK_SET    FILESYSTEM_SEEK_SET
#define SEEK_CUR    FILESYSTEM_SEEK_CUR
#define SEEK_END    FILESYSTEM_SEEK_END

#define feof        kEOFFile
#define fopen       kOpenFile
#define fread       kReadFile
#define fwrite      kWriteFile
#define fseek       kSeekFile
#define fclose      kCloseFile
#define remove      kRemoveFile
#define opendir     kOpenDirectory
#define readdir     kReadDirectory
#define rewinddir   kRewindDirectory
#define closedir    kCloseDirectory

#define size_t      DWORD       
#define dirent      kDirectoryEntryStruct
#define d_name      vcFileName

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

typedef struct kFileHandleStruct
{
  // Directory Entry offset
  int iDirectoryEntryOffset;

  // file size
  DWORD dwFileSize;

  // file start cluster 
  DWORD dwStartClusterIndex;

  // current cluster
  DWORD dwCurrentClusterIndex;

  // previous cluster
  DWORD dwPreviousClusterIndex;

  // file pointer offset in cluster
  DWORD dwCurrentOffset;

  // Flags : Using EOF Flags  (Temp)
  DWORD dwFlags
} FILEHANDLE;

typedef struct kDirectoryHandleStruct 
{
  // Root Dir 
  DIRECTORYENTRY* pstDirectoryBuffer;

  // Dir Pointer offset
  int iCurrentOffset;
} DIRECTORYHANDLE;

typedef struct kFileDirectoryHandleStruct
{
  // Type 
  // 0 : Free, 1 : File, 2 : DIR
  BYTE bType;

  union 
  {
    FILEHANDLE stFileHandle;
    DIRECTORYHANDLE stDirectoryHandle;
  };
} FILE, DIR;

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

  // Handle Pool
  FILE* pstHandlePool;

  // Cache Use flag
  BOOL bCacheEnable;
} FILESYSTEMMANAGER;

#pragma pack(pop)

// low level function
BOOL kInitializeFileSystem(void);
BOOL kFormat(void);
BOOL kMount(void);
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation);

static BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer);
static DWORD kFindFreeCluster(void);
static BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* pdwData);
static BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData);
static int kFindFreeDirectoryEntry(void);
static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry);
void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager);

// cache
static CACHEBUFFER* kAllocateCacheBufferWithFlush(int iCacheTableIndex);
BOOL kFlushFileSystemCache(void);

static BOOL kInternalReadClusterLinkTableWithoutCache(DWORD dwOffset,
  BYTE* pbBuffer);
static BOOL kInternalReadClusterLinkTableWithCache(DWORD dwOffset,
  BYTE* pbBuffer);
static BOOL kInternalWriteClusterLinkTableWithoutCache(DWORD dwOffset,
  BYTE* pbBuffer);
static BOOL kInternalWriteClusterLinkTableWithCache(DWORD dwOffset,
  BYTE* pbBuffer);

static BOOL kInternalReadClusterWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalReadClusterWithCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterWithCache(DWORD dwOffset, BYTE* pbBuffer);

// high level function
FILE* kOpenFile(const char* pcFileName, const char* pcMode);
DWORD kReadFile(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
DWORD kWriteFile(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
int kSeekFile(FILE* pstFile, int iOffset, int iOrigin);
int kEOFFile(FILE* pstFile);
BOOL kWriteZero(FILE* pstFile, DWORD dwCount);
int kCloseFile(FILE* pstFile);
BOOL kIsFileOpened(const DIRECTORYENTRY* pstEntry);
int kRemoveFile(const char* pcFileName);
DIR* kOpenDirectory(const char* pcDirectoryName);
struct kDirectoryEntryStruct* kReadDirectory(DIR* pstDirectory);
void kRewindDirectory(DIR* pstDirectory);
int kCloseDirectory(DIR* pstDirectory);

static void* kAllocateFileDirectoryHandle(void);
static void kFreeFileDirectoryHandle(FILE* pstFile);
static BOOL kCreateFile(const char* pcFileName, DIRECTORYENTRY* pstEntry,
  int* piDirectoryEntryIndex);
static BOOL kFreeClusterUntilEnd(DWORD dwClusterIndex);
static BOOL kUpdateDirectoryEntry(FILEHANDLE* pstFileHandle);
#endif // !__FILESYSTEM_H__
