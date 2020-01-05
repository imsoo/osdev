#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"

/*
  Common Exception Handler
*/
void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode)
{
  char vcBuffer[3] = { 0, };

  vcBuffer[0] = '0' + iVectorNumber / 10;
  vcBuffer[1] = '0' + iVectorNumber % 10;

  kPrintStringXY(0, 0, "====================================================");
  kPrintStringXY(0, 1, "                 Exception Occur~!!!!               ");
  kPrintStringXY(0, 2, "                    Vector:                         ");
  kPrintStringXY(27, 2, vcBuffer);
  kPrintStringXY(0, 3, "====================================================");

  while (1);
}

/*
  Common Interrupt Handler
*/
void kCommonInterruptHandler(int iVectorNumber)
{
  char vcBuffer[] = "[INT:  , ]";
  static int g_iCommonInterruptCount = 0;


  vcBuffer[5] = '0' + iVectorNumber / 10;
  vcBuffer[6] = '0' + iVectorNumber % 10;

  vcBuffer[8] = '0' + g_iCommonInterruptCount;
  g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
  kPrintStringXY(70, 0, vcBuffer);

  // EOI Send
  kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

/*
  Keyboard Interrupt Handler
*/
void kKeyboardHandler(int iVectorNumber)
{
  char vcBuffer[] = "[INT:  , ]";
  static int g_iKeyboardInterruptCount = 0;
  BYTE bTemp;

  vcBuffer[5] = '0' + iVectorNumber / 10;
  vcBuffer[6] = '0' + iVectorNumber % 10;

  vcBuffer[8] = '0' + g_iKeyboardInterruptCount;
  g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
  kPrintStringXY(0, 0, vcBuffer);

  if (kIsOutputBufferFull() == TRUE) {
    bTemp = kGetKeyboardScanCode();
    kConvertScanCodeAndPutQueue(bTemp);
  }

  // EOI Send
  kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

/*
  Timer Handler
*/
void kTimerHandler(int iVectorNumber)
{
  char vcBuffer[] = "[INT:  , ]";
  static int g_iTimerInterruptCount = 0;

  vcBuffer[5] = '0' + iVectorNumber / 10;
  vcBuffer[6] = '0' + iVectorNumber % 10;

  vcBuffer[8] = '0' + g_iTimerInterruptCount;
  g_iTimerInterruptCount = (g_iTimerInterruptCount + 1) % 10;
  kPrintStringXY(70, 0, vcBuffer);

  // EOI Send
  kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);

  g_qwTickCount++;

  kDecreaseProcessorTime();
  if (kIsProcessorTimeExpired() == TRUE) {
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

  char vcBuffer[] = "[INT:  , ]";
  static int g_iFPUInterruptCount = 0;

  vcBuffer[5] = '0' + iVectorNumber / 10;
  vcBuffer[6] = '0' + iVectorNumber % 10;

  vcBuffer[8] = '0' + g_iFPUInterruptCount;
  g_iFPUInterruptCount = (g_iFPUInterruptCount + 1) % 10;
  kPrintStringXY(0, 0, vcBuffer);

  kClearTS();

  qwLastFPUTaskID = kGetLastFPUUsedTaskID();
  pstCurrentTask = kGetRunningTask();

  // Last Used Task is mine, nothing to do
  if (qwLastFPUTaskID == pstCurrentTask->stLink.qwID) {
    return;
  }
  // Save other task's FPU Context
  else if (qwLastFPUTaskID != TASK_INVALIDID) {
    pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));
    if ((pstFPUTask != NULL) && (pstFPUTask->stLink.qwID == qwLastFPUTaskID)) {
      kSaveFPUContext(pstFPUTask->vqwFPUContex);
    }
  }

  // if first FPU use, init
  if (pstCurrentTask->bFPUUsed == FALSE) {
    kInitializeFPU();
    pstCurrentTask->bFPUUsed = TRUE;
  }
  // reuse FPU, load FPU context
  else {
    kLoadFPUContext(pstCurrentTask->vqwFPUContex);
  }

  kSetLastFPUUsedTaskID(pstCurrentTask->stLink.qwID);
}
