#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdarg.h>
#include "Types.h"

#define MIN( x, y )     ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )
#define MAX( x, y )     ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )

extern volatile QWORD g_qwTickCount;

QWORD kGetTickCount(void);
void kSleep(QWORD qwMillisecond);

void kMemSet(void* pvDestination, BYTE bData, int iSize);
inline void kMemSetWord(void* pvDestination, WORD wData, int iWordSize);
int kMemCpy(void* pvDestination, const void* pvSource, int iSize);
int kMemCmp(const void* pvDestination, const void* pvSource, int iSize);
BOOL kSetInterruptFlag(BOOL bEnableInterrupt);

void kCheckTotalRAMSize(void);
QWORD kGetTotalRAMSize(void);
void kReverseString(char* pcBuffer);
long kAToI(const char* pcBuffer, int iRadix);
QWORD kHexStringToQword(const char* pcBuffer);
long kDecimalStringToLong(const char* pcBuffer);
int kIToA(long lValue, char* pcBuffer, int iRadix);
int kHexToString(QWORD qwValue, char* pcBuffer);
int kDecimalToString(long lValue, char* pcBuffer);
int kSPrintf(char* pcBuffer, const char* pcFormatString, ...);
int kVSPrintf(char* pcBuffer, const char* pcFormatString, va_list ap);

#endif /* __UTILITY_H__ */