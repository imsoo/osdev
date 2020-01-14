#include "2DGraphics.h"
#include "VBE.h"
#include "Font.h"
#include "Utility.h"

/*
  Draw Pixel
*/
void kDrawPixel(int iX, int iY, COLOR stColor)
{
  VBEMODEINFOBLOCK* pstModeInfo;

  pstModeInfo = kGetVBEModeInfoBlock();

  *(((COLOR*)(QWORD)pstModeInfo->dwPhysicalBasePointer) +
    pstModeInfo->wXResolution * iY + iX) = stColor;
}

/*
  Draw Line Using Bresenham Algorithm
*/
void kDrawLine(int iX1, int iY1, int iX2, int iY2, COLOR stColor)
{
  int iDeltaX, iDeltaY;
  int iError = 0;
  int iDeltaError;
  int iX, iY;
  int iStepX, iStepY;

  // Calc Delta
  iDeltaX = iX2 - iX1;
  iDeltaY = iY2 - iY1;

  // Calc direction X
  if (iDeltaX < 0) {
    iDeltaX = -iDeltaX;
    iStepX = -1;
  }
  else {
    iStepX = 1;
  }

  // Calc direction Y
  if (iDeltaY < 0) {
    iDeltaY = -iDeltaY;
    iStepY = -1;
  }
  else {
    iStepY = 1;
  }

  // Draw Line 
  if (iDeltaX > iDeltaY)
  {
    // iDeltaY * 2
    iDeltaError = iDeltaY << 1;
    iY = iY1;
    for (iX = iX1; iX != iX2; iX += iStepX)
    {
      // Draw Pixel
      kDrawPixel(iX, iY, stColor);

      // Accumutate Error
      iError += iDeltaError;

      // if Error >= iDeltaX : moveY next step
      if (iError >= iDeltaX)
      {
        iY += iStepY;
        iError -= iDeltaX << 1;
      }
    }
    kDrawPixel(iX, iY, stColor);
  }
  else
  {
    // iDeltaX * 2
    iDeltaError = iDeltaX << 1;
    iX = iX1;
    for (iY = iY1; iY != iY2; iY += iStepY)
    {
      // Draw Pixel
      kDrawPixel(iX, iY, stColor);

      // Accumutate Error
      iError += iDeltaError;

      // if Error >= iDeltaY : moveX next step
      if (iError >= iDeltaY)
      {
        iX += iStepX;
        iError -= iDeltaY << 1;
      }
    }
    kDrawPixel(iX, iY, stColor);
  }
}

/*
  Draw Rect Using Draw Line
*/
void kDrawRect(int iX1, int iY1, int iX2, int iY2, COLOR stColor, BOOL bFill)
{
  int iWidth;
  int iTemp;
  int iY;
  int iStepY;
  COLOR* pstVideoMemoryAddress;
  VBEMODEINFOBLOCK* pstModeInfo;

  // only line
  if (bFill == FALSE)
  {
    kDrawLine(iX1, iY1, iX2, iY1, stColor);
    kDrawLine(iX1, iY1, iX1, iY2, stColor);
    kDrawLine(iX2, iY1, iX2, iY2, stColor);
    kDrawLine(iX1, iY2, iX2, iY2, stColor);
  }
  // fill
  else
  {
    // Get Video Memory Address
    pstModeInfo = kGetVBEModeInfoBlock();
    pstVideoMemoryAddress =
      (COLOR*)((QWORD)pstModeInfo->dwPhysicalBasePointer);

    // X1 has to less than X2
    if (iX2 < iX1)
    {
      iTemp = iX1;
      iX1 = iX2;
      iX2 = iTemp;

      iTemp = iY1;
      iY1 = iY2;
      iY2 = iTemp;
    }

    // Calc Width
    iWidth = iX2 - iX1 + 1;

    // Calc Y direction
    if (iY1 <= iY2)
    {
      iStepY = 1;
    }
    else
    {
      iStepY = -1;
    }

    // Calc Start Address
    pstVideoMemoryAddress += iY1 * pstModeInfo->wXResolution + iX1;

    // Fill Rect
    for (iY = iY1; iY != iY2; iY += iStepY)
    {
      kMemSetWord(pstVideoMemoryAddress, stColor, iWidth);

      if (iStepY >= 0)
      {
        pstVideoMemoryAddress += pstModeInfo->wXResolution;
      }
      else
      {
        pstVideoMemoryAddress -= pstModeInfo->wXResolution;
      }
    }
    kMemSetWord(pstVideoMemoryAddress, stColor, iWidth);
  }
}

