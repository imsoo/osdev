#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "Types.h"

void kMemSet(void* pvDestination, BYTE bData, int iSize);
int kMemCpy(void* pvDestination, const void* pvSource, int iSize);
int kMemCmp(const void* pvDestination, const void* pvSource, int iSize);

#endif /* __UTILITY_H__ */