#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"
#include "LocalAPIC.h"
#include "Ethernet.h"

static INTERRUPTMANAGER gs_stInterruptManager;

/*
  Init Interrupt Manager;
*/
void kInitalizeHandler(void)
{
  kMemSet(&gs_stInterruptManager, 0, sizeof(gs_stInterruptManager));
}

/*
  Set Symmetric Mode Flag
*/
void kSetSymmetricIOMode(BOOL bSymmetricIOMode)
{
  gs_stInterruptManager.bSymmetricIOMode = bSymmetricIOMode;
}

/*
  Set LoadBalancing Flag
*/
void kSetInterruptLoadBalancing(BOOL bUseLoadBalancing)
{
  gs_stInterruptManager.bUseLoadBalancing = bUseLoadBalancing;
}

/*
  Increase Interrupt Count
*/
void kIncreaseInterruptCount(int iIRQ)
{
  gs_stInterruptManager.vvqwCoreInterruptCount[kGetAPICID()][iIRQ]++;
}

/*
  Send EOI 
  if Symmetric Mode : Send to LocalAPIC
  else : send to PIC 
*/
void kSendEOI(int iIRQ)
{
  if (gs_stInterruptManager.bSymmetricIOMode == FALSE) {
    kSendEOIToPIC(iIRQ);
  }
  else {
    kSendEOIToLocalAPIC();
  }
}

/*
  Get Interrupt Manager
*/
INTERRUPTMANAGER* kGetInterruptManager(void)
{
  return &gs_stInterruptManager;
}

/*
  Find Min execution Count Core
  and Change I/O Redirection Table to LoadBalancing
*/
void kProcessLoadBalancing(int iIRQ)
{
  QWORD qwMinCount = 0xFFFFFFFFFFFFFFFF;
  int iMinCountCoreIndex;
  int iCoreCount;
  int i;
  BOOL bResetCount = FALSE;
  BYTE bAPICID;

  bAPICID = kGetAPICID();

  // if Count did not over threshhold OR Not Use LoadBalancing
  if ((gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] == 0) ||
    ((gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] % INTERRUPT_LOADBALANCINGDIVIDOR) != 0) ||
    (gs_stInterruptManager.bUseLoadBalancing == FALSE)) {
    return;
  }

  iMinCountCoreIndex = 0;
  iCoreCount = kGetProcessorCount();

  // find Min Count Core
  for (i = 0; i < iCoreCount; i++) {
    if ((gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] < qwMinCount)) {
      qwMinCount = gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ];
      iMinCountCoreIndex = i;
    }
    // prevent overflow
    else if (gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] >= 0xFFFFFFFFFFFFFFFE) {
      bResetCount = TRUE;
    }
  }

  // Change I/O Redirection Table 
  kRoutingIRQToAPICID(iIRQ, iMinCountCoreIndex);

  // Count Table Reset
  if (bResetCount == TRUE) {
    for (i = 0; i < iCoreCount; i++) {
      gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] = 0;
    }
  }
}

/*
  Common Exception Handler
*/
void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode)
{
  char vcBuffer[100];
  BYTE bAPICID;
  TCB* pstTask;

  bAPICID = kGetAPICID();
  pstTask = kGetRunningTask(bAPICID);

  kPrintStringXY(0, 0, "====================================================");
  kPrintStringXY(0, 1, "                 Exception Occur~!!!!               ");
  kSPrintf(vcBuffer, "     Vector:%d     Core ID:0x%X     ErrorCode:0x%X  ",
    iVectorNumber, bAPICID, qwErrorCode);
  kPrintStringXY(0, 2, vcBuffer);
  kSPrintf(vcBuffer, "                Task ID:0x%Q", pstTask->stLink.qwID);
  kPrintStringXY(0, 3, vcBuffer);
  kPrintStringXY(0, 4, "====================================================");

  // UserLevel Task : Exit Task
  if (pstTask->qwFlags & TASK_FLAGS_USERLEVEL) {
    kEndTask(pstTask->stLink.qwID);
    while (1);
  }
  // Kernel Task
  else {
    while (1);
  }

  while (1);
}

