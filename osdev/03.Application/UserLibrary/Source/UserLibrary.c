#include "Types.h"
#include "UserLibrary.h"
#include <stdarg.h>

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
