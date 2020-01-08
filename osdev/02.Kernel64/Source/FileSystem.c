#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Utility.h"

static FILESYSTEMMANAGER gs_stFileSystemManager;
static BYTE gs_vbTempBuffer[FILESYSTEM_SECTORPERCLUSTER * 512];

fReadHDDInformation gs_pfReadHDDInformation = NULL;
fReadHDDSector gs_pfReadHDDSector = NULL;
fWriteHDDSector gs_pfWriteHDDSector = NULL;

/*
  Init File System
*/
BOOL kInitializeFileSystem(void)
{
  kMemSet(&gs_stFileSystemManager, 0, sizeof(gs_stFileSystemManager));
  kInitializeMutex(&(gs_stFileSystemManager.stMutex));

  if (kInitializeHDD() == TRUE) {
    gs_pfReadHDDInformation = kReadHDDInformation;
    gs_pfReadHDDSector = kReadHDDSector;
    gs_pfWriteHDDSector = kWriteHDDSector;
  }
  else {
    return FALSE;
  }

  if (kMount() == FALSE) {
    return FALSE;
  }

  // Handle Pool Init
  gs_stFileSystemManager.pstHandlePool = (FILE*)kAllocateMemory(FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));
  if (gs_stFileSystemManager.pstHandlePool == NULL) {
    gs_stFileSystemManager.bMounted = FALSE;
    return FALSE;
  }
  kMemSet(gs_stFileSystemManager.pstHandlePool, 0, FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));

  return TRUE;
}

/*
  Mount File System
*/
BOOL kMount(void)
{
  MBR* pstMBR;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  // read MBR (read first sector)
  if (gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }
  pstMBR = (MBR*)gs_vbTempBuffer;

  // check filesystem signature
  if (pstMBR->dwSignature != FILESYSTEM_SIGNATURE) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // set FSM 
  gs_stFileSystemManager.bMounted = TRUE;
  gs_stFileSystemManager.dwReservedSectorCount = pstMBR->dwReservedSectorCount;
  gs_stFileSystemManager.dwClusterLinkAreaStartAddress = pstMBR->dwReservedSectorCount + 1;
  gs_stFileSystemManager.dwClusterLinkAreaSize = pstMBR->dwClusterLinkSectorCount;
  gs_stFileSystemManager.dwDataAreaStartAddress = pstMBR->dwReservedSectorCount + pstMBR->dwClusterLinkSectorCount + 1;
  gs_stFileSystemManager.dwTotalClusterCount = pstMBR->dwTotalClusterCount;

  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---
  return TRUE;
}

