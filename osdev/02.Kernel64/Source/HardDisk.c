#include "HardDisk.h"

static HDDMANAGER gs_stHDDManager;

/*
  Init HDD Manager
*/
BOOL kInitializeHDD(void)
{
  // Init Mutex
  kInitializeMutex(&(gs_stHDDManager.stMutex));

  // Interrupt flag
  gs_stHDDManager.bPrimaryInterruptOccur = FALSE;
  gs_stHDDManager.bSecondaryInterruptOccur = FALSE;

  // Interrupt Enable
  kOutPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
  kOutPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);

  // HDD Info Request
  if (kReadHDDInformation(TRUE, TRUE, &(gs_stHDDManager.stHDDInformation))
    == FALSE)
  {
    gs_stHDDManager.bHDDDetected = FALSE;
    gs_stHDDManager.bCanWrite = FALSE;
    return FALSE;
  }

  // QEMU Hard Check
  gs_stHDDManager.bHDDDetected = TRUE;
  if (kMemCmp(gs_stHDDManager.stHDDInformation.vwModelNumber, "QEMU", 4) == 0)
  {
    gs_stHDDManager.bCanWrite = TRUE;
  }
  else
  {
    gs_stHDDManager.bCanWrite = FALSE;
  }
  return TRUE;
}

/*
  Read Status Register
*/
static BYTE kReadHDDStatus(BOOL bPrimary)
{
  if (bPrimary == TRUE)
  {
    // Read Primary HDD's Status Register
    return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
  }
  // Read Secondary HDD's Status Register
  return kInPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_STATUS);
}

