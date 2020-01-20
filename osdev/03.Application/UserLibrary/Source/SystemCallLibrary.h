#ifndef __SYSTEMCALLLIBRARY_H__
#define __SYSTEMCALLLIBRARY_H__

#include "Types.h"
#include "SystemCallList.h"

#define SYSTEMCALL_MAXPARAMETERCOUNT    20
#define PARAM( x )   ( stParameter.vqwValue[ ( x ) ] )

#pragma pack( push, 1 )

typedef struct kSystemCallParameterTableStruct
{
    QWORD vqwValue[ SYSTEMCALL_MAXPARAMETERCOUNT ];
} PARAMETERTABLE;

#pragma pack( pop )

// function
QWORD ExecuteSystemCall( QWORD qwServiceNumber, PARAMETERTABLE* pstParameter );

// Console I/O
int ConsolePrintString( const char* pcBuffer );
BOOL SetCursor( int iX, int iY );
BOOL GetCursor( int *piX, int *piY );
BOOL ClearScreen( void );
BYTE getch( void );

// Dynamic Memory
void* malloc( QWORD qwSize );
BOOL free( void* pvAddress );

// File I/O
FILE* fopen( const char* pcFileName, const char* pcMode );
DWORD fread( void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile );
DWORD fwrite( const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile );
int fseek( FILE* pstFile, int iOffset, int iOrigin );
int fclose( FILE* pstFile );
int remove( const char* pcFileName );
DIR* opendir( const char* pcDirectoryName );
struct dirent* readdir( DIR* pstDirectory );
BOOL rewinddir( DIR* pstDirectory );
int closedir( DIR* pstDirectory );
BOOL IsFileOpened( const struct dirent* pstEntry );

// HDD I/O
int ReadHDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, 
        char* pcBuffer );
int WriteHDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, 
        char* pcBuffer );

// Task
QWORD CreateTask( QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, 
                  QWORD qwEntryPointAddress, BYTE bAffinity );
BOOL Schedule( void );
BOOL ChangePriority( QWORD qwID, BYTE bPriority, BOOL bExecutedInInterrupt );
BOOL EndTask( QWORD qwTaskID );
void exit( int iExitValue );
int GetTaskCount( BYTE bAPICID );
BOOL IsTaskExist( QWORD qwID );
QWORD GetProcessorLoad( BYTE bAPICID );
BOOL ChangeProcessorAffinity( QWORD qwTaskID, BYTE bAffinity );
QWORD ExecuteProgram(const char* pcFileName, const char* pcArgumentString,
  BYTE bAffinity);
QWORD CreateThread(QWORD qwEntryPoint, QWORD qwArgument, BYTE bAffinity);

// GUI
QWORD GetBackgroundWindowID( void );
void GetScreenArea( RECT* pstScreenArea );
QWORD CreateWindow( int iX, int iY, int iWidth, int iHeight, DWORD dwFlags,
        const char* pcTitle );
BOOL DeleteWindow( QWORD qwWindowID );
BOOL ShowWindow( QWORD qwWindowID, BOOL bShow );
QWORD FindWindowByPoint( int iX, int iY );
QWORD FindWindowByTitle( const char* pcTitle );
BOOL IsWindowExist( QWORD qwWindowID );
QWORD GetTopWindowID( void );
BOOL MoveWindowToTop( QWORD qwWindowID );
BOOL IsInTitleBar( QWORD qwWindowID, int iX, int iY );
BOOL IsInCloseButton( QWORD qwWindowID, int iX, int iY );
BOOL MoveWindow( QWORD qwWindowID, int iX, int iY );
BOOL ResizeWindow( QWORD qwWindowID, int iX, int iY, int iWidth, int iHeight );
BOOL GetWindowArea( QWORD qwWindowID, RECT* pstArea );
BOOL UpdateScreenByID( QWORD qwWindowID );
BOOL UpdateScreenByWindowArea( QWORD qwWindowID, const RECT* pstArea );
BOOL UpdateScreenByScreenArea( const RECT* pstArea );
BOOL SendEventToWindow( QWORD qwWindowID, const EVENT* pstEvent );
BOOL ReceiveEventFromWindowQueue( QWORD qwWindowID, EVENT* pstEvent );
BOOL DrawWindowFrame( QWORD qwWindowID );
BOOL DrawWindowBackground( QWORD qwWindowID );
BOOL DrawWindowTitle( QWORD qwWindowID, const char* pcTitle, BOOL bSelectedTitle );
BOOL DrawButton( QWORD qwWindowID, RECT* pstButtonArea, COLOR stBackgroundColor,
        const char* pcText, COLOR stTextColor );
BOOL DrawPixel( QWORD qwWindowID, int iX, int iY, COLOR stColor );
BOOL DrawLine( QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor );
BOOL DrawRect( QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2,
        COLOR stColor, BOOL bFill );
BOOL DrawCircle( QWORD qwWindowID, int iX, int iY, int iRadius, COLOR stColor,
        BOOL bFill );
BOOL DrawText( QWORD qwWindowID, int iX, int iY, COLOR stTextColor,
        COLOR stBackgroundColor, const char* pcString, int iLength );
void MoveCursor( int iX, int iY );
void GetCursorPosition( int* piX, int* piY );
BOOL BitBlt( QWORD qwWindowID, int iX, int iY, COLOR* pstBuffer, int iWidth, 
        int iHeight );

// JPEG
BOOL JPEGInit(JPEG *jpeg, BYTE* pbFileBuffer, DWORD dwFileSize);
BOOL JPEGDecode(JPEG *jpeg, COLOR* rgb);

// RTC
BOOL ReadRTCTime( BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond );
BOOL ReadRTCDate( WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, 
                  BYTE* pbDayOfWeek );

// Serial Port I/O
void SendSerialData( BYTE* pbBuffer, int iSize );
int ReceiveSerialData( BYTE* pbBuffer, int iSize );
void ClearSerialFIFO( void );

// ETC
QWORD GetTotalRAMSize( void );
QWORD GetTickCount( void );
void Sleep( QWORD qwMillisecond );

#endif /*__SYSTEMCALLLIBRARY_H__*/