/*
  Format HDD
*/
BOOL kFormat(void)
{
  HDDINFORMATION* pstHDD;
  MBR* pstMBR;
  DWORD dwTotalSectorCount, dwRemainSectorCount;
  DWORD dwMaxClusterCount, dwClusterCount;
  DWORD dwClusterLinkSectorCount;
  DWORD i;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  // Read HDD Info
  pstHDD = (HDDINFORMATION*)gs_vbTempBuffer;
  if (gs_pfReadHDDInformation(TRUE, TRUE, pstHDD) == FALSE) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // calc Sector per part
  dwTotalSectorCount = pstHDD->dwTotalSectors;
  dwMaxClusterCount = dwTotalSectorCount / FILESYSTEM_SECTORPERCLUSTER;
  dwClusterLinkSectorCount = (dwMaxClusterCount + 127) / 128;

  // Remain Sector = MBR(1) - Cluster Link Table 
  dwRemainSectorCount = dwTotalSectorCount - dwClusterLinkSectorCount - 1;
  dwClusterCount = dwRemainSectorCount / FILESYSTEM_SECTORPERCLUSTER;
  dwClusterLinkSectorCount = (dwClusterCount + 127) / 128;

  // READ MBR
  if (gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // format MBR
  pstMBR = (MBR*)gs_vbTempBuffer;
  kMemSet(pstMBR->vstPartition, 0, sizeof(pstMBR->vstPartition));
  pstMBR->dwSignature = FILESYSTEM_SIGNATURE;
  pstMBR->dwReservedSectorCount = 0;
  pstMBR->dwClusterLinkSectorCount = dwClusterLinkSectorCount;
  pstMBR->dwTotalClusterCount = dwClusterCount;

  // write MBR
  if (gs_pfWriteHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // format remain part (Link Table + Root Dir)
  kMemSet(gs_vbTempBuffer, 0, 512);

  for (i = 0; i < (dwClusterLinkSectorCount + FILESYSTEM_SECTORPERCLUSTER); i++) {
    if (i == 0) {
      // Root Directory 
      ((DWORD*)(gs_vbTempBuffer))[0] = FILESYSTEM_LASTCLUSTER;
    }
    else {
      ((DWORD*)(gs_vbTempBuffer))[0] = FILESYSTEM_FREECLUSTER;
    }

    if (gs_pfWriteHDDSector(TRUE, TRUE, i + 1, 2, gs_vbTempBuffer) == FALSE) {
      kUnlock(&(gs_stFileSystemManager.stMutex));
      // --- CRITCAL SECTION END ---
      return FALSE;
    }
  }

  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---
  return TRUE;
}

/*
  Get GDD Info
*/
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  bResult = gs_pfReadHDDInformation(TRUE, TRUE, pstInformation);

  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---

  return bResult;
}

static BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer) {
  return gs_pfReadHDDSector(TRUE, TRUE, dwOffset +
    gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}

static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer)
{
  return gs_pfWriteHDDSector(TRUE, TRUE, dwOffset +
    gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}

static BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer)
{
  return gs_pfReadHDDSector(TRUE, TRUE, (dwOffset * FILESYSTEM_SECTORPERCLUSTER) +
    gs_stFileSystemManager.dwDataAreaStartAddress,
    FILESYSTEM_SECTORPERCLUSTER, pbBuffer);
}

static BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer)
{
  return gs_pfWriteHDDSector(TRUE, TRUE, (dwOffset * FILESYSTEM_SECTORPERCLUSTER) +
    gs_stFileSystemManager.dwDataAreaStartAddress,
    FILESYSTEM_SECTORPERCLUSTER, pbBuffer);
}

/*
  Get Free Cluster's Offset In Link Table
*/
static DWORD kFindFreeCluster(void)
{
  DWORD dwLinkCountInSector;
  DWORD dwLastSectorOffset, dwCurrentSectorOffset;
  DWORD i, j;

  if (gs_stFileSystemManager.bMounted == FALSE) {
    return FILESYSTEM_LASTCLUSTER;
  }

  dwLastSectorOffset = gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset;

  for (i = 0; i < gs_stFileSystemManager.dwClusterLinkAreaSize; i++) {
    if ((dwLastSectorOffset + i) == (gs_stFileSystemManager.dwClusterLinkAreaSize - 1)) {
      dwLinkCountInSector = gs_stFileSystemManager.dwTotalClusterCount % 128;
    }
    else {
      dwLinkCountInSector = 128;
    }

    dwCurrentSectorOffset = (dwLastSectorOffset + i) %
      gs_stFileSystemManager.dwClusterLinkAreaSize;

    if (kReadClusterLinkTable(dwCurrentSectorOffset, gs_vbTempBuffer) == FALSE) {
      return FILESYSTEM_LASTCLUSTER;
    }

    // find empty cluster
    for (j = 0; j < dwLinkCountInSector; j++) {
      if (((DWORD*)gs_vbTempBuffer)[j] == FILESYSTEM_FREECLUSTER) {
        break;
      }
    }

    // found free cluster
    if (j != dwLinkCountInSector) {
      gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset = dwCurrentSectorOffset;

      return (dwCurrentSectorOffset * 128) + j;
    }
  }

  return FILESYSTEM_LASTCLUSTER;
}

/*
  Get Cluster Link Table Entry
*/
static BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* pdwData)
{
  DWORD dwSectorOffset;

  if (gs_stFileSystemManager.bMounted == FALSE) {
    return FALSE;
  }

  dwSectorOffset = dwClusterIndex / 128;

  if (dwSectorOffset > gs_stFileSystemManager.dwClusterLinkAreaSize) {
    return FALSE;
  }

  // Read Link Table
  if (kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) {
    return FALSE;
  }

  *pdwData = ((DWORD*)gs_vbTempBuffer)[dwClusterIndex % 128];
  return TRUE;
}


