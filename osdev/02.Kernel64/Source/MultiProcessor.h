#ifndef __MULTIPROCESSOR_H__
#define __MULTIPROCESSOR_H__

#include "Types.h"

 // MultiProcessor Flag Address
#define BOOTSTRAPPROCESSOR_FLAGADDRESS      0x7C09
// Max Core Count
#define MAXPROCESSORCOUNT                   16

// function
BOOL kStartUpApplicationProcessor(void);
BYTE kGetAPICID(void);
static BOOL kWakeUpApplicationProcessor(void);

#endif /*__MULTIPROCESSOR_H__*/