/*
  Draw Circle using Midpoint Circle Algorithm
*/
void kDrawCircle(int iX, int iY, int iRadius, COLOR stColor, BOOL bFill)
{
  int iCircleX, iCircleY;
  int iDistance;

  if (iRadius < 0)
  {
    return;
  }

  iCircleY = iRadius;

  // Start 4 Point
  if (bFill == FALSE)
  {
    kDrawPixel(0 + iX, iRadius + iY, stColor);
    kDrawPixel(0 + iX, -iRadius + iY, stColor);
    kDrawPixel(iRadius + iX, 0 + iY, stColor);
    kDrawPixel(-iRadius + iX, 0 + iY, stColor);
  }
  else
  {
    kDrawLine(0 + iX, iRadius + iY, 0 + iX, -iRadius + iY, stColor);
    kDrawLine(iRadius + iX, 0 + iY, -iRadius + iX, 0 + iY, stColor);
  }

  iDistance = -iRadius;

  // draw circle
  for (iCircleX = 1; iCircleX <= iCircleY; iCircleX++)
  {
    iDistance += (iCircleX << 1) - 1;  //2 * iCircleX - 1;

    if (iDistance >= 0)
    {
      iCircleY--;
      iDistance += (-iCircleY << 1) + 2; //-2 * iCircleY + 2;
    }

    if (bFill == FALSE)
    {
      kDrawPixel(iCircleX + iX, iCircleY + iY, stColor);
      kDrawPixel(iCircleX + iX, -iCircleY + iY, stColor);
      kDrawPixel(-iCircleX + iX, iCircleY + iY, stColor);
      kDrawPixel(-iCircleX + iX, -iCircleY + iY, stColor);
      kDrawPixel(iCircleY + iX, iCircleX + iY, stColor);
      kDrawPixel(iCircleY + iX, -iCircleX + iY, stColor);
      kDrawPixel(-iCircleY + iX, iCircleX + iY, stColor);
      kDrawPixel(-iCircleY + iX, -iCircleX + iY, stColor);
    }
    else
    {
      kDrawRect(-iCircleX + iX, iCircleY + iY,
        iCircleX + iX, iCircleY + iY, stColor, TRUE);
      kDrawRect(-iCircleX + iX, -iCircleY + iY,
        iCircleX + iX, -iCircleY + iY, stColor, TRUE);
      kDrawRect(-iCircleY + iX, iCircleX + iY,
        iCircleY + iX, iCircleX + iY, stColor, TRUE);
      kDrawRect(-iCircleY + iX, -iCircleX + iY,
        iCircleY + iX, -iCircleX + iY, stColor, TRUE);
    }
  }
}

/*
  Draw Text
*/
void kDrawText(int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor,
  const char* pcString, int iLength)
{
  int iCurrentX, iCurrentY;
  int i, j, k;
  BYTE bBitmask;
  int iBitmaskStartIndex;
  VBEMODEINFOBLOCK* pstModeInfo;
  COLOR* pstVideoMemoryAddress;

  // Get Video Memory Address
  pstModeInfo = kGetVBEModeInfoBlock();
  pstVideoMemoryAddress =
    (COLOR*)((QWORD)pstModeInfo->dwPhysicalBasePointer);

  iCurrentX = iX;

  // Print String
  for (k = 0; k < iLength; k++)
  {
    // Get Y
    iCurrentY = iY * pstModeInfo->wXResolution;

    // Font Start index
    iBitmaskStartIndex = pcString[k] * FONT_ENGLISHHEIGHT;

    // Print Height 16
    for (j = 0; j < FONT_ENGLISHHEIGHT; j++)
    {
      bBitmask = g_vucEnglishFont[iBitmaskStartIndex++];

      // Print Width 8
      for (i = 0; i < FONT_ENGLISHWIDTH; i++)
      {
        if (bBitmask & (0x01 << (FONT_ENGLISHWIDTH - i - 1)))
        {
          pstVideoMemoryAddress[iCurrentY + iCurrentX + i] = stTextColor;
        }
        else
        {
          pstVideoMemoryAddress[iCurrentY + iCurrentX + i] = stBackgroundColor;
        }
      }
      iCurrentY += pstModeInfo->wXResolution;
    }
    iCurrentX += FONT_ENGLISHWIDTH;
  }
}
