#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Console.h" // temp

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
  if (gs_pfReadHDDSector(TRUE, TRUE, 0, 2, gs_vbTempBuffer) == FALSE) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }
  pstMBR = (MBR*) gs_vbTempBuffer;
  
  // check filesystem signature
  if (pstMBR->dwSignature != FILESYSTEM_SIGNATURE) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // set FSM 
  gs_stFileSystemManager.bMounted = TRUE;
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
  if (gs_pfReadHDDSector(TRUE, TRUE, 0, 2, gs_vbTempBuffer) == FALSE) {
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // format MBR
  pstMBR = (MBR*) gs_vbTempBuffer;
  kMemSet(pstMBR->vstPartition, 0, sizeof(pstMBR->vstPartition));
  pstMBR->dwSignature = FILESYSTEM_SIGNATURE;
  pstMBR->dwReservedSectorCount = 0;
  pstMBR->dwClusterLinkSectorCount = dwClusterLinkSectorCount;
  pstMBR->dwTotalClusterCount = dwClusterCount;

  // write MBR
  if (gs_pfWriteHDDSector(TRUE, TRUE, 0, 2, gs_vbTempBuffer) == FALSE) {
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

BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer) {
  return gs_pfReadHDDSector(TRUE, TRUE, dwOffset + 
    gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}

BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer)
{
  return gs_pfWriteHDDSector(TRUE, TRUE, dwOffset +
    gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}

BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer)
{
  return gs_pfReadHDDSector(TRUE, TRUE, (dwOffset * FILESYSTEM_SECTORPERCLUSTER) +
    gs_stFileSystemManager.dwDataAreaStartAddress,
    FILESYSTEM_SECTORPERCLUSTER, pbBuffer);
}

BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer)
{
  return gs_pfWriteHDDSector(TRUE, TRUE, (dwOffset * FILESYSTEM_SECTORPERCLUSTER) +
    gs_stFileSystemManager.dwDataAreaStartAddress,
    FILESYSTEM_SECTORPERCLUSTER, pbBuffer);
}

/*
  Get Free Cluster's Offset In Link Table
*/
DWORD kFindFreeCluster(void)
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
      if ((DWORD*)gs_vbTempBuffer[j] == FILESYSTEM_FREECLUSTER) {
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
BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* pdwData)
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
BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData)
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
int kFindFreeDirectoryEntry(void)
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
BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry)
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
BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry)
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
int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry)
{
  DIRECTORYENTRY* pstRootEntry;
  int i, iLength;

  if (gs_stFileSystemManager.bMounted == FALSE)
  {
    return FALSE;
  }

  // read rootdir
  if (kReadCluster(0, gs_vbTempBuffer) == FALSE) {
    return FALSE;
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
