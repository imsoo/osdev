#include "DynamicMemory.h"
#include "Utility.h"
#include "Task.h"
static DYNAMICMEMORY gs_stDynamicMemory;

void kInitializeDynamicMemory(void)
{
  QWORD qwDynamicMemorySize;
  int i, j;
  BYTE* pbCurrentBitmapPosition;
  int iBlockCountOfLevel, iMetaBlockCount;

  // Calc Part Size
  qwDynamicMemorySize = kCalculateDynamicMemorySize();
  iMetaBlockCount = kCalculateMetaBlockCount(qwDynamicMemorySize);

  // Meta Info init
  gs_stDynamicMemory.iBlockCountOfSmallestBlock = (qwDynamicMemorySize / DYNAMICMEMORY_MIN_SIZE) - iMetaBlockCount;

  // calc Buddy Block Max Level
  for (i = 0; (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) > 0; i++) {
    // Do Nothing
    ;
  }
  gs_stDynamicMemory.iMaxLevelCount = i;

  // Block List Index Init
  gs_stDynamicMemory.pbAllocatedBlockListIndex = (BYTE*)DYNAMICMEMORY_START_ADDRESS;
  for (i = 0; i < gs_stDynamicMemory.iBlockCountOfSmallestBlock; i++) {
    gs_stDynamicMemory.pbAllocatedBlockListIndex[i] = 0xFF;
  }

  // Bitmap Init
  // Bitmap Pointer Start Address
  gs_stDynamicMemory.pstBitmapOfLevel = (BITMAP*)(DYNAMICMEMORY_START_ADDRESS +
    (sizeof(BYTE) * gs_stDynamicMemory.iBlockCountOfSmallestBlock));

  // Bitmap Object Start Address
  pbCurrentBitmapPosition = ((BYTE*)gs_stDynamicMemory.pstBitmapOfLevel) +
    (sizeof(BITMAP) * gs_stDynamicMemory.iMaxLevelCount);

  for (j = 0; j < gs_stDynamicMemory.iMaxLevelCount; j++) {
    gs_stDynamicMemory.pstBitmapOfLevel[j].pbBitmap = pbCurrentBitmapPosition;
    gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 0;
    iBlockCountOfLevel = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> j;

    // Bitmap Object Init
    for (i = 0; i < iBlockCountOfLevel / 8; i++) {
      *pbCurrentBitmapPosition = 0x00;
      pbCurrentBitmapPosition++;
    }

    // Bitmap Object Rest part
    if (iBlockCountOfLevel % 8 != 0) {
      *pbCurrentBitmapPosition = 0x00;
      i = iBlockCountOfLevel % 8;
      if ((i % 2) == 1) {
        *pbCurrentBitmapPosition |= (DYNAMICMEMORY_EXIST << (i - 1));
        gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 1;
      }
      pbCurrentBitmapPosition++;
    }
  }

  // set Address
  gs_stDynamicMemory.qwStartAddress = DYNAMICMEMORY_START_ADDRESS + iMetaBlockCount * DYNAMICMEMORY_MIN_SIZE;
  gs_stDynamicMemory.qwEndAddress = kCalculateDynamicMemorySize() + DYNAMICMEMORY_START_ADDRESS;
  gs_stDynamicMemory.qwUsedSize = 0;
}

/*
  Return Dynamic Memory Size (Byte)
*/
static QWORD kCalculateDynamicMemorySize(void)
{
  QWORD qwRAMSize;

  qwRAMSize = (kGetTotalRAMSize() * 1024 * 1024);

  // Max Total RAM = 3GB
  if (qwRAMSize > (QWORD)3 * 1024 * 1024 * 1024) {
    qwRAMSize = (QWORD)3 * 1024 * 1024 * 1024;
  }

  // Total Ram - System memory
  return qwRAMSize - DYNAMICMEMORY_START_ADDRESS;
}

/*
  Return MetaInfo Part Block Count (DYNAMICMEMORY_MIN_SIZE 1K Byte) Count
  MetaInfo = Block List + Bitmap Pointer + Bitmap
*/
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize)
{
  long lBlockCountOfSmallestBlock;
  DWORD dwSizeOfAllocatedBlockListIndex;
  DWORD dwSizeOfBitmap;
  long i;

  // Calc SmallestBlock Count
  lBlockCountOfSmallestBlock = qwDynamicRAMSize / DYNAMICMEMORY_MIN_SIZE;

  // Calc Block List Index Size 
  dwSizeOfAllocatedBlockListIndex = lBlockCountOfSmallestBlock * sizeof(BYTE);

  dwSizeOfBitmap = 0;
  for (i = 0; (lBlockCountOfSmallestBlock >> i) > 0; i++) {
    // Bitmap Pointer
    dwSizeOfBitmap += sizeof(BITMAP);
    // Bitmap Size 
    dwSizeOfBitmap += ((lBlockCountOfSmallestBlock >> i) + 7) / 8;
  }

  return (dwSizeOfAllocatedBlockListIndex + dwSizeOfBitmap + DYNAMICMEMORY_MIN_SIZE - 1) / DYNAMICMEMORY_MIN_SIZE;
}