/*
  Set Cluster Link Table Entry
*/
static BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData)
{
  DWORD dwSectorOffset;

  if (gs_stFileSystemManager.bMounted == FALSE) {
    return FALSE;
  }

  dwSectorOffset = dwClusterIndex / 128;

  // Read Link Table
  if (kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) {
    return FALSE;
  }

  ((DWORD*)gs_vbTempBuffer)[dwClusterIndex % 128] = dwData;

  // Write Link Table
  if (kWriteClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) {
    return FALSE;
  }

  return TRUE;
}

/*
  Find Free entry in rootdir
*/
static int kFindFreeDirectoryEntry(void)
{
  DIRECTORYENTRY* pstEntry;
  int i;

  if (gs_stFileSystemManager.bMounted == FALSE) {
    return -1;
  }

  if (kReadCluster(0, gs_vbTempBuffer) == FALSE) {
    return -1;
  }

  pstEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
  for (i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++) {
    // Free entry
    if (pstEntry[i].dwStartClusterIndex == 0) {
      return i;
    }
  }

  return -1;
}

/*
  Set entry in rootdir
*/
static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry)
{
  DIRECTORYENTRY* pstRootEntry;

  if ((gs_stFileSystemManager.bMounted == FALSE) ||
    (iIndex < 0) || (iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT))
  {
    return FALSE;
  }

  // read rootdir
  if (kReadCluster(0, gs_vbTempBuffer) == FALSE) {
    return FALSE;
  }

  // set entry
  pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
  kMemCpy(pstRootEntry + iIndex, pstEntry, sizeof(DIRECTORYENTRY));


  // write rootdir
  if (kWriteCluster(0, gs_vbTempBuffer) == FALSE) {
    return FALSE;
  }

  return TRUE;
}

/*
  Get entry in rootdir
*/
static BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry)
{
  DIRECTORYENTRY* pstRootEntry;

  if ((gs_stFileSystemManager.bMounted == FALSE) ||
    (iIndex < 0) || (iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT))
  {
    return FALSE;
  }

  // read rootdir
  if (kReadCluster(0, gs_vbTempBuffer) == FALSE) {
    return FALSE;
  }

  // get entry
  pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
  kMemCpy(pstEntry, pstRootEntry + iIndex, sizeof(DIRECTORYENTRY));

  return TRUE;
}

/*
  Get entry in rootdir using filename
*/
static int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry)
{
  DIRECTORYENTRY* pstRootEntry;
  int i, iLength;

  if (gs_stFileSystemManager.bMounted == FALSE)
  {
    return -1;
  }

  // read rootdir
  if (kReadCluster(0, gs_vbTempBuffer) == FALSE) {
    return -1;
  }

  iLength = kStrLen(pcFileName);
  // get entry
  pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
  for (i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++) {
    if (kMemCmp(pstRootEntry[i].vcFileName, pcFileName, iLength) == 0) {
      kMemCpy(pstEntry, pstRootEntry + i, sizeof(DIRECTORYENTRY));
      return i;
    }
  }

  return -1;
}

/*
  Get FileSystemManager Info
*/
void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager)
{
  kMemCpy(pstManager, &gs_stFileSystemManager, sizeof(gs_stFileSystemManager));
}
/*
  Allocate Handle
*/
static void* kAllocateFileDirectoryHandle(void)
{
  int i;
  FILE* pstFile;

  // find free handle in Handle Pool
  pstFile = gs_stFileSystemManager.pstHandlePool;
  for (i = 0; i < FILESYSTEM_HANDLE_MAXCOUNT; i++) {
    if (pstFile->bType == FILESYSTEM_TYPE_FREE) {
      pstFile->bType = FILESYSTEM_TYPE_FILE;
      return pstFile;
    }
    pstFile++;
  }

  return NULL;
}

