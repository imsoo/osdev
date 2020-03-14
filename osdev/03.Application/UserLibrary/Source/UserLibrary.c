#include "OSLibrary.h"

// ctype
int
isalnum(int c)
{
  return (isalpha(c) || isdigit(c));
}

int
isalpha(int c)
{
  return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int
isdigit(int c)
{
  return (c >= '0' && c <= '9');
}

int 
tolower(int c)
{
  if (c >= 'A' && c <= 'Z')
    return c + 'a' - 'A';
  else
    return c;
}

/* https://github.com/open-power/skiboot/blob/master/libc/ctype/isspace.c */
int 
isspace(int ch)
{
  switch (ch) {
  case ' ':
  case '\f':
  case '\n':
  case '\r':
  case '\t':
  case '\v':
    return 1;

  default:
    return 0;
  }
}

int fgetc(FILE * f)
{
  unsigned char ch;

  return (fread(&ch, 1, 1, f) == 1) ? (int)ch : EOF;
}

char *fgets(char *s, int n, FILE *f)
{
  int ch;
  char *p = s;

  while (n > 1) {
    ch = fgetc(f);
    if (ch == EOF) {
      *p = '\0';
      if (p == s) return NULL;
      else return s;
    }
    *p++ = ch;
    n--;
    if (ch == '\n')
      break;
  }
  if (n)
    *p = '\0';

  return s;
}


//
void memset( void* pvDestination, BYTE bData, int iSize )
{
    int i;
    QWORD qwData;
    int iRemainByteStartOffset;

    qwData = 0;
    for( i = 0 ; i < 8 ; i++ )
    {
        qwData = ( qwData << 8 ) | bData;
    }
    
    for( i = 0 ; i < ( iSize / 8 ) ; i++ )
    {
        ( ( QWORD* ) pvDestination )[ i ] = qwData;
    }
    
    iRemainByteStartOffset = i * 8;
    for( i = 0 ; i < ( iSize % 8 ) ; i++ )
    {
        ( ( char* ) pvDestination )[ iRemainByteStartOffset++ ] = bData;
    }
}

int memcpy( void* pvDestination, const void* pvSource, int iSize )
{
    int i;
    int iRemainByteStartOffset;
    
    for( i = 0 ; i < ( iSize / 8 ) ; i++ )
    {
        ( ( QWORD* ) pvDestination )[ i ] = ( ( QWORD* ) pvSource )[ i ];
    }
    
    iRemainByteStartOffset = i * 8;
    for( i = 0 ; i < ( iSize % 8 ) ; i++ )
    {
        ( ( char* ) pvDestination )[ iRemainByteStartOffset ] = 
            ( ( char* ) pvSource )[ iRemainByteStartOffset ];
        iRemainByteStartOffset++;
    }
    return iSize;
}

int memcmp( const void* pvDestination, const void* pvSource, int iSize )
{
    int i, j;
    int iRemainByteStartOffset;
    QWORD qwValue;
    char cValue;
    
    for( i = 0 ; i < ( iSize / 8 ) ; i++ )
    {
        qwValue = ( ( QWORD* ) pvDestination )[ i ] - ( ( QWORD* ) pvSource )[ i ];

        if( qwValue != 0 )
        {
            for( i = 0 ; i < 8 ; i++ )
            {
                if( ( ( qwValue >> ( i * 8 ) ) & 0xFF ) != 0 )
                {
                    return ( qwValue >> ( i * 8 ) ) & 0xFF;
                }
            }
        }
    }
    
    iRemainByteStartOffset = i * 8;
    for( i = 0 ; i < ( iSize % 8 ) ; i++ )
    {
        cValue = ( ( char* ) pvDestination )[ iRemainByteStartOffset ] -
            ( ( char* ) pvSource )[ iRemainByteStartOffset ];
        if( cValue != 0 )
        {
            return cValue;
        }
        iRemainByteStartOffset++;
    }    
    return 0;
}

int strcpy( char* pcDestination, const char* pcSource )
{
    int i;
    
    for( i = 0 ; pcSource[ i ] != 0 ; i++ )
    {
        pcDestination[ i ] = pcSource[ i ];
    }
    return i;
}

int strcmp( char* pcDestination, const char* pcSource )
{
    int i;
    
    for( i = 0 ; ( pcDestination[ i ] != 0 ) && ( pcSource[ i ] != 0 ) ; i++ )
    {
        if( ( pcDestination[ i ] - pcSource[ i ] ) != 0 )
        {
            break;
        }
    }
    
    return ( pcDestination[ i ] - pcSource[ i ] );
}

/* https://gist.github.com/reagent/3758387 */
char *
strdup(const char *s)
{
  size_t len = strlen(s) + 1;
  void *new_s = malloc(len);

  if (new_s == NULL)
    return NULL;
  memcpy(new_s, s, len);
  return (char *)new_s;
}

int strlen( const char* pcBuffer )
{
    int i;
    
    for( i = 0 ; ; i++ )
    {
        if( pcBuffer[ i ] == '\0' )
        {
            break;
        }
    }
    return i;
}

/* https://github.com/gcc-mirror/gcc/blob/master/libiberty/strchr.c */
char* strchr(const char* s, int c)
{
  do {
    if (*s == c)
    {
      return (char*)s;
    }
  } while (*s++);
  return (0);
}

long atoi( const char* pcBuffer, int iRadix )
{
    long lReturn;
    
    switch( iRadix )
    {
    case 16:
        lReturn = HexStringToQword( pcBuffer );
        break;
        
    case 10:
    default:
        lReturn = DecimalStringToLong( pcBuffer );
        break;
    }
    return lReturn;
}

/* https://github.com/GaloisInc/minlibc/blob/master/atof.c */
double atof(const char *s)
{
  // This function stolen from either Rolf Neugebauer or Andrew Tolmach. 
  // Probably Rolf.
  double a = 0.0;
  int e = 0;
  int c;
  while ((c = *s++) != '\0' && isdigit(c)) {
    a = a * 10.0 + (c - '0');
  }
  if (c == '.') {
    while ((c = *s++) != '\0' && isdigit(c)) {
      a = a * 10.0 + (c - '0');
      e = e - 1;
    }
  }
  if (c == 'e' || c == 'E') {
    int sign = 1;
    int i = 0;
    c = *s++;
    if (c == '+')
      c = *s++;
    else if (c == '-') {
      c = *s++;
      sign = -1;
    }
    while (isdigit(c)) {
      i = i * 10 + (c - '0');
      c = *s++;
    }
    e += i * sign;
  }
  while (e > 0) {
    a *= 10.0;
    e--;
  }
  while (e < 0) {
    a *= 0.1;
    e++;
  }
  return a;
}

void assert(int a)
{

}

QWORD HexStringToQword( const char* pcBuffer )
{
    QWORD qwValue = 0;
    int i;
    
    for( i = 0 ; pcBuffer[ i ] != '\0' ; i++ )
    {
        qwValue *= 16;
        if( ( 'A' <= pcBuffer[ i ] )  && ( pcBuffer[ i ] <= 'Z' ) )
        {
            qwValue += ( pcBuffer[ i ] - 'A' ) + 10;
        }
        else if( ( 'a' <= pcBuffer[ i ] )  && ( pcBuffer[ i ] <= 'z' ) )
        {
            qwValue += ( pcBuffer[ i ] - 'a' ) + 10;
        }
        else 
        {
            qwValue += pcBuffer[ i ] - '0';
        }
    }
    return qwValue;
}

long DecimalStringToLong( const char* pcBuffer )
{
    long lValue = 0;
    int i;
    
    if( pcBuffer[ 0 ] == '-' )
    {
        i = 1;
    }
    else
    {
        i = 0;
    }
    
    for( ; pcBuffer[ i ] != '\0' ; i++ )
    {
        lValue *= 10;
        lValue += pcBuffer[ i ] - '0';
    }
    
    if( pcBuffer[ 0 ] == '-' )
    {
        lValue = -lValue;
    }
    return lValue;
}

int itoa( long lValue, char* pcBuffer, int iRadix )
{
    int iReturn;
    
    switch( iRadix )
    {
    case 16:
        iReturn = HexToString( lValue, pcBuffer );
        break;
        
    case 10:
    default:
        iReturn = DecimalToString( lValue, pcBuffer );
        break;
    }
    
    return iReturn;
}

int HexToString( QWORD qwValue, char* pcBuffer )
{
    QWORD i;
    QWORD qwCurrentValue;

    if( qwValue == 0 )
    {
        pcBuffer[ 0 ] = '0';
        pcBuffer[ 1 ] = '\0';
        return 1;
    }
    
    for( i = 0 ; qwValue > 0 ; i++ )
    {
        qwCurrentValue = qwValue % 16;
        if( qwCurrentValue >= 10 )
        {
            pcBuffer[ i ] = 'A' + ( qwCurrentValue - 10 );
        }
        else
        {
            pcBuffer[ i ] = '0' + qwCurrentValue;
        }
        
        qwValue = qwValue / 16;
    }
    pcBuffer[ i ] = '\0';
    
    ReverseString( pcBuffer );
    return i;
}

int DecimalToString( long lValue, char* pcBuffer )
{
    long i;

    if( lValue == 0 )
    {
        pcBuffer[ 0 ] = '0';
        pcBuffer[ 1 ] = '\0';
        return 1;
    }
    
    if( lValue < 0 )
    {
        i = 1;
        pcBuffer[ 0 ] = '-';
        lValue = -lValue;
    }
    else
    {
        i = 0;
    }

    for( ; lValue > 0 ; i++ )
    {
        pcBuffer[ i ] = '0' + lValue % 10;        
        lValue = lValue / 10;
    }
    pcBuffer[ i ] = '\0';
    
    if( pcBuffer[ 0 ] == '-' )
    {
        ReverseString( &( pcBuffer[ 1 ] ) );
    }
    else
    {
        ReverseString( pcBuffer );
    }
    
    return i;
}

void ReverseString( char* pcBuffer )
{
   int iLength;
   int i;
   char cTemp;
   
   
   iLength = strlen( pcBuffer );
   for( i = 0 ; i < iLength / 2 ; i++ )
   {
       cTemp = pcBuffer[ i ];
       pcBuffer[ i ] = pcBuffer[ iLength - 1 - i ];
       pcBuffer[ iLength - 1 - i ] = cTemp;
   }
}

int sprintf( char* pcBuffer, const char* pcFormatString, ... )
{
    va_list ap;
    int iReturn;
    
    va_start( ap, pcFormatString );
    iReturn = vsprintf( pcBuffer, pcFormatString, ap );
    va_end( ap );
    
    return iReturn;
}

int vsprintf( char* pcBuffer, const char* pcFormatString, va_list ap )
{
    QWORD i, j, k;
    int iBufferIndex = 0;
    int iFormatLength, iCopyLength;
    char* pcCopyString;
    QWORD qwValue;
    int iValue;
    double dValue;
    
    iFormatLength = strlen( pcFormatString );
    for( i = 0 ; i < iFormatLength ; i++ ) 
    {
        if( pcFormatString[ i ] == '%' ) 
        {
            i++;
            switch( pcFormatString[ i ] ) 
            {
            case 's':
                pcCopyString = ( char* ) ( va_arg(ap, char* ));
                iCopyLength = strlen( pcCopyString );
                memcpy( pcBuffer + iBufferIndex, pcCopyString, iCopyLength );
                iBufferIndex += iCopyLength;
                break;
                
            case 'c':
                pcBuffer[ iBufferIndex ] = ( char ) ( va_arg( ap, int ) );
                iBufferIndex++;
                break;

            case 'd':
            case 'i':
                iValue = ( int ) ( va_arg( ap, int ) );
                iBufferIndex += itoa( iValue, pcBuffer + iBufferIndex, 10 );
                break;
                
            case 'x':
            case 'X':
                qwValue = ( DWORD ) ( va_arg( ap, DWORD ) ) & 0xFFFFFFFF;
                iBufferIndex += itoa( qwValue, pcBuffer + iBufferIndex, 16 );
                break;

            case 'q':
            case 'Q':
            case 'p':
                qwValue = ( QWORD ) ( va_arg( ap, QWORD ) );
                iBufferIndex += itoa( qwValue, pcBuffer + iBufferIndex, 16 );
                break;
            
            case 'f':
                dValue = ( double) ( va_arg( ap, double ) );
                dValue += 0.005;
                pcBuffer[ iBufferIndex ] = '0' + ( QWORD ) ( dValue * 100 ) % 10;
                pcBuffer[ iBufferIndex + 1 ] = '0' + ( QWORD ) ( dValue * 10 ) % 10;
                pcBuffer[ iBufferIndex + 2 ] = '.';
                for( k = 0 ; ; k++ )
                {
                    if( ( ( QWORD ) dValue == 0 ) && ( k != 0 ) )
                    {
                        break;
                    }
                    pcBuffer[ iBufferIndex + 3 + k ] = '0' + ( ( QWORD ) dValue % 10 );
                    dValue = dValue / 10;
                }
                pcBuffer[ iBufferIndex + 3 + k ] = '\0';
                ReverseString( pcBuffer + iBufferIndex );
                iBufferIndex += 3 + k;
                break;
                
            default:
                pcBuffer[ iBufferIndex ] = pcFormatString[ i ];
                iBufferIndex++;
                break;
            }
        } 
        else
        {
            pcBuffer[ iBufferIndex ] = pcFormatString[ i ];
            iBufferIndex++;
        }
    }
    
    pcBuffer[ iBufferIndex ] = '\0';
    return iBufferIndex;
}

void printf( const char* pcFormatString, ... )
{
    va_list ap;
    char vcBuffer[ 1024 ];
    int iNextPrintOffset;

    va_start( ap, pcFormatString );
    vsprintf( vcBuffer, pcFormatString, ap );
    va_end( ap );
    
    iNextPrintOffset = ConsolePrintString( vcBuffer );
    
    SetCursor( iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH );
}

void fprintf(int type, const char* pcFormatString, ...)
{
  va_list ap;
  char vcBuffer[1024];
  int iNextPrintOffset;

  va_start(ap, pcFormatString);
  vsprintf(vcBuffer, pcFormatString, ap);
  va_end(ap);

  iNextPrintOffset = ConsolePrintString(vcBuffer);

  SetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}

static volatile QWORD gs_qwRandomValue = 0;

void srand( QWORD qwSeed )
{
    gs_qwRandomValue = qwSeed;
}

QWORD rand( void )
{
    gs_qwRandomValue = ( gs_qwRandomValue * 412153 + 5571031 ) >> 16;
    return gs_qwRandomValue;
}

BOOL IsInRectangle( const RECT* pstArea, int iX, int iY )
{
    if( ( iX <  pstArea->iX1 ) || ( pstArea->iX2 < iX ) ||
        ( iY <  pstArea->iY1 ) || ( pstArea->iY2 < iY ) )
    {
        return FALSE;
    }
    
    return TRUE;
}

int GetRectangleWidth( const RECT* pstArea )
{
    int iWidth;
    
    iWidth = pstArea->iX2 - pstArea->iX1 + 1;
    
    if( iWidth < 0 )
    {
        return -iWidth;
    }
    
    return iWidth;
}

int GetRectangleHeight( const RECT* pstArea )
{
    int iHeight;
    
    iHeight = pstArea->iY2 - pstArea->iY1 + 1;
    
    if( iHeight < 0 )
    {
        return -iHeight;
    }
    
    return iHeight;
}

BOOL GetOverlappedRectangle( const RECT* pstArea1, const RECT* pstArea2,
        RECT* pstIntersection  )
{
    int iMaxX1;
    int iMinX2;
    int iMaxY1;
    int iMinY2;

    iMaxX1 = MAX( pstArea1->iX1, pstArea2->iX1 );
    iMinX2 = MIN( pstArea1->iX2, pstArea2->iX2 );
    if( iMinX2 < iMaxX1 )
    {
        return FALSE;
    }

    iMaxY1 = MAX( pstArea1->iY1, pstArea2->iY1 );
    iMinY2 = MIN( pstArea1->iY2, pstArea2->iY2 );
    if( iMinY2 < iMaxY1 )
    {
        return FALSE;
    }

    pstIntersection->iX1 = iMaxX1;
    pstIntersection->iY1 = iMaxY1;
    pstIntersection->iX2 = iMinX2;
    pstIntersection->iY2 = iMinY2;

    return TRUE;
}

BOOL ConvertPointScreenToClient( QWORD qwWindowID, const POINT* pstXY, 
        POINT* pstXYInWindow )
{
    RECT stArea;
    
    if( GetWindowArea( qwWindowID, &stArea ) == FALSE )
    {
        return FALSE;
    }
    
    pstXYInWindow->iX = pstXY->iX - stArea.iX1;
    pstXYInWindow->iY = pstXY->iY - stArea.iY1;
    return TRUE;
}

BOOL ConvertPointClientToScreen( QWORD qwWindowID, const POINT* pstXY, 
        POINT* pstXYInScreen )
{
    RECT stArea;
    
    if( GetWindowArea( qwWindowID, &stArea ) == FALSE )
    {
        return FALSE;
    }
    
    pstXYInScreen->iX = pstXY->iX + stArea.iX1;
    pstXYInScreen->iY = pstXY->iY + stArea.iY1;
    return TRUE;
}

BOOL ConvertRectScreenToClient( QWORD qwWindowID, const RECT* pstArea, 
        RECT* pstAreaInWindow )
{
    RECT stWindowArea;
    
    if( GetWindowArea( qwWindowID, &stWindowArea ) == FALSE )
    {
        return FALSE;
    }
    
    pstAreaInWindow->iX1 = pstArea->iX1 - stWindowArea.iX1;
    pstAreaInWindow->iY1 = pstArea->iY1 - stWindowArea.iY1;
    pstAreaInWindow->iX2 = pstArea->iX2 - stWindowArea.iX1;
    pstAreaInWindow->iY2 = pstArea->iY2 - stWindowArea.iY1;
    return TRUE;
}

BOOL ConvertRectClientToScreen( QWORD qwWindowID, const RECT* pstArea, 
        RECT* pstAreaInScreen )
{
    RECT stWindowArea;
    
    if( GetWindowArea( qwWindowID, &stWindowArea ) == FALSE )
    {
        return FALSE;
    }
    
    pstAreaInScreen->iX1 = pstArea->iX1 + stWindowArea.iX1;
    pstAreaInScreen->iY1 = pstArea->iY1 + stWindowArea.iY1;
    pstAreaInScreen->iX2 = pstArea->iX2 + stWindowArea.iX1;
    pstAreaInScreen->iY2 = pstArea->iY2 + stWindowArea.iY1;
    return TRUE;
}

void SetRectangleData( int iX1, int iY1, int iX2, int iY2, RECT* pstRect )
{
    if( iX1 < iX2 )
    {
        pstRect->iX1 = iX1;
        pstRect->iX2 = iX2;
    }
    else
    {
        pstRect->iX1 = iX2;
        pstRect->iX2 = iX1;
    }
    
    if( iY1 < iY2 )
    {
        pstRect->iY1 = iY1;
        pstRect->iY2 = iY2;
    }
    else
    {
        pstRect->iY1 = iY2;
        pstRect->iY2 = iY1;
    }
}

BOOL SetMouseEvent( QWORD qwWindowID, QWORD qwEventType, int iMouseX, int iMouseY, 
        BYTE bButtonStatus, EVENT* pstEvent )
{
    POINT stMouseXYInWindow;
    POINT stMouseXY;
    
    switch( qwEventType )
    {
    case EVENT_MOUSE_MOVE:
    case EVENT_MOUSE_LBUTTONDOWN:
    case EVENT_MOUSE_LBUTTONUP:            
    case EVENT_MOUSE_RBUTTONDOWN:
    case EVENT_MOUSE_RBUTTONUP:
    case EVENT_MOUSE_MBUTTONDOWN:
    case EVENT_MOUSE_MBUTTONUP:
        stMouseXY.iX = iMouseX;
        stMouseXY.iY = iMouseY;
        
        if( ConvertPointScreenToClient( qwWindowID, &stMouseXY, &stMouseXYInWindow ) 
                == FALSE )
        {
            return FALSE;
        }

        pstEvent->qwType = qwEventType;
        pstEvent->stMouseEvent.qwWindowID = qwWindowID;    
        pstEvent->stMouseEvent.bButtonStatus = bButtonStatus;
        memcpy( &( pstEvent->stMouseEvent.stPoint ), &stMouseXYInWindow, 
                sizeof( POINT ) );
        break;
        
    default:
        return FALSE;
        break;
    }    
    return TRUE;
}

BOOL SetWindowEvent( QWORD qwWindowID, QWORD qwEventType, EVENT* pstEvent )
{
    RECT stArea;
    
    switch( qwEventType )
    {
    case EVENT_WINDOW_SELECT:
    case EVENT_WINDOW_DESELECT:
    case EVENT_WINDOW_MOVE:
    case EVENT_WINDOW_RESIZE:
    case EVENT_WINDOW_CLOSE:
        pstEvent->qwType = qwEventType;
        pstEvent->stWindowEvent.qwWindowID = qwWindowID;
        if( GetWindowArea( qwWindowID, &stArea ) == FALSE )
        {
            return FALSE;
        }
        
        memcpy( &( pstEvent->stWindowEvent.stArea ), &stArea, sizeof( RECT ) );
        break;
        
    default:
        return FALSE;
        break;
    }    
    return TRUE;
}

void SetKeyEvent( QWORD qwWindow, const KEYDATA* pstKeyData, EVENT* pstEvent )
{
    if( pstKeyData->bFlags & KEY_FLAGS_DOWN )
    {
        pstEvent->qwType = EVENT_KEY_DOWN;
    }
    else
    {
        pstEvent->qwType = EVENT_KEY_UP;
    }
    
    pstEvent->stKeyEvent.bASCIICode = pstKeyData->bASCIICode;
    pstEvent->stKeyEvent.bScanCode = pstKeyData->bScanCode;
    pstEvent->stKeyEvent.bFlags = pstKeyData->bFlags;
}


/* Copyright (C) 1991-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Douglas C. Schmidt (schmidt@ics.uci.edu).

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

   /* If you consider tuning this algorithm, you should consult first:
      Engineering a sort function; Jon Bentley and M. Douglas McIlroy;
      Software - Practice and Experience; Vol. 23 (11), 1249-1265, 1993.  */

      /* Byte-wise swap two items of size SIZE. */
#define SWAP(a, b, size)\
  do\
    {\
      size_t __size = (size);\
      char *__a = (a), *__b = (b);\
      do\
	      {\
	        char __tmp = *__a;\
	        *__a++ = *__b;\
	        *__b++ = __tmp;\
	      } while (--__size > 0);\
    } while (0)

/* Discontinue quicksort algorithm when partition gets below this size.
   This particular magic number was chosen to work best on a Sun 4/260. */
#define MAX_THRESH 4

   /* Stack node declarations used to store unfulfilled partition obligations. */
typedef struct
{
  char *lo;
  char *hi;
} stack_node;

/* The next 4 #defines implement a very fast in-line stack abstraction. */
/* The stack needs log (total_elements) entries (we could even subtract
   log(MAX_THRESH)).  Since total_elements has type size_t, we get as
   upper bound for log (total_elements):
   bits per byte (CHAR_BIT) * sizeof(size_t).  */
#define CHAR_BIT	8
#define STACK_SIZE	(CHAR_BIT * sizeof(size_t))
#define PUSH(low, high)	((void) ((top->lo = (low)), (top->hi = (high)), ++top))
#define	POP(low, high)	((void) (--top, (low = top->lo), (high = top->hi)))
#define	STACK_NOT_EMPTY	(stack < top)

   /* Order size using quicksort.  This implementation incorporates
      four optimizations discussed in Sedgewick:

      1. Non-recursive, using an explicit stack of pointer that store the
         next array partition to sort.  To save time, this maximum amount
         of space required to store an array of SIZE_MAX is allocated on the
         stack.  Assuming a 32-bit (64 bit) integer for size_t, this needs
         only 32 * sizeof(stack_node) == 256 bytes (for 64 bit: 1024 bytes).
         Pretty cheap, actually.

      2. Chose the pivot element using a median-of-three decision tree.
         This reduces the probability of selecting a bad pivot value and
         eliminates certain extraneous comparisons.

      3. Only quicksorts TOTAL_ELEMS / MAX_THRESH partitions, leaving
         insertion sort to order the MAX_THRESH items within each partition.
         This is a big win, since insertion sort is faster for small, mostly
         sorted array segments.

      4. The larger of the two sub-partitions is always pushed onto the
         stack first, with the algorithm then concentrating on the
         smaller partition.  This *guarantees* no more than log (total_elems)
         stack size is needed (actually O(1) in this case)!  */
typedef int(*__compar_d_fn_t) (const void *, const void *);
void
qsort(void *const pbase, size_t total_elems, size_t size,
  __compar_d_fn_t cmp)
{
  char *base_ptr = (char *)pbase;

  const size_t max_thresh = MAX_THRESH * size;

  if (total_elems == 0)
    /* Avoid lossage with unsigned arithmetic below.  */
    return;

  if (total_elems > MAX_THRESH)
  {
    char *lo = base_ptr;
    char *hi = &lo[size * (total_elems - 1)];
    stack_node stack[STACK_SIZE];
    stack_node *top = stack;

    PUSH(NULL, NULL);

    while (STACK_NOT_EMPTY)
    {
      char *left_ptr;
      char *right_ptr;

      /* Select median value from among LO, MID, and HI. Rearrange
         LO and HI so the three values are sorted. This lowers the
         probability of picking a pathological pivot value and
         skips a comparison for both the LEFT_PTR and RIGHT_PTR in
         the while loops. */

      char *mid = lo + size * ((hi - lo) / size >> 1);

      if ((*cmp) ((void *)mid, (void *)lo) < 0)
        SWAP(mid, lo, size);
      if ((*cmp) ((void *)hi, (void *)mid) < 0)
        SWAP(mid, hi, size);
      else
        goto jump_over;
      if ((*cmp) ((void *)mid, (void *)lo) < 0)
        SWAP(mid, lo, size);
    jump_over:;

      left_ptr = lo + size;
      right_ptr = hi - size;

      /* Here's the famous ``collapse the walls'' section of quicksort.
         Gotta like those tight inner loops!  They are the main reason
         that this algorithm runs much faster than others. */
      do
      {
        while ((*cmp) ((void *)left_ptr, (void *)mid) < 0)
          left_ptr += size;

        while ((*cmp) ((void *)mid, (void *)right_ptr) < 0)
          right_ptr -= size;

        if (left_ptr < right_ptr)
        {
          SWAP(left_ptr, right_ptr, size);
          if (mid == left_ptr)
            mid = right_ptr;
          else if (mid == right_ptr)
            mid = left_ptr;
          left_ptr += size;
          right_ptr -= size;
        }
        else if (left_ptr == right_ptr)
        {
          left_ptr += size;
          right_ptr -= size;
          break;
        }
      } while (left_ptr <= right_ptr);

      /* Set up pointers for next iteration.  First determine whether
         left and right partitions are below the threshold size.  If so,
         ignore one or both.  Otherwise, push the larger partition's
         bounds on the stack and continue sorting the smaller one. */

      if ((size_t)(right_ptr - lo) <= max_thresh)
      {
        if ((size_t)(hi - left_ptr) <= max_thresh)
          /* Ignore both small partitions. */
          POP(lo, hi);
        else
          /* Ignore small left partition. */
          lo = left_ptr;
      }
      else if ((size_t)(hi - left_ptr) <= max_thresh)
        /* Ignore small right partition. */
        hi = right_ptr;
      else if ((right_ptr - lo) > (hi - left_ptr))
      {
        /* Push larger left partition indices. */
        PUSH(lo, right_ptr);
        lo = left_ptr;
      }
      else
      {
        /* Push larger right partition indices. */
        PUSH(left_ptr, hi);
        hi = right_ptr;
      }
    }
  }

  /* Once the BASE_PTR array is partially sorted by quicksort the rest
     is completely sorted using insertion sort, since this is efficient
     for partitions below MAX_THRESH size. BASE_PTR points to the beginning
     of the array to sort, and END_PTR points at the very last element in
     the array (*not* one beyond it!). */

#define min(x, y) ((x) < (y) ? (x) : (y))

  {
    char *const end_ptr = &base_ptr[size * (total_elems - 1)];
    char *tmp_ptr = base_ptr;
    char *thresh = min(end_ptr, base_ptr + max_thresh);
    char *run_ptr;

    /* Find smallest element in first threshold and place it at the
       array's beginning.  This is the smallest array element,
       and the operation speeds up insertion sort's inner loop. */

    for (run_ptr = tmp_ptr + size; run_ptr <= thresh; run_ptr += size)
      if ((*cmp) ((void *)run_ptr, (void *)tmp_ptr) < 0)
        tmp_ptr = run_ptr;

    if (tmp_ptr != base_ptr)
      SWAP(tmp_ptr, base_ptr, size);

    /* Insertion sort, running from left-hand-side up to right-hand-side.  */

    run_ptr = base_ptr + size;
    while ((run_ptr += size) <= end_ptr)
    {
      tmp_ptr = run_ptr - size;
      while ((*cmp) ((void *)run_ptr, (void *)tmp_ptr) < 0)
        tmp_ptr -= size;

      tmp_ptr += size;
      if (tmp_ptr != run_ptr)
      {
        char *trav;

        trav = run_ptr + size;
        while (--trav >= run_ptr)
        {
          char c = *trav;
          char *hi, *lo;

          for (hi = lo = trav; (lo -= size) >= tmp_ptr; hi = lo)
            *hi = *lo;
          *hi = c;
        }
      }
    }
  }
}