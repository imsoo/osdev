#include "Mouse.h"
#include "Queue.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"

static MOUSEMANAGER gs_stMouseManager = { 0, };
static QUEUE gs_stMouseQueue;
static MOUSEDATA gs_vstMouseQueueBuffer[MOUSE_MAXQUEUECOUNT];

/*
  Init Mouse 
*/
BOOL kInitializeMouse(void)
{
  // Init Queue 
  kInitializeQueue(&gs_stMouseQueue, gs_vstMouseQueueBuffer, MOUSE_MAXQUEUECOUNT, sizeof(MOUSEDATA));

  // Init SpinLock
  kInitializeSpinLock(&(gs_stMouseManager.stSpinLock));

  // Activate Mouse
  if (kActivateMouse() == TRUE) {
    kEnableMouseInterrupt();
    return TRUE;
  }
  return FALSE;
}

/*
  Activate Mouse (Mouse Device, Mouse Enable)
*/
BOOL kActivateMouse(void)
{
  int i, j;
  BOOL bPreviousInterrupt;
  BOOL bResult;

  bPreviousInterrupt = kSetInterruptFlag(FALSE);

  // Send Mouse Device Enable Command (0xA8) to control Register (0x64)
  kOutPortByte(0x64, 0xA8);

  // Send Mouse Command (0xD4) to control Register (0x64)
  kOutPortByte(0x64, 0xD4);

  // wait Input Buffer
  for (i = 0; i < 0xFFFF; i++) {
    if (kIsInputBufferFull() == FALSE) {
      break;
    }
  }

  // Send Mouse Enable Command (0xF4) to Input Buffer (0x60)
  kOutPortByte(0x60, 0xF4);

  // Wait Ack
  bResult = kWaitForAckAndPutOtherScanCode();

  kSetInterruptFlag(bPreviousInterrupt);

  return bResult;
}

/*
  Set Interrupt Flag bit in command Register
*/
void kEnableMouseInterrupt(void)
{
  BYTE bOutputPortData;
  int i;

  // Send Read Command Byte Command (0x20) to control Register (0x64)
  kOutPortByte(0x64, 0x20);

  // wait Output Buffer
  for (i = 0; i < 0xFFFF; i++) {
    if (kIsOutputBufferFull() == TRUE) {
      break;
    }
  }

  // Read From Output Buffer (0x60)
  bOutputPortData = kInPortByte(0x60);

  // Set interrupt Flag 
  bOutputPortData |= 0x02;

  // Send Write Command Byte Command (0x60) to control Register (0x64)
  kOutPortByte(0x64, 0x60);

  // wait Input Buffer
  for (i = 0; i < 0xFFFF; i++) {
    if (kIsInputBufferFull() == FALSE) {
      break;
    }
  }

  // Write Command 
  kOutPortByte(0x60, bOutputPortData);
}

/*
  Accumulate 3 Mouse Data (1 Mouse Packet) and Put Mouse Queue
*/
BOOL kAccumulatedMouseDataAndPutQueue(BYTE bMouseData)
{
  BOOL bResult;

  // Accumulate
  switch (gs_stMouseManager.iByteCount)
  {

  case 0:
    gs_stMouseManager.stCurrentData.bButtonStatusAndFlag = bMouseData;
    gs_stMouseManager.iByteCount++;
    break;

  case 1:
    gs_stMouseManager.stCurrentData.bXMovement = bMouseData;
    gs_stMouseManager.iByteCount++;
    break;

  case 2:
    gs_stMouseManager.stCurrentData.bYMovement = bMouseData;
    gs_stMouseManager.iByteCount++;
    break;

  default:
    gs_stMouseManager.iByteCount = 0;
    break;
  }

  // Put Queue
  if (gs_stMouseManager.iByteCount >= 3) {

    // --- CRITCAL SECTION BEGIN ---
    kLockForSpinLock(&(gs_stMouseManager.stSpinLock));

    bResult = kPutQueue(&gs_stMouseQueue, &gs_stMouseManager.stCurrentData);

    kUnlockForSpinLock(&(gs_stMouseManager.stSpinLock));
    // --- CRITCAL SECTION END ---

    gs_stMouseManager.iByteCount = 0;
  }
}

/*
  Get Mouse Packet From Mouse Queue
*/
BOOL kGetMouseDataFromMouseQueue(BYTE* pbButtonStatus, int* piRelativeX, int* piRelativeY)
{
  MOUSEDATA stData;
  BOOL bResult;

  if (kIsQueueEmpty(&(gs_stMouseQueue)) == TRUE) {
    return FALSE;
  }

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stMouseManager.stSpinLock));

  // Get Mouse Packet
  bResult = kGetQueue(&(gs_stMouseQueue), &stData);

  kUnlockForSpinLock(&(gs_stMouseManager.stSpinLock));
  // --- CRITCAL SECTION END ---

  if (bResult == FALSE) {
    return FALSE;
  }

  // Button Status (0 ~ 2) Bit
  *pbButtonStatus = stData.bButtonStatusAndFlag & 0x7;

  // X Move, if X Sign Bit (4) is Set, Set Negative
  *piRelativeX = stData.bXMovement & 0xFF;
  if (stData.bButtonStatusAndFlag & 0x10) {
    *piRelativeX |= (0xFFFFFF00);
  }

  // Y Move, if Y Sign Bit (5) is Set, Set Negative
  *piRelativeY = stData.bYMovement & 0xFF;
  if (stData.bButtonStatusAndFlag & 0x20) {
    *piRelativeY |= (0xFFFFFF00);
  }
  *piRelativeY = -(*piRelativeY);

  return TRUE;
}

/*
  Read Status Register(0x64) and check AUXB Bit(5)
  1 : Mouse Data
*/
BOOL kIsMouseDataInOutputBuffer(void)
{
  if (kInPortByte(0x64) & 0x20) {
    return TRUE;
  }
  return FALSE;
}