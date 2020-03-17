#include "SystemCall.h"
#include "SystemCallList.h"
#include "Descriptor.h"
#include "Task.h"
#include "AssemblyUtility.h"
#include "FileSystem.h"
#include "DynamicMemory.h"
#include "RTC.h"
#include "Utility.h"
#include "Window.h"
#include "JPEG.h"
#include "Loader.h"
#include "TCP.h"
#include "DNS.h"

/*
  Initialize System Call MSR Register
*/
void kInitializeSystemCall(void)
{
  QWORD qwRDX, qwRAX;

  // IA32_STAR MSR(0xC0000081)
  qwRDX = ((GDT_KERNELDATASEGMENT | SELECTOR_RPL_3) << 16) | GDT_KERNELCODESEGMENT;
  qwRAX = 0;
  kWriteMSR(0xC0000081, qwRDX, qwRAX);

  // IA32_LSTAR MSR(0xC0000082)
  kWriteMSR(0xC0000082, 0, (QWORD)kSystemCallEntryPoint);

  // IA32_FMASK MSR(0xC0000084)
  kWriteMSR(0xC0000084, 0, 0x00);
}

QWORD kProcessSystemCall(QWORD qwServiceNumber, PARAMETERTABLE* pstParameter)
{
  TCB* pstTCB;

  switch (qwServiceNumber)
  {

  // Console I/O
  case SYSCALL_CONSOLEPRINTSTRING:
    return kConsolePrintString(PARAM(0));
  case SYSCALL_SETCURSOR:
    kSetCursor(PARAM(0), PARAM(1));
    return TRUE;
  case SYSCALL_GETCURSOR:
    kGetCursor((int*)PARAM(0), (int*)PARAM(1));
    return TRUE;
  case SYSCALL_CLEARSCREEN:
    kClearScreen();
    return TRUE;
  case SYSCALL_GETCH:
    return kGetCh();

  // Dynamic Memory
  case SYSCALL_MALLOC:
    return (QWORD)kAllocateMemory(PARAM(0));
  case SYSCALL_REALLOC:
    return (QWORD)kReallocateMemory((void*)PARAM(0), PARAM(1));
  case SYSCALL_CALLOC:
    return (QWORD)kCallocateMemory(PARAM(0), PARAM(1));
  case SYSCALL_FREE:
    return kFreeMemory((void*)PARAM(0));

  // File I/O
  case SYSCALL_FOPEN:
    return (QWORD)fopen((char*)PARAM(0), (char*)PARAM(1));
  case SYSCALL_FREAD:
    return fread((void*)PARAM(0), PARAM(1), PARAM(2),
      (FILE*)PARAM(3));
  case SYSCALL_FWRITE:
    return fwrite((void*)PARAM(0), PARAM(1), PARAM(2),
      (FILE*)PARAM(3));
  case SYSCALL_FSEEK:
    return fseek((FILE*)PARAM(0), PARAM(1), PARAM(2));
  case SYSCALL_FEOF:
    return feof((FILE*)PARAM(0));
  case SYSCALL_FCLOSE:
    return fclose((FILE*)PARAM(0));
  case SYSCALL_REMOVE:
    return remove((char*)PARAM(0));
  case SYSCALL_OPENDIR:
    return (QWORD)opendir((char*)PARAM(0));
  case SYSCALL_READDIR:
    return (QWORD)readdir((DIR*)PARAM(0));
  case SYSCALL_REWINDDIR:
    rewinddir((DIR*)PARAM(0));
    return TRUE;
  case SYSCALL_CLOSEDIR:
    return closedir((DIR*)PARAM(0));
  case SYSCALL_ISFILEOPENED:
    return kIsFileOpened((DIRECTORYENTRY*)PARAM(0));

  // HDD I/O
  case SYSCALL_READHDDSECTOR:
    return kReadHDDSector(PARAM(0), PARAM(1), PARAM(2), PARAM(3),
      (char*)PARAM(4));
  case SYSCALL_WRITEHDDSECTOR:
    return kWriteHDDSector(PARAM(0), PARAM(1), PARAM(2), PARAM(3),
      (char*)PARAM(4));

  // Task
  case SYSCALL_CREATETASK:
    pstTCB = kCreateTask(PARAM(0), (void*)PARAM(1), PARAM(2),
      PARAM(3), PARAM(4));
    if (pstTCB == NULL)
    {
      return TASK_INVALIDID;
    }
    return pstTCB->stLink.qwID;
  case SYSCALL_SCHEDULE:
    return kSchedule();
  case SYSCALL_CHANGEPRIORITY:
    return kChangePriority(PARAM(0), PARAM(1));
  case SYSCALL_ENDTASK:
    return kEndTask(PARAM(0));
  case SYSCALL_EXITTASK:
    kExitTask();
    return TRUE;
  case SYSCALL_GETTASKCOUNT:
    return kGetTaskCount(PARAM(0));
  case SYSCALL_ISTASKEXIST:
    return kIsTaskExist(PARAM(0));
  case SYSCALL_GETPROCESSORLOAD:
    return kGetProcessorLoad(PARAM(0));
  case SYSCALL_CHANGEPROCESSORAFFINITY:
    return kChangePriority(PARAM(0), PARAM(1));
  case SYSCALL_EXECUTEPROGRAM:
    return kExecuteProgram((char*)PARAM(0), (char*)PARAM(1), (BYTE)PARAM(2));
  case SYSCALL_CREATETHREAD:
    return kCreateThread(PARAM(0), PARAM(1), (BYTE)PARAM(2), PARAM(3));

  // GUI
  case SYSCALL_GETBACKGROUNDWINDOWID:
    return kGetBackgroundWindowID();
  case SYSCALL_GETSCREENAREA:
    kGetScreenArea((RECT*)PARAM(0));
    return TRUE;
  case SYSCALL_CREATEWINDOW:
    return kCreateWindow(PARAM(0), PARAM(1), PARAM(2), PARAM(3),
      PARAM(4), (char*)PARAM(5));
  case SYSCALL_DELETEWINDOW:
    return kDeleteWindow(PARAM(0));
  case SYSCALL_SHOWWINDOW:
    return kShowWindow(PARAM(0), PARAM(1));
  case SYSCALL_FINDWINDOWBYPOINT:
    return kFindWindowByPoint(PARAM(0), PARAM(1));
  case SYSCALL_FINDWINDOWBYTITLE:
    return kFindWindowByTitle((char*)PARAM(0));
  case SYSCALL_ISWINDOWEXIST:
    return kIsWindowExist(PARAM(0));
  case SYSCALL_GETTOPWINDOWID:
    return kGetTopWindowID();
  case SYSCALL_MOVEWINDOWTOTOP:
    return kMoveWindowToTop(PARAM(0));
  case SYSCALL_ISINTITLEBAR:
    return kIsInTitleBar(PARAM(0), PARAM(1), PARAM(2));
  case SYSCALL_ISINCLOSEBUTTON:
    return kIsInCloseButton(PARAM(0), PARAM(1), PARAM(2));
  case SYSCALL_MOVEWINDOW:
    return kMoveWindow(PARAM(0), PARAM(1), PARAM(2));
  case SYSCALL_RESIZEWINDOW:
    return kResizeWindow(PARAM(0), PARAM(1), PARAM(2), PARAM(3),
      PARAM(4));
  case SYSCALL_GETWINDOWAREA:
    return kGetWindowArea(PARAM(0), (RECT*)PARAM(1));
  case SYSCALL_UPDATESCREENBYID:
    return kUpdateScreenByID(PARAM(0));
  case SYSCALL_UPDATESCREENBYWINDOWAREA:
    return kUpdateScreenByWindowArea(PARAM(0), (RECT*)PARAM(1));
  case SYSCALL_UPDATESCREENBYSCREENAREA:
    return kUpdateScreenByScreenArea((RECT*)PARAM(0));
  case SYSCALL_SENDEVENTTOWINDOW:
    return kSendEventToWindow(PARAM(0), (EVENT*)PARAM(1));
  case SYSCALL_RECEIVEEVENTFROMWINDOWQUEUE:
    return kReceiveEventFromWindowQueue(PARAM(0), (EVENT*)PARAM(1));
  case SYSCALL_DRAWWINDOWFRAME:
    return kDrawWindowFrame(PARAM(0));
  case SYSCALL_DRAWWINDOWBACKGROUND:
    return kDrawWindowBackground(PARAM(0));
  case SYSCALL_DRAWWINDOWTITLE:
    return kDrawWindowTitle(PARAM(0), (char*)PARAM(1), PARAM(2));
  case SYSCALL_DRAWBUTTON:
    return kDrawButton(PARAM(0), (RECT*)PARAM(1), PARAM(2),
      (char*)PARAM(3), PARAM(4));
  case SYSCALL_DRAWPIXEL:
    return kDrawPixel(PARAM(0), PARAM(1), PARAM(2), PARAM(3));
  case SYSCALL_DRAWLINE:
    return kDrawLine(PARAM(0), PARAM(1), PARAM(2), PARAM(3),
      PARAM(4), PARAM(5));
  case SYSCALL_DRAWRECT:
    return kDrawRect(PARAM(0), PARAM(1), PARAM(2), PARAM(3),
      PARAM(4), PARAM(5), PARAM(6));
  case SYSCALL_DRAWCIRCLE:
    return kDrawCircle(PARAM(0), PARAM(1), PARAM(2), PARAM(3),
      PARAM(4), PARAM(5));
  case SYSCALL_DRAWTEXT:
    return kDrawText(PARAM(0), PARAM(1), PARAM(2), PARAM(3),
      PARAM(4), (char*)PARAM(5), PARAM(6));
  case SYSCALL_MOVECURSOR:
    kMoveCursor(PARAM(0), PARAM(1));
    return TRUE;
  case SYSCALL_GETCURSORPOSITION:
    kGetCursorPosition((int*)PARAM(0), (int*)PARAM(1));
    return TRUE;
  case SYSCALL_BITBLT:
    return kBitBlt(PARAM(0), PARAM(1), PARAM(2), (COLOR*)PARAM(3),
      PARAM(4), PARAM(5));

  // JPEG 
  case SYSCALL_JPEGINIT:
    return kJPEGInit((JPEG*)PARAM(0), (char*)PARAM(1), PARAM(2));
  case SYSCALL_JPEGDECODE:
    return kJPEGDecode((JPEG*)PARAM(0), (COLOR*)PARAM(1));

  // RTC 
  case SYSCALL_READRTCTIME:
    kReadRTCTime((BYTE*)PARAM(0), (BYTE*)PARAM(1),
      (BYTE*)PARAM(2));
    return TRUE;
  case SYSCALL_READRTCDATE:
    kReadRTCDate((WORD*)PARAM(0), (BYTE*)PARAM(1),
      (BYTE*)PARAM(2), (BYTE*)PARAM(3));
    return TRUE;

  // Serial Port I/O
  case SYSCALL_SENDSERIALDATA:
    return kSendSerialData((BYTE*)PARAM(0), PARAM(1));
  case SYSCALL_RECEIVESERIALDATA:
    return kReceiveSerialData((BYTE*)PARAM(0), PARAM(1));
  case SYSCALL_CLEARSERIALFIFO:
    kClearSerialFIFO();
    return TRUE;

  // ETC
  case SYSCALL_GETTOTALRAMSIZE:
    return kGetTotalRAMSize();
  case SYSCALL_GETTICKCOUNT:
    return kGetTickCount();
  case SYSCALL_SLEEP:
    kSleep(PARAM(0));
    return TRUE;
  case SYSCALL_ISGRAPHICMODE:
    return kIsGraphicMode();
  case SYSCALL_TEST:
    kPrintf("Test System Call... System Call Test Success~!!\n");
    return TRUE;

  // Net
  case SYSCALL_TCPOPEN:
    return (QWORD)kTCP_Open(PARAM(0), PARAM(1), PARAM(2));
  case SYSCALL_TCPSEND:
    return kTCP_Send((TCP_TCB*)PARAM(0), (BYTE*)PARAM(1), PARAM(2), PARAM(3));
  case SYSCALL_TCPRECV:
    return kTCP_Recv((TCP_TCB*)PARAM(0), (BYTE*)PARAM(1), PARAM(2), PARAM(3));
  case SYSCALL_TCPCLOSE:
    return kTCP_Close((TCP_TCB*)PARAM(0));
  case SYSCALL_TCPSTATUS:
    return kTCP_Status((TCP_TCB*)PARAM(0));
  case SYSCALL_DNSGET:
    return kDNS_GetAddress((const BYTE*)PARAM(0), (BYTE*)PARAM(1));

  default:
    kPrintf("Undefined System Call~!!!, Service Number: %d\n",
      qwServiceNumber);
    return FALSE;
  }
}