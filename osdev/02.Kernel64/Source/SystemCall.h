#ifndef __SYSTEMCALL_H__
#define __SYSTEMCALL_H__

#include "Types.h"

#define SYSTEMCALL_MAXPARAMETERCOUNT  20
#define PARAM( x )   ( pstParameter->vqwValue[ ( x ) ] )

#pragma pack(push, 1)

typedef struct kSystemCallParameterTableStruct
{
  QWORD vqwValue[SYSTEMCALL_MAXPARAMETERCOUNT];
} PARAMETERTABLE;

#pragma pack(pop)

// function
void kInitializeSystemCall(void);

void kSystemCallEntryPoint(QWORD qwServiceNumber, PARAMETERTABLE* pstParameter);
QWORD kProcessSystemCall(QWORD qwServiceNumber, PARAMETERTABLE* pstParameter);

void kSystemCallTestTask(void);

#endif // !__SYSTEMCALL_H__
