#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include "Types.h"
#include <stdarg.h>

// function

// STDIO
void memset( void* pvDestination, BYTE bData, int iSize );
int memcpy( void* pvDestination, const void* pvSource, int iSize );
int memcmp( const void* pvDestination, const void* pvSource, int iSize );
int strcpy( char* pcDestination, const char* pcSource );
int strcmp( char* pcDestination, const char* pcSource );
int strlen( const char* pcBuffer );
long atoi( const char* pcBuffer, int iRadix );
QWORD HexStringToQword( const char* pcBuffer );
long DecimalStringToLong( const char* pcBuffer );
int itoa( long lValue, char* pcBuffer, int iRadix );
int HexToString( QWORD qwValue, char* pcBuffer );
int DecimalToString( long lValue, char* pcBuffer );
void ReverseString( char* pcBuffer );
int sprintf( char* pcBuffer, const char* pcFormatString, ... );
int vsprintf( char* pcBuffer, const char* pcFormatString, va_list ap );
void printf( const char* pcFormatString, ... );
void srand( QWORD qwSeed );
QWORD rand( void );

// GUI
BOOL IsInRectangle( const RECT* pstArea, int iX, int iY );
int GetRectangleWidth( const RECT* pstArea );
int GetRectangleHeight( const RECT* pstArea );
BOOL GetOverlappedRectangle( const RECT* pstArea1, const RECT* pstArea2,
        RECT* pstIntersection  );
BOOL ConvertPointScreenToClient( QWORD qwWindowID, const POINT* pstXY, 
        POINT* pstXYInWindow );
BOOL ConvertPointClientToScreen( QWORD qwWindowID, const POINT* pstXY, 
        POINT* pstXYInScreen );
BOOL ConvertRectScreenToClient( QWORD qwWindowID, const RECT* pstArea, 
        RECT* pstAreaInWindow );
BOOL ConvertRectClientToScreen( QWORD qwWindowID, const RECT* pstArea, 
        RECT* pstAreaInScreen );
void SetRectangleData( int iX1, int iY1, int iX2, int iY2, RECT* pstRect );
BOOL SetMouseEvent( QWORD qwWindowID, QWORD qwEventType, int iMouseX, int iMouseY, 
        BYTE bButtonStatus, EVENT* pstEvent );
BOOL SetWindowEvent( QWORD qwWindowID, QWORD qwEventType, EVENT* pstEvent );
void SetKeyEvent( QWORD qwWindow, const KEYDATA* pstKeyData, EVENT* pstEvent );

#endif /*__LIBRARY_H__*/