/*
  Wait until HDD busy flag clear
*/
static BOOL kWaitForHDDNoBusy(BOOL bPrimary)
{
  QWORD qwStartTickCount;
  BYTE bStatus;

  qwStartTickCount = kGetTickCount();

  // wait until busy flag's is clear
  while ((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME)
  {
    // Read HDD Status Register
    bStatus = kReadHDDStatus(bPrimary);

    // if Busy (bit 7) clear, then return
    if ((bStatus & HDD_STATUS_BUSY) != HDD_STATUS_BUSY)
    {
      return TRUE;
    }
    kSleep(1);
  }
  return FALSE;
}

/*
  Wait until HDD ready flag set
*/
static BOOL kWaitForHDDReady(BOOL bPrimary)
{
  QWORD qwStartTickCount;
  BYTE bStatus;

  qwStartTickCount = kGetTickCount();

  // wait until ready flag's is set
  while ((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME)
  {
    // Read HDD Status Register
    bStatus = kReadHDDStatus(bPrimary);

    // if Busy (bit 6) set, then return
    if ((bStatus & HDD_STATUS_READY) == HDD_STATUS_READY)
    {
      return TRUE;
    }
    kSleep(1);
  }
  return FALSE;
}
/*
  Set Interrupt OCcur flag in HDD Manager
*/
void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag)
{
  if (bPrimary == TRUE)
  {
    gs_stHDDManager.bPrimaryInterruptOccur = bFlag;
  }
  else
  {
    gs_stHDDManager.bSecondaryInterruptOccur = bFlag;
  }
}

/*
  Wait Until HDD Manager Interrupt Occur flag set
*/
static BOOL kWaitForHDDInterrupt(BOOL bPrimary)
{
  QWORD qwTickCount;

  qwTickCount = kGetTickCount();

  // wait until HDD Manager Interrupt Occur flag
  while (kGetTickCount() - qwTickCount <= HDD_WAITTIME)
  {
    if ((bPrimary == TRUE) &&
      (gs_stHDDManager.bPrimaryInterruptOccur == TRUE))
    {
      return TRUE;
    }
    else if ((bPrimary == FALSE) &&
      (gs_stHDDManager.bSecondaryInterruptOccur == TRUE))
    {
      return TRUE;
    }
  }
  return FALSE;
}

/*
  Read HDD Info and Set HDD Manager
*/
BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation)
{
  WORD wPortBase;
  QWORD qwLastTickCount;
  BYTE bStatus;
  BYTE bDriveFlag;
  int i;
  WORD wTemp;
  BOOL bWaitResult;

  // Select Port (Primary, Second)
  if (bPrimary == TRUE)
  {
    wPortBase = HDD_PORT_PRIMARYBASE;
  }
  else
  {
    wPortBase = HDD_PORT_SECONDARYBASE;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stHDDManager.stMutex));

  // Wait until Busy clear
  if (kWaitForHDDNoBusy(bPrimary) == FALSE)
  {
    kUnlock(&(gs_stHDDManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // Set LBA Flag and if Slave, Set Slave Flag
  if (bMaster == TRUE)
  {
    bDriveFlag = HDD_DRIVEANDHEAD_LBA;
  }
  else
  {
    bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
  }
  kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag);

  // Wait until Ready Flag set
  if (kWaitForHDDReady(bPrimary) == FALSE)
  {
    kUnlock(&(gs_stHDDManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // Interrupt Occur flag clear
  kSetHDDInterruptFlag(bPrimary, FALSE);

  // Send command
  kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);

  // wait interrupt
  bWaitResult = kWaitForHDDInterrupt(bPrimary);
  bStatus = kReadHDDStatus(bPrimary);
  if ((bWaitResult == FALSE) ||
    ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR))
  {
    kUnlock(&(gs_stHDDManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // receive data 
  // one sector at a time
  for (i = 0; i < 512 / 2; i++)
  {
    ((WORD*)pstHDDInformation)[i] =
      kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
  }

  // Word Reverse 
  kSwapByteInWord(pstHDDInformation->vwModelNumber,
    sizeof(pstHDDInformation->vwModelNumber) / 2);
  kSwapByteInWord(pstHDDInformation->vwSerialNumber,
    sizeof(pstHDDInformation->vwSerialNumber) / 2);

  kUnlock(&(gs_stHDDManager.stMutex));
  // --- CRITCAL SECTION END ---
  return TRUE;
}

/*
  Word Swap for Byte Order
*/
static void kSwapByteInWord(WORD* pwData, int iWordCount)
{
  int i;
  WORD wTemp;

  for (i = 0; i < iWordCount; i++)
  {
    wTemp = pwData[i];
    pwData[i] = (wTemp >> 8) | (wTemp << 8);
  }
}

/*
  Read Sector
*/
int kReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount,
  char* pcBuffer)
{
  WORD wPortBase;
  int i, j;
  BYTE bDriveFlag;
  BYTE bStatus;
  long lReadCount = 0;
  BOOL bWaitResult;

  // Check HDD Manager Init, SectorCount 
  if ((gs_stHDDManager.bHDDDetected == FALSE) ||
    (iSectorCount <= 0) || (256 < iSectorCount) ||
    ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors))
  {
    return 0;
  }

  // Select Port (Primary, Second)
  if (bPrimary == TRUE)
  {
    wPortBase = HDD_PORT_PRIMARYBASE;
  }
  else
  {
    wPortBase = HDD_PORT_SECONDARYBASE;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stHDDManager.stMutex));

  // Wait until Ready Flag set
  if (kWaitForHDDNoBusy(bPrimary) == FALSE)
  {
    // --- CRITCAL SECTION END ---
    kUnlock(&(gs_stHDDManager.stMutex));
    return FALSE;
  }

  // Sector Count to Read
  kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);
  // Set LBA Address SectorNumber->Cylinder->Head
  // Sector
  kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
  kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
  kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);

  // Set LBA Flag and if Slave, Set Slave Flag
  if (bMaster == TRUE)
  {
    bDriveFlag = HDD_DRIVEANDHEAD_LBA;
  }
  else
  {
    bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
  }
  kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA
    >> 24) & 0x0F));

  // Wait until Ready Flag set
  if (kWaitForHDDReady(bPrimary) == FALSE)
  {
    kUnlock(&(gs_stHDDManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // Interrupt Occur flag clear
  kSetHDDInterruptFlag(bPrimary, FALSE);

  // Send command
  kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ);

  for (i = 0; i < iSectorCount; i++)
  {
    // Read HDD Status Register, and Check error
    bStatus = kReadHDDStatus(bPrimary);
    if ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)
    {
      kPrintf("Error Occur\n");
      kUnlock(&(gs_stHDDManager.stMutex));
      // --- CRITCAL SECTION END ---
      return i;
    }

    // Check DATAREQUEST Bit
    if ((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST)
    {
      // wait Interrupt
      bWaitResult = kWaitForHDDInterrupt(bPrimary);
      kSetHDDInterruptFlag(bPrimary, FALSE);

      if (bWaitResult == FALSE)
      {
        kPrintf("Interrupt Not Occur\n");
        kUnlock(&(gs_stHDDManager.stMutex));
        // --- CRITCAL SECTION END ---
        return FALSE;
      }
    }

    // Read One Sector
    for (j = 0; j < 512 / 2; j++)
    {
      ((WORD*)pcBuffer)[lReadCount++]
        = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
    }
  }

  kUnlock(&(gs_stHDDManager.stMutex));
  // --- CRITCAL SECTION END ---
  return i;
}

/*
  Write Sector
*/
int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount,
  char* pcBuffer)
{
  WORD wPortBase;
  WORD wTemp;
  int i, j;
  BYTE bDriveFlag;
  BYTE bStatus;
  long lReadCount = 0;
  BOOL bWaitResult;

  // Check HDD Manager Init, SectorCount 
  if ((gs_stHDDManager.bCanWrite == FALSE) ||
    (iSectorCount <= 0) || (256 < iSectorCount) ||
    ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors))

  {
    return 0;
  }

  // Select Port (Primary, Second)
  if (bPrimary == TRUE)
  {
    wPortBase = HDD_PORT_PRIMARYBASE;
  }
  else
  {
    wPortBase = HDD_PORT_SECONDARYBASE;
  }

  // Wait until Ready Flag set
  if (kWaitForHDDNoBusy(bPrimary) == FALSE)
  {
    return FALSE;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stHDDManager.stMutex));

  // Sector Count to Write
  kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);

  // Set LBA Address SectorNumber->Cylinder->Head
  kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
  kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
  kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);
  if (bMaster == TRUE)
  {
    bDriveFlag = HDD_DRIVEANDHEAD_LBA;
  }
  else
  {
    bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
  }
  kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA
    >> 24) & 0x0F));

  // Wait until Ready Flag set
  if (kWaitForHDDReady(bPrimary) == FALSE)
  {
    kUnlock(&(gs_stHDDManager.stMutex));
    // --- CRITCAL SECTION END ---
    return FALSE;
  }

  // Send command
  kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE);

  while (1)
  {
    // Read HDD Status Register, and Check error
    bStatus = kReadHDDStatus(bPrimary);
    if ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)
    {
      kUnlock(&(gs_stHDDManager.stMutex));
      // --- CRITCAL SECTION END ---
      return 0;
    }

    // Check DATAREQUEST Bit
    if ((bStatus & HDD_STATUS_DATAREQUEST) == HDD_STATUS_DATAREQUEST)
    {
      break;
    }

    kSleep(1);
  }

  // Write Data
  for (i = 0; i < iSectorCount; i++)
  {
    // Interrupt Occur flag clear, and Write One Sector Data
    kSetHDDInterruptFlag(bPrimary, FALSE);
    for (j = 0; j < 512 / 2; j++)
    {
      kOutPortWord(wPortBase + HDD_PORT_INDEX_DATA,
        ((WORD*)pcBuffer)[lReadCount++]);
    }

    // Interrupt Occur flag clear
    bStatus = kReadHDDStatus(bPrimary);
    if ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)
    {
      kUnlock(&(gs_stHDDManager.stMutex));
      // --- CRITCAL SECTION END ---
      return i;
    }

    // Check DATAREQUEST Bit
    if ((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST)
    {
      // wait Interrupt
      bWaitResult = kWaitForHDDInterrupt(bPrimary);
      kSetHDDInterruptFlag(bPrimary, FALSE);

      if (bWaitResult == FALSE)
      {
        kUnlock(&(gs_stHDDManager.stMutex));
        // --- CRITCAL SECTION END ---
        return FALSE;
      }
    }
  }
  kUnlock(&(gs_stHDDManager.stMutex));
  // --- CRITCAL SECTION END ---
  return i;
}