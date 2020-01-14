#ifndef __2DGRAPHICS_H__
#define __2DGRAPHICS_H__

#include "Types.h"

// 16Bit Color
typedef WORD  COLOR;

#define RGB(R, G, B) ( (((BYTE)(R) >> 3) << 11) | (((BYTE)(G) >> 2) << 5) | ((BYTE)(B) >> 3) )

// function
inline void kDrawPixel(int iX, int iY, COLOR stColor);
void kDrawLine(int iX1, int iY1, int iX2, int iY2, COLOR stColor);
void kDrawRect(int iX1, int iY1, int iX2, int iY2, COLOR stColor, BOOL bFill);
void kDrawCircle(int iX, int iY, int iRadius, COLOR stColor, BOOL bFill);
void kDrawText(int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor, const char* pcString, int iLength);

#endif // !__2DGRAPHICS_H__
