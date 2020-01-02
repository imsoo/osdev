#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"

BYTE kInPortByte(WORD wPort);
void kOutPortByte(WORD wPort, BYTE bData);

#endif /* __ASSEMBLYUTILITY_H__ */