/*
  Return Handle
*/
static void kFreeFileDirectoryHandle(FILE* pstFile)
{
  // data clear
  kMemSet(pstFile, 0, sizeof(FILE));
  pstFile->bType = FILESYSTEM_TYPE_FREE;
}

/*
  file open using filename and return handle
*/
FILE* kOpenFile(const char* pcFileName, const char* pcMode)
{
  DIRECTORYENTRY stEntry;
  int iDirectoryEntryOffset;
  int iFileNameLength;

  DWORD dwSecondCluster;
  FILE* pstFile;

  // file name len check
  iFileNameLength = kStrLen(pcFileName);
  if ((iFileNameLength > (sizeof(stEntry.vcFileName) - 1)) ||
    (iFileNameLength == 0)) {
    return NULL;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);

  // File not exist
  if (iDirectoryEntryOffset == -1) {
    // if 'R' mode 
    if (pcMode[0] == 'r') {
      kUnlock(&(gs_stFileSystemManager.stMutex));
      // --- CRITCAL SECTION END ---
      return NULL;
    }

    // create file
    if (kCreateFile(pcFileName, &stEntry, &iDirectoryEntryOffset) == FALSE) {
      kUnlock(&(gs_stFileSystemManager.stMutex));
      // --- CRITCAL SECTION END ---
      return NULL;
    }
  }
  // if file exist, File Clear
  else if (pcMode[0] == 'w') {
    // Get Cluster Link
    if (kGetClusterLinkData(stEntry.dwStartClusterIndex, &dwSecondCluster) == FALSE) {
      kUnlock(&(gs_stFileSystemManager.stMutex));
      // --- CRITCAL SECTION END ---
      return NULL;
    }

    // Set Last Cluster for Clear
    if (kSetClusterLinkData(stEntry.dwStartClusterIndex, FILESYSTEM_LASTCLUSTER) == FALSE) {
      kUnlock(&(gs_stFileSystemManager.stMutex));
      // --- CRITCAL SECTION END ---
      return NULL;
    }

    // File Clear Until end cluster
    if (kFreeClusterUntilEnd(dwSecondCluster) == FALSE) {
      kUnlock(&(gs_stFileSystemManager.stMutex));
      // --- CRITCAL SECTION END ---
      return NULL;
    }

    stEntry.dwFileSize = 0;
    if (kSetDirectoryEntryData(iDirectoryEntryOffset, &stEntry) == FALSE) {
      kUnlock(&(gs_stFileSystemManager.stMutex));
      // --- CRITCAL SECTION END ---
      return NULL;
    }
  }

  // Allocate Handle
  pstFile = kAllocateFileDirectoryHandle();
  if (pstFile == NULL) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return NULL;
  }

  // Set Handle
  pstFile->bType = FILESYSTEM_TYPE_FILE;
  pstFile->stFileHandle.iDirectoryEntryOffset = iDirectoryEntryOffset;
  pstFile->stFileHandle.dwFileSize = stEntry.dwFileSize;
  pstFile->stFileHandle.dwStartClusterIndex = stEntry.dwStartClusterIndex;
  pstFile->stFileHandle.dwCurrentClusterIndex = stEntry.dwStartClusterIndex;
  pstFile->stFileHandle.dwPreviousClusterIndex = stEntry.dwStartClusterIndex;
  pstFile->stFileHandle.dwCurrentOffset = 0;

  // if 'a' mode, move filepointer to end
  if (pcMode[0] == 'a') {
    kSeekFile(pstFile, 0, FILESYSTEM_SEEK_END);
  }

  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---
  return pstFile;
}

/*
  CreateFile
*/
static BOOL kCreateFile(const char* pcFileName, DIRECTORYENTRY* pstEntry,
  int* piDirectoryEntryIndex)
{
  DWORD dwCluster;

  // Find Free Cluster 
  dwCluster = kFindFreeCluster();
  if ((dwCluster == FILESYSTEM_LASTCLUSTER) ||
    (kSetClusterLinkData(dwCluster, FILESYSTEM_LASTCLUSTER) == FALSE))
  {
    return FALSE;
  }

  // Find Free Dir Entry
  *piDirectoryEntryIndex = kFindFreeDirectoryEntry();
  if (*piDirectoryEntryIndex == -1)
  {
    kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
    return FALSE;
  }

  // and Set Entry
  kMemCpy(pstEntry->vcFileName, pcFileName, kStrLen(pcFileName) + 1);
  pstEntry->dwStartClusterIndex = dwCluster;
  pstEntry->dwFileSize = 0;
  if (kSetDirectoryEntryData(*piDirectoryEntryIndex, pstEntry) == FALSE)
  {
    kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
    return FALSE;
  }
  return TRUE;
}

