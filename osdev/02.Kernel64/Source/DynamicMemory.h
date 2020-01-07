#ifndef __DYNAMICMEMORY_H__
#define __DYNAMICMEMORY_H__

#include "Types.h"

#define DYNAMICMEMORY_START_ADDRESS ((TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * TASK_MAXCOUNT) + 0xfffff) & 0xfffffffffff00000)

// Buddy Block Min Size : 1KB
#define DYNAMICMEMORY_MIN_SIZE (1 * 1024)

#define DYNAMICMEMORY_EXIST 0x01
#define DYNAMICMEMORY_EMPTY 0x00

#pragma pack(push, 1)

typedef struct kBitmapStruct
{
  BYTE* pbBitmap;
  QWORD qwExistBitCount;
} BITMAP;

typedef struct kDynamicMemoryManagerStruct {
  // Block List Count
  int iMaxLevelCount;

  // Smallest Block(1K) Count
  int iBlockCountOfSmallestBlock;

  // allocated memory size
  QWORD qwUsedSize;

  // Block Pool Address
  QWORD qwStartAddress;
  QWORD qwEndAddress;

  // Block List index
  BYTE* pbAllocatedBlockListIndex;

  // Block List Bitmap
  BITMAP* pstBitmapOfLevel;
} DYNAMICMEMORY;

#pragma pack(pop)

// fucntion
void kInitializeDynamicMemory(void);
void* kAllocateMemory(QWORD qwSize);
BOOL kFreeMemory(void* pvAddress);
void kGetDynamicMemoryInformation(QWORD* pqwDynamicMemoryStartAddress, QWORD* pqwDynamicMemoryTotalSize,
  QWORD* pqwMetaDataSize, QWORD* pqwUsedMemorySize);
DYNAMICMEMORY* kGetDynamicMemoryManager(void);

static QWORD kCalculateDynamicMemorySize(void);
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize);
static int kAllocationBuddyBlock(QWORD qwAlignedSize);
static QWORD kGetBuddyBlockSize(QWORD qwSize);
static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize);
static int kFindFreeBlockInBitmap(int iBlockListIndex);
static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag);
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset);
static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset);

#endif  /* __DYNAMICMEMORY_H__ */