/*
  Common Interrupt Handler
*/
void kCommonInterruptHandler(int iVectorNumber)
{
  char vcBuffer[] = "[INT:  , ]";
  static int g_iCommonInterruptCount = 0;
  int iIRQ;

  vcBuffer[5] = '0' + iVectorNumber / 10;
  vcBuffer[6] = '0' + iVectorNumber % 10;

  vcBuffer[8] = '0' + g_iCommonInterruptCount;
  g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
  kPrintStringXY(20, 0, vcBuffer);

  iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

  // Send EOI
  kSendEOI(iIRQ);

  // Update Interrupt Count
  kIncreaseInterruptCount(iIRQ);

  // Interrupt Load Balancing
  kProcessLoadBalancing(iIRQ);
}

/*
  Keyboard Interrupt Handler
*/
void kKeyboardHandler(int iVectorNumber)
{
  char vcBuffer[] = "[INT:  , ]";
  static int g_iKeyboardInterruptCount = 0;
  BYTE bTemp;
  int iIRQ;

  vcBuffer[5] = '0' + iVectorNumber / 10;
  vcBuffer[6] = '0' + iVectorNumber % 10;

  vcBuffer[8] = '0' + g_iKeyboardInterruptCount;
  g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
  kPrintStringXY(0, 0, vcBuffer);

  if (kIsOutputBufferFull() == TRUE) {
    if (kIsMouseDataInOutputBuffer() == FALSE) {
      bTemp = kGetKeyboardScanCode();
      kConvertScanCodeAndPutQueue(bTemp);
    }
    else {
      bTemp = kGetKeyboardScanCode();
      kAccumulatedMouseDataAndPutQueue(bTemp);
    }
  }

  iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

  // Send EOI
  kSendEOI(iIRQ);

  // Update Interrupt Count
  kIncreaseInterruptCount(iIRQ);

  // Interrupt Load Balancing
  kProcessLoadBalancing(iIRQ);
}

/*
  Mouse Interrupt Handler
*/
void kMouseHandler(int iVectorNumber)
{
  char vcBuffer[] = "[INT:  , ]";
  static int g_iMouseInterruptCount = 0;
  BYTE bTemp;
  int iIRQ;

  vcBuffer[5] = '0' + iVectorNumber / 10;
  vcBuffer[6] = '0' + iVectorNumber % 10;

  vcBuffer[8] = '0' + g_iMouseInterruptCount;
  g_iMouseInterruptCount = (g_iMouseInterruptCount + 1) % 10;
  kPrintStringXY(0, 0, vcBuffer);

  if (kIsOutputBufferFull() == TRUE) {
    if (kIsMouseDataInOutputBuffer() == FALSE) {
      bTemp = kGetKeyboardScanCode();
      kConvertScanCodeAndPutQueue(bTemp);
    }
    else {
      bTemp = kGetKeyboardScanCode();
      kAccumulatedMouseDataAndPutQueue(bTemp);
    }
  }

  iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

  // Send EOI
  kSendEOI(iIRQ);

  // Update Interrupt Count
  kIncreaseInterruptCount(iIRQ);

  // Interrupt Load Balancing
  kProcessLoadBalancing(iIRQ);
}


/*
  Timer Handler
*/
void kTimerHandler(int iVectorNumber)
{
  char vcBuffer[] = "[INT:  , ]";
  static int g_iTimerInterruptCount = 0;
  int iIRQ;
  BYTE bCurrentAPICID;

  vcBuffer[5] = '0' + iVectorNumber / 10;
  vcBuffer[6] = '0' + iVectorNumber % 10;

  vcBuffer[8] = '0' + g_iTimerInterruptCount;
  g_iTimerInterruptCount = (g_iTimerInterruptCount + 1) % 10;
  kPrintStringXY(70, 0, vcBuffer);

  iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

  // Send EOI
  kSendEOI(iIRQ);

  // Update Interrupt Count
  kIncreaseInterruptCount(iIRQ);

  // Only BSP
  bCurrentAPICID = kGetAPICID();
  if (bCurrentAPICID == 0) {
    g_qwTickCount++;
  }

  kDecreaseProcessorTime(bCurrentAPICID);

  if (kIsProcessorTimeExpired(bCurrentAPICID) == TRUE) {
    kScheduleInInterrupt();
  }
}