/*
  Free All Cluster in Link
*/
static BOOL kFreeClusterUntilEnd(DWORD dwClusterIndex)
{
  DWORD dwCurrentClusterIndex;
  DWORD dwNextClusterIndex;

  dwCurrentClusterIndex = dwClusterIndex;

  while (dwCurrentClusterIndex != FILESYSTEM_LASTCLUSTER) {
    if (kGetClusterLinkData(dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE) {
      return FALSE;
    }

    if (kSetClusterLinkData(dwCurrentClusterIndex, FILESYSTEM_FREECLUSTER) == FALSE) {
      return FALSE;
    }

    dwCurrentClusterIndex = dwNextClusterIndex;
  }

  return TRUE;
}

/*
  Read file and return read record count
*/
DWORD kReadFile(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile)
{
  DWORD dwTotalCount;
  DWORD dwReadCount;
  DWORD dwOffsetInCluster;
  DWORD dwCopySize;
  FILEHANDLE* pstFileHandle;
  DWORD dwNextClusterIndex;

  if ((pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE)) {
    return 0;
  }
  pstFileHandle = &(pstFile->stFileHandle);

  // if file pointer is end OR Last Cluster
  if ((pstFileHandle->dwCurrentOffset == pstFileHandle->dwFileSize) ||
    (pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER)) {
    return 0;
  }

  dwTotalCount = MIN(dwSize * dwCount, pstFileHandle->dwFileSize - pstFileHandle->dwCurrentOffset);

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  dwReadCount = 0;
  while (dwReadCount != dwTotalCount) {
    // Read Cluster
    if (kReadCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE) {
      break;
    }

    // Calc offset and copysize
    dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;
    dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwReadCount);

    // Copy
    kMemCpy((char*)pvBuffer + dwReadCount, gs_vbTempBuffer + dwOffsetInCluster, dwCopySize);

    dwReadCount += dwCopySize;
    pstFileHandle->dwCurrentOffset += dwCopySize;

    // if Cluster read until end, move next cluster
    if ((pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0) {
      if (kGetClusterLinkData(pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE) {
        break;
      }

      pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
      pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
    }
  }
  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---

  return (dwReadCount / dwSize);
}

/*
  Write file and return writen record count
*/
DWORD kWriteFile(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile)
{
  DWORD dwWriteCount;
  DWORD dwTotalCount;
  DWORD dwOffsetInCluster;
  DWORD dwCopySize;
  DWORD dwAllocatedClusterIndex;
  DWORD dwNextClusterIndex;
  FILEHANDLE* pstFileHandle;

  if ((pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE)) {
    return 0;
  }
  pstFileHandle = &(pstFile->stFileHandle);

  dwTotalCount = dwSize * dwCount;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  dwWriteCount = 0;
  while (dwWriteCount != dwTotalCount) {
    // if current cluster is last cluster
    if (pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER) {
      
      // allocate free cluster to link
      dwAllocatedClusterIndex = kFindFreeCluster();
      if (dwAllocatedClusterIndex == FILESYSTEM_LASTCLUSTER) {
        break;
      }

      // allocated cluster init
      if (kSetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_LASTCLUSTER) == FALSE) {
        break;
      }

      // set link
      if (kSetClusterLinkData(pstFileHandle->dwPreviousClusterIndex, dwAllocatedClusterIndex) == FALSE) {
        kSetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_FREECLUSTER);
        break;
      }

      pstFileHandle->dwCurrentClusterIndex = dwAllocatedClusterIndex;
      kMemSet(gs_vbTempBuffer, 0, sizeof(gs_vbTempBuffer));
    }
    // if current cluster is not last, and file pointer are middle of cluster
    else if (((pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) != 0) ||
      ((dwTotalCount - dwWriteCount) < FILESYSTEM_CLUSTERSIZE)) {
      if (kReadCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE) {
        break;
      }
    }

    // write in cluster
    dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;
    dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwWriteCount);
    kMemCpy(gs_vbTempBuffer + dwOffsetInCluster, (char*)pvBuffer + dwWriteCount, dwCopySize);

    if (kWriteCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE) {
      break;
    }

    dwWriteCount += dwCopySize;
    pstFileHandle->dwCurrentOffset += dwCopySize;

    // if Cluster write until end, move next cluster
    if ((pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0) {

      if (kGetClusterLinkData(pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE) {
        break;
      }

      pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
      pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
    }
  }

  // if file size is change (bigger), update dir entry
  if (pstFileHandle->dwFileSize < pstFileHandle->dwCurrentOffset) {
    pstFileHandle->dwFileSize = pstFileHandle->dwCurrentOffset;
    kUpdateDirectoryEntry(pstFileHandle);
  }

  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---
  return (dwWriteCount / dwSize);
}

/*
  Update Dir Entry Using FileHandle
*/
static BOOL kUpdateDirectoryEntry(FILEHANDLE* pstFileHandle)
{
  DIRECTORYENTRY stEntry;

  if ((pstFileHandle == NULL) ||
    (kGetDirectoryEntryData(pstFileHandle->iDirectoryEntryOffset, &stEntry) == FALSE)) {
    return FALSE;
  }
  stEntry.dwFileSize = pstFileHandle->dwFileSize;
  stEntry.dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;

  if (kSetDirectoryEntryData(pstFileHandle->iDirectoryEntryOffset, &stEntry) == FALSE) {
    return FALSE;
  }
  return TRUE;
}

/*
  Move FileHandle's current offset
*/
int kSeekFile(FILE* pstFile, int iOffset, int iOrigin)
{
  DWORD dwRealOffset;
  DWORD dwClusterOffsetToMove;
  DWORD dwCurrentClusterOffset;
  DWORD dwLastClusterOffset;
  DWORD dwMoveCount;
  DWORD i;
  DWORD dwStartClusterIndex;
  DWORD dwPreviousClusterIndex;
  DWORD dwCurrentClusterIndex;
  FILEHANDLE* pstFileHandle;

  if ((pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE)) {
    return 0;
  }
  pstFileHandle = &(pstFile->stFileHandle);

  switch (iOrigin)
  {
  case FILESYSTEM_SEEK_SET:
    if (iOffset <= 0) {
      dwRealOffset = 0;
    }
    else {
      dwRealOffset = iOffset;
    }
    break;

  case FILESYSTEM_SEEK_CUR:
    if ((iOffset < 0) && (pstFileHandle->dwCurrentOffset <= (DWORD)-iOffset)) {
      dwRealOffset = 0;
    }
    else {
      dwRealOffset = pstFileHandle->dwCurrentOffset + iOffset;
    }
    break;

  case FILESYSTEM_SEEK_END:
    if ((iOffset < 0) && (pstFileHandle->dwFileSize <= (DWORD)-iOffset)) {
      dwRealOffset = 0;
    }
    else {
      dwRealOffset = pstFileHandle->dwFileSize + iOffset;
    }
    break;
  }

  dwLastClusterOffset = pstFileHandle->dwFileSize / FILESYSTEM_CLUSTERSIZE;
  dwClusterOffsetToMove = dwRealOffset / FILESYSTEM_CLUSTERSIZE;
  dwCurrentClusterOffset = pstFileHandle->dwCurrentOffset / FILESYSTEM_CLUSTERSIZE;

  // if ClusterOFfsetToMove is bigger than LasterClusterOFfset
  // move to LastCluster (and Write remain part)
  if (dwLastClusterOffset < dwClusterOffsetToMove) {
    dwMoveCount = dwLastClusterOffset - dwCurrentClusterOffset;
    dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
  }
  // else if ClusterOFfsetToMove is bigger than CurrentClusterOFfset
  // just move from current cluster
  else if (dwCurrentClusterOffset <= dwClusterOffsetToMove) {
    dwMoveCount = dwClusterOffsetToMove - dwCurrentClusterOffset;
    dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
  }
  // else move from start cluster
  else {
    dwMoveCount = dwClusterOffsetToMove;
    dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  // find Cluster
  dwCurrentClusterIndex = dwStartClusterIndex;
  for (i = 0; i < dwMoveCount; i++) {
    dwPreviousClusterIndex = dwCurrentClusterIndex;

    if (kGetClusterLinkData(dwPreviousClusterIndex, &dwCurrentClusterIndex) == FALSE) {
      kUnlock(&(gs_stFileSystemManager.stMutex));
      // --- CRITCAL SECTION END ---
      return -1;
    }
  }

  // FileHandle Cluster Info Update
  if (dwMoveCount > 0) {
    pstFileHandle->dwPreviousClusterIndex = dwPreviousClusterIndex;
    pstFileHandle->dwCurrentClusterIndex = dwCurrentClusterIndex;
  }
  else if (dwStartClusterIndex == pstFileHandle->dwStartClusterIndex) {
    pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwStartClusterIndex;
    pstFileHandle->dwCurrentClusterIndex = pstFileHandle->dwStartClusterIndex;
  }

  // if ClusterOFfsetToMove is bigger than LasterClusterOFfset
  if (dwLastClusterOffset < dwClusterOffsetToMove) {
    pstFileHandle->dwCurrentOffset = pstFileHandle->dwFileSize;

    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---

    // Write remain part
    if (kWriteZero(pstFile, dwRealOffset - pstFileHandle->dwFileSize) == FALSE) {
      return 0;
    }
  }
  pstFileHandle->dwCurrentOffset = dwRealOffset;

  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---
  return 0;
}

/*
  Write Zero from Current File Ooffset
*/
BOOL kWriteZero(FILE* pstFile, DWORD dwCount)
{
  BYTE* pbBuffer;
  DWORD dwRemainCount;
  DWORD dwWriteCount;

  // Handle Check
  if (pstFile == NULL) {
    return FALSE;
  }

  // Allocate buffer for cluster
  pbBuffer = (BYTE*)kAllocateMemory(FILESYSTEM_CLUSTERSIZE);
  if (pbBuffer == NULL) {
    return FALSE;
  }
  kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);

  // Write 
  dwRemainCount = dwCount;
  while (dwRemainCount != 0) {
    dwWriteCount = MIN(dwRemainCount, FILESYSTEM_CLUSTERSIZE);
    if (kWriteFile(pbBuffer, 1, dwWriteCount, pstFile) != dwWriteCount) {
      kFreeMemory(pbBuffer);
      return FALSE;
    }
    dwRemainCount -= dwWriteCount;
  }

  kFreeMemory(pbBuffer);
  return TRUE;
}

/*
  Close Handle
*/
int kCloseFile(FILE* pstFile)
{
  if ((pstFile == NULL) ||
    (pstFile->bType != FILESYSTEM_TYPE_FILE)) {
    return -1;
  }

  kFreeFileDirectoryHandle(pstFile);

  return 0;
}

/*
  Check file opened
*/
BOOL kIsFileOpened(const DIRECTORYENTRY* pstEntry)
{
  int i;
  FILE* pstFile;

  pstFile = gs_stFileSystemManager.pstHandlePool;
  for (i = 0; i < FILESYSTEM_HANDLE_MAXCOUNT; i++) {
    // Scan HandlePool and Check StartCluster Index filed is same
    if ((pstFile[i].bType == FILESYSTEM_TYPE_FILE) &&
      (pstFile[i].stFileHandle.dwStartClusterIndex == pstEntry->dwStartClusterIndex)) {
      return TRUE;
    }
  }

  return FALSE;
}

/*
  Remove file using file name
*/
int kRemoveFile(const char* pcFileName)
{
  DIRECTORYENTRY stEntry;
  int iDirectoryEntryOffset;
  int iFileNameLength;

  iFileNameLength = kStrLen(pcFileName);
  if ((iFileNameLength > (sizeof(stEntry.vcFileName) - 1)) ||
    (iFileNameLength == 0)) {
    return -1;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  // Find Dir Entry
  iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);
  if (iDirectoryEntryOffset == -1) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return -1;
  }

  // check File Opened
  if (kIsFileOpened(&stEntry) == TRUE) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return -1;
  }

  // Free Cluster Link
  if (kFreeClusterUntilEnd(stEntry.dwStartClusterIndex) == FALSE) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return -1;
  }

  // Set Dir Entry
  kMemSet(&stEntry, 0, sizeof(stEntry));
  if (kSetDirectoryEntryData(iDirectoryEntryOffset, &stEntry) == FALSE) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return -1;
  }

  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---
  return 0;
}