void* kAllocateMemory(QWORD qwSize)
{
  QWORD qwAlignedSize;
  QWORD qwRelativeAddress;
  long lOffset;
  int iSizeArrayOffset;
  int iIndexOfBlockList;

  // Find Buddy Block Size
  qwAlignedSize = kGetBuddyBlockSize(qwSize);
  if (qwAlignedSize == 0) {
    return NULL;
  }

  // Not Enough Memory
  if (gs_stDynamicMemory.qwStartAddress + gs_stDynamicMemory.qwUsedSize +
    qwAlignedSize > gs_stDynamicMemory.qwEndAddress) {
    return NULL;
  }

  // Allocation Block and Get Alloctaed Block Offset
  lOffset = kAllocationBuddyBlock(qwAlignedSize);
  if (lOffset == -1) {
    return NULL;
  }

  iIndexOfBlockList = kGetBlockListIndexOfMatchSize(qwAlignedSize);

  qwRelativeAddress = qwAlignedSize * lOffset;
  iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

  // Update BlockList and UsedSize
  gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] = (BYTE)iIndexOfBlockList;
  gs_stDynamicMemory.qwUsedSize += qwAlignedSize;

  return (void*)(qwRelativeAddress + gs_stDynamicMemory.qwStartAddress);
}

/*
  Find Fit Block Size
*/
static QWORD kGetBuddyBlockSize(QWORD qwSize)
{
  long i;
  for (i = 0; i < gs_stDynamicMemory.iMaxLevelCount; i++) {
    if (qwSize <= (DYNAMICMEMORY_MIN_SIZE << i)) {
      return (DYNAMICMEMORY_MIN_SIZE << i);
    }
  }
  return 0;
}

/*
  Allocation Buddy Block
*/
static int kAllocationBuddyBlock(QWORD qwAlignedSize)
{
  int iBlockListIndex, iFreeOffset;
  int i;
  BOOL bPreviousInterruptFlag;

  // get Block List index
  iBlockListIndex = kGetBlockListIndexOfMatchSize(qwAlignedSize);
  if (iBlockListIndex == -1) {
    return -1;
  };

  // --- CRITCAL SECTION BEGIN ---
  bPreviousInterruptFlag = kLockForSystemData();

  // find Block begin i Level to MaxLevel
  for (i = iBlockListIndex; i < gs_stDynamicMemory.iMaxLevelCount; i++) {
    iFreeOffset = kFindFreeBlockInBitmap(i);
    // find
    if (iFreeOffset != -1) {
      break;
    }
  }

  // not find
  if (iFreeOffset == -1) {
    kUnlockForSystemData(bPreviousInterruptFlag);
    // --- CRITCAL SECTION END ---
    return -1;
  }

  kSetFlagInBitmap(i, iFreeOffset, DYNAMICMEMORY_EMPTY);

  // if find block in upper level, split
  if (i > iBlockListIndex) {
    for (i = i - 1; i >= iBlockListIndex; i--) {
      kSetFlagInBitmap(i, iFreeOffset * 2, DYNAMICMEMORY_EMPTY);
      kSetFlagInBitmap(i, iFreeOffset * 2 + 1, DYNAMICMEMORY_EXIST);
      iFreeOffset = iFreeOffset * 2;
    }
  }
  kUnlockForSystemData(bPreviousInterruptFlag);
  // --- CRITCAL SECTION END ---

  return iFreeOffset;
}


/*
  Find Bitmap level Index using Block Size
*/
static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize)
{
  int i;

  for (i = 0; i < gs_stDynamicMemory.iMaxLevelCount; i++) {
    if (qwAlignedSize <= (DYNAMICMEMORY_MIN_SIZE << i))
      return i;
  }
  return -1;
}