/*
  Device Not Available Handler
*/
void kDeviceNotAvailableHandler(int iVectorNumber)
{
  TCB* pstFPUTask, *pstCurrentTask;
  QWORD qwLastFPUTaskID;
  BYTE bCurrentAPICID;
  
  char vcBuffer[] = "[INT:  , ]";
  static int g_iFPUInterruptCount = 0;

  vcBuffer[5] = '0' + iVectorNumber / 10;
  vcBuffer[6] = '0' + iVectorNumber % 10;

  vcBuffer[8] = '0' + g_iFPUInterruptCount;
  g_iFPUInterruptCount = (g_iFPUInterruptCount + 1) % 10;
  kPrintStringXY(0, 0, vcBuffer);

  bCurrentAPICID = kGetAPICID();

  kClearTS();

  qwLastFPUTaskID = kGetLastFPUUsedTaskID(bCurrentAPICID);
  pstCurrentTask = kGetRunningTask(bCurrentAPICID);

  // Last Used Task is mine, nothing to do
  if (qwLastFPUTaskID == pstCurrentTask->stLink.qwID) {
    return;
  }
  // Save other task's FPU Context
  else if (qwLastFPUTaskID != TASK_INVALIDID) {
    pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));
    if ((pstFPUTask != NULL) && (pstFPUTask->stLink.qwID == qwLastFPUTaskID)) {
      kSaveFPUContext(pstFPUTask->vqwFPUContext);
    }
  }

  // if first FPU use, init
  if (pstCurrentTask->bFPUUsed == FALSE) {
    kInitializeFPU();
    pstCurrentTask->bFPUUsed = TRUE;
  }
  // reuse FPU, load FPU context
  else {
    kLoadFPUContext(pstCurrentTask->vqwFPUContext);
  }

  kSetLastFPUUsedTaskID(bCurrentAPICID, pstCurrentTask->stLink.qwID);
}


/*
  HDD Handler
*/
void kHDDHandler(int iVectorNumber)
{
  char vcBuffer[] = "[INT:  , ]";
  static int g_iHDDInterruptCount = 0;
  int iIRQ;

  vcBuffer[5] = '0' + iVectorNumber / 10;
  vcBuffer[6] = '0' + iVectorNumber % 10;

  vcBuffer[8] = '0' + g_iHDDInterruptCount;
  g_iHDDInterruptCount = (g_iHDDInterruptCount + 1) % 10;
  kPrintStringXY(10, 0, vcBuffer);

  iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;
  // 14 Primary PATA
  if (iIRQ == 14) {
    kSetHDDInterruptFlag(TRUE, TRUE);
  }
  // 15 Second PATA
  else {
    kSetHDDInterruptFlag(FALSE, TRUE);
  }
  
  // Send EOI
  kSendEOI(iIRQ);

  // Update Interrupt Count
  kIncreaseInterruptCount(iIRQ);

  // Interrupt Load Balancing
  kProcessLoadBalancing(iIRQ);
}

void kEthernetHandler(int iVectorNumber)
{
  char vcBuffer[] = "[INT:  , ]";
  static int g_iEthernetInterruptCount = 0;
  int iIRQ;

  vcBuffer[5] = '0' + iVectorNumber / 10;
  vcBuffer[6] = '0' + iVectorNumber % 10;

  vcBuffer[8] = '0' + g_iEthernetInterruptCount;
  g_iEthernetInterruptCount = (g_iEthernetInterruptCount + 1) % 10;
  kPrintStringXY(30, 0, vcBuffer);

  iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

  kEthernet_Handler();

  // Send EOI
  kSendEOI(iIRQ);

  // Update Interrupt Count
  kIncreaseInterruptCount(iIRQ);

  // Interrupt Load Balancing
  kProcessLoadBalancing(iIRQ);
}