/*
  Get Root Dir Handle
*/
DIR* kOpenDirectory(const char* pcDirectoryName)
{
  DIR* pstDirectory;
  DIRECTORYENTRY* pstDirectoryBuffer;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  // Allocate Handle
  pstDirectory = kAllocateFileDirectoryHandle();
  if (pstDirectory == NULL) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return NULL;
  }

  // Allocate Dir buffer
  pstDirectoryBuffer = (DIRECTORYENTRY*)kAllocateMemory(FILESYSTEM_CLUSTERSIZE);
  if (pstDirectoryBuffer == NULL) {
    kFreeFileDirectoryHandle(pstDirectory);
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return NULL;
  }

  // Read Root Dir Cluster
  if (kReadCluster(0, (BYTE*)pstDirectoryBuffer) == FALSE) {
    kFreeFileDirectoryHandle(pstDirectory);
    kFreeMemory(pstDirectoryBuffer);
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return NULL;
  }

  // Set Dir filed
  pstDirectory->bType = FILESYSTEM_TYPE_DIRECTORY;
  pstDirectory->stDirectoryHandle.iCurrentOffset = 0;
  pstDirectory->stDirectoryHandle.pstDirectoryBuffer = pstDirectoryBuffer;

  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---
  return pstDirectory;
}

