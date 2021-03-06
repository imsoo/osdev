#ifndef __TYPES_H
#define __TYPES_H

#define BYTE  unsigned char
#define WORD  unsigned short
#define DWORD unsigned int
#define QWORD unsigned long
#define BOOL  unsigned char

#define TRUE  0
#define FALSE 1
#define NULL  0

#pragma pack(push, 1)

// for video print
typedef struct kCharactorStruct
{
  BYTE bCharactor;
  BYTE bAttrribute;
} CHARACTER;

#pragma pack(pop)

#endif /* __TYPES_H__ */