/*
  Find Empty Block and Return Offset
*/
static int kFindFreeBlockInBitmap(int iBlockListIndex)
{
  int i, iMaxCount;
  BYTE* pbBitmap;
  QWORD* pqwBitmap;

  if (gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount == 0) {
    return -1;
  }

  iMaxCount = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> iBlockListIndex;
  pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;
  for (i = 0; i < iMaxCount; ) {
    if (((iMaxCount - i) / 64) > 0) {
      pqwBitmap = (QWORD*)&(pbBitmap[i / 8]);
      if (*pqwBitmap == 0) {
        i += 64;
        continue;
      }
    }

    if ((pbBitmap[i / 8] & (DYNAMICMEMORY_EXIST << (i % 8))) != 0) {
      return i;
    }
    i++;
  }
  return -1;
}

/*
  Set Bitmap Flag
*/
static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag)
{
  BYTE* pbBitmap;
  pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;
  if (bFlag == DYNAMICMEMORY_EXIST) {
    if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) == 0) {
      gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount++;
    }
    pbBitmap[iOffset / 8] |= (0x01 << (iOffset % 8));
  }
  else {
    if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0) {
      gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount--;
    }
    pbBitmap[iOffset / 8] &= ~(0x01 << (iOffset % 8));
  }
}

/*
  Free Memory
*/
BOOL kFreeMemory(void* pvAddress)
{
  QWORD qwRelativeAddress;
  int iSizeArrayOffset;
  QWORD qwBlockSize;
  int iBlockListIndex;
  int iBitmapOffset;

  if (pvAddress == NULL) {
    return FALSE;
  }

  qwRelativeAddress = ((QWORD)pvAddress - gs_stDynamicMemory.qwStartAddress);
  iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

  // if not allocated
  if (gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] == 0xFF) {
    return FALSE;
  }

  // get BlockList Index and Init Index
  iBlockListIndex = (int)gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset];
  gs_stDynamicMemory.pbAllocatedBlockListIndex[iBlockListIndex] = 0xFF;

  qwBlockSize = DYNAMICMEMORY_MIN_SIZE << iBlockListIndex;
  iBitmapOffset = qwRelativeAddress / qwBlockSize;

  // Free Buddy Block
  if (kFreeBuddyBlock(iBlockListIndex, iBitmapOffset) == TRUE) {
    gs_stDynamicMemory.qwUsedSize -= qwBlockSize;
    return TRUE;
  }

  return FALSE;
}

/*
  Free Buddy Block, Begin iBlockListIndex to MaxLevel
*/
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset)
{
  int iBuddyBlockOffset;
  int i;
  BOOL bFlag;
  BOOL bPreviousInterruptFlag;

  // --- CRITCAL SECTION BEGIN ---
  bPreviousInterruptFlag = kLockForSystemData();

  for (i = iBlockListIndex; i < gs_stDynamicMemory.iMaxLevelCount; i++) {
    // set block exist
    kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EXIST);

    // find buddy block index
    if ((iBlockOffset % 2) == 0) {
      iBuddyBlockOffset = iBlockOffset + 1;
    }
    else {
      iBuddyBlockOffset = iBlockOffset - 1;
    }
    bFlag = kGetFlagInBitmap(i, iBuddyBlockOffset);

    // if buddy block is empty
    if (bFlag == DYNAMICMEMORY_EMPTY) {
      break;
    }

    kSetFlagInBitmap(i, iBuddyBlockOffset, DYNAMICMEMORY_EMPTY);
    kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EMPTY);

    // goto next level
    iBlockOffset = iBlockOffset / 2;
  }
  kUnlockForSystemData(bPreviousInterruptFlag);
  // --- CRITCAL SECTION END ---
  return TRUE;
}

/*
  Get Bitmap Flag
*/
static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset)
{
  BYTE* pbBitmap;

  pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;

  if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0x00) {
    return DYNAMICMEMORY_EXIST;
  }
  return DYNAMICMEMORY_EMPTY;
}

/*
  Get Dynamic Memory Info
*/
void kGetDynamicMemoryInformation(QWORD* pqwDynamicMemoryStartAddress, QWORD* pqwDynamicMemoryTotalSize,
  QWORD* pqwMetaDataSize, QWORD* pqwUsedMemorySize)
{
  *pqwDynamicMemoryStartAddress = DYNAMICMEMORY_START_ADDRESS;
  *pqwDynamicMemoryTotalSize = kCalculateDynamicMemorySize();
  *pqwMetaDataSize = kCalculateMetaBlockCount(*pqwDynamicMemoryTotalSize) * DYNAMICMEMORY_MIN_SIZE;
  *pqwUsedMemorySize = gs_stDynamicMemory.qwUsedSize;
}

/*
  Get DynamicMemory Manager
*/
DYNAMICMEMORY* kGetDynamicMemoryManager(void)
{
  return &gs_stDynamicMemory;
}