/*
  Read DIR Entry
*/
struct kDirectoryEntryStruct* kReadDirectory(DIR* pstDirectory) {
  DIRECTORYHANDLE* pstDirectoryHandle;
  DIRECTORYENTRY* pstEntry;

  if ((pstEntry == NULL) ||
    (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY)) {
    return NULL;
  }
  pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

  if ((pstDirectoryHandle->iCurrentOffset < 0) ||
    (pstDirectoryHandle->iCurrentOffset >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT)) {
    return NULL;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  pstEntry = pstDirectoryHandle->pstDirectoryBuffer;
  while (pstDirectoryHandle->iCurrentOffset < FILESYSTEM_MAXDIRECTORYENTRYCOUNT) {
    if (pstEntry[pstDirectoryHandle->iCurrentOffset].dwStartClusterIndex != 0) {
      kUnlock(&(gs_stFileSystemManager.stMutex));
      // --- CRITCAL SECTION END ---
      return &(pstEntry[pstDirectoryHandle->iCurrentOffset++]);
    }
    pstDirectoryHandle->iCurrentOffset++;
  }

  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---
  return NULL;
}

/*
  Rewind : clear Dir current offset
*/
void kRewindDirectory(DIR* pstDirectory)
{
  DIRECTORYHANDLE* pstDirectoryHandle;

  if ((pstDirectory == NULL) || (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY)) {
    return;
  }
  pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  pstDirectoryHandle->iCurrentOffset = 0;

  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---
}

/*
  Close Dir
*/
int kCloseDirectory(DIR* pstDirectory)
{
  DIRECTORYHANDLE* pstDirectoryHandle;

  if ((pstDirectory == NULL) || (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY)) {
    return -1;
  }
  pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stFileSystemManager.stMutex));

  // Free buffer
  kFreeMemory(pstDirectoryHandle->pstDirectoryBuffer);

  // Free Handle
  kFreeFileDirectoryHandle(pstDirectory);

  kUnlock(&(gs_stFileSystemManager.stMutex));
  // --- CRITCAL SECTION END ---

  return 0;
}