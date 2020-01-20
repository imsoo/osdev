#include "SystemCallLibrary.h"

int ConsolePrintString( const char* pcBuffer )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pcBuffer;
    return ExecuteSystemCall( SYSCALL_CONSOLEPRINTSTRING, &stParameter );
}

BOOL SetCursor( int iX, int iY )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) iX;
    PARAM( 1 ) = ( QWORD ) iY;
    return ExecuteSystemCall( SYSCALL_SETCURSOR, &stParameter );
}

BOOL GetCursor( int *piX, int *piY )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) piX;
    PARAM( 1 ) = ( QWORD ) piY;
    return ExecuteSystemCall( SYSCALL_GETCURSOR, &stParameter );
}

BOOL ClearScreen( void )
{
    return ExecuteSystemCall( SYSCALL_CLEARSCREEN, NULL );
}

BYTE getch( void )
{
    return ExecuteSystemCall( SYSCALL_GETCH, NULL );
}

void* malloc( QWORD qwSize )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwSize;
    return ( void* ) ExecuteSystemCall( SYSCALL_MALLOC, &stParameter );
}

BOOL free( void* pvAddress )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pvAddress;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_FREE, &stParameter );    
}

FILE* fopen( const char* pcFileName, const char* pcMode )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pcFileName;
    PARAM( 1 ) = ( QWORD ) pcMode;
    return ( FILE* ) ExecuteSystemCall( SYSCALL_FOPEN, &stParameter );      
}

DWORD fread( void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pvBuffer;
    PARAM( 1 ) = ( QWORD ) dwSize;
    PARAM( 2 ) = ( QWORD ) dwCount;
    PARAM( 3 ) = ( QWORD ) pstFile;
    return ExecuteSystemCall( SYSCALL_FREAD, &stParameter );      
}

DWORD fwrite( const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pvBuffer;
    PARAM( 1 ) = ( QWORD ) dwSize;
    PARAM( 2 ) = ( QWORD ) dwCount;
    PARAM( 3 ) = ( QWORD ) pstFile;
    return ExecuteSystemCall( SYSCALL_FWRITE, &stParameter );    
}

int fseek( FILE* pstFile, int iOffset, int iOrigin )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pstFile;
    PARAM( 1 ) = ( QWORD ) iOffset;
    PARAM( 2 ) = ( QWORD ) iOrigin;
    return ( int ) ExecuteSystemCall( SYSCALL_FSEEK, &stParameter );     
}

int fclose( FILE* pstFile )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pstFile;
    return ( int ) ExecuteSystemCall( SYSCALL_FCLOSE, &stParameter );      
}

int remove( const char* pcFileName )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pcFileName;
    return ( int ) ExecuteSystemCall( SYSCALL_REMOVE, &stParameter );          
}

DIR* opendir( const char* pcDirectoryName )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pcDirectoryName;
    return ( DIR* ) ExecuteSystemCall( SYSCALL_OPENDIR, &stParameter );         
}

struct dirent* readdir( DIR* pstDirectory )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pstDirectory;
    return ( struct dirent* ) ExecuteSystemCall( SYSCALL_READDIR, &stParameter );       
}

BOOL rewinddir( DIR* pstDirectory )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pstDirectory;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_REWINDDIR, &stParameter );          
}

int closedir( DIR* pstDirectory )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pstDirectory;
    return ( int ) ExecuteSystemCall( SYSCALL_CLOSEDIR, &stParameter );       
}

BOOL IsFileOpened( const struct dirent* pstEntry )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pstEntry;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_ISFILEOPENED, &stParameter );    
}

int ReadHDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) bPrimary;
    PARAM( 1 ) = ( QWORD ) bMaster;
    PARAM( 2 ) = ( QWORD ) dwLBA;
    PARAM( 3 ) = ( QWORD ) iSectorCount;
    PARAM( 4 ) = ( QWORD ) pcBuffer;
    return ( int ) ExecuteSystemCall( SYSCALL_READHDDSECTOR, &stParameter );     
}

int WriteHDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, 
        char* pcBuffer )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) bPrimary;
    PARAM( 1 ) = ( QWORD ) bMaster;
    PARAM( 2 ) = ( QWORD ) dwLBA;
    PARAM( 3 ) = ( QWORD ) iSectorCount;
    PARAM( 4 ) = ( QWORD ) pcBuffer;
    return ( int ) ExecuteSystemCall( SYSCALL_WRITEHDDSECTOR, &stParameter );      
}

QWORD CreateTask( QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, 
                  QWORD qwEntryPointAddress, BYTE bAffinity )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) qwFlags;
    PARAM( 1 ) = ( QWORD ) pvMemoryAddress;
    PARAM( 2 ) = ( QWORD ) qwMemorySize;
    PARAM( 3 ) = ( QWORD ) qwEntryPointAddress;
    PARAM( 4 ) = ( QWORD ) bAffinity;
    return ExecuteSystemCall( SYSCALL_CREATETASK, &stParameter );     
}

BOOL Schedule( void )
{
    return ( BOOL) ExecuteSystemCall( SYSCALL_SCHEDULE, NULL );
}

BOOL ChangePriority( QWORD qwID, BYTE bPriority, BOOL bExecutedInInterrupt )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwID;
    PARAM( 1 ) = ( QWORD ) bPriority;
    PARAM( 2 ) = ( QWORD ) bExecutedInInterrupt;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_CHANGEPRIORITY, &stParameter );        
}

BOOL EndTask( QWORD qwTaskID )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwTaskID;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_ENDTASK, &stParameter );     
}

void exit( int iExitValue )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) iExitValue;
    ExecuteSystemCall( SYSCALL_EXITTASK, &stParameter );      
}

int GetTaskCount( BYTE bAPICID )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) bAPICID;
    return ( int ) ExecuteSystemCall( SYSCALL_GETTASKCOUNT, &stParameter );     
}

BOOL IsTaskExist( QWORD qwID )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwID;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_ISTASKEXIST, &stParameter );       
}

QWORD GetProcessorLoad( BYTE bAPICID )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) bAPICID;
    return ExecuteSystemCall( SYSCALL_GETPROCESSORLOAD, &stParameter );   
}

BOOL ChangeProcessorAffinity( QWORD qwTaskID, BYTE bAffinity )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwTaskID;
    PARAM( 1 ) = ( QWORD ) bAffinity;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_CHANGEPROCESSORAFFINITY, &stParameter );      
}

QWORD GetBackgroundWindowID( void )
{
    return ExecuteSystemCall( SYSCALL_GETBACKGROUNDWINDOWID, NULL );         
}

void GetScreenArea( RECT* pstScreenArea )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pstScreenArea;
    ExecuteSystemCall( SYSCALL_GETSCREENAREA, &stParameter );     
}

QWORD CreateWindow( int iX, int iY, int iWidth, int iHeight, DWORD dwFlags,
        const char* pcTitle )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) iX;
    PARAM( 1 ) = ( QWORD ) iY;
    PARAM( 2 ) = ( QWORD ) iWidth;
    PARAM( 3 ) = ( QWORD ) iHeight;
    PARAM( 4 ) = ( QWORD ) dwFlags;
    PARAM( 5 ) = ( QWORD ) pcTitle;
    return ExecuteSystemCall( SYSCALL_CREATEWINDOW, &stParameter );
}

BOOL DeleteWindow( QWORD qwWindowID )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_DELETEWINDOW, &stParameter );    
}

BOOL ShowWindow( QWORD qwWindowID, BOOL bShow )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) bShow;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_SHOWWINDOW, &stParameter );    
}

QWORD FindWindowByPoint( int iX, int iY )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) iX;
    PARAM( 1 ) = ( QWORD ) iY;
    return ExecuteSystemCall( SYSCALL_FINDWINDOWBYPOINT, &stParameter );    
}

QWORD FindWindowByTitle( const char* pcTitle )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pcTitle;
    return ExecuteSystemCall( SYSCALL_FINDWINDOWBYTITLE, &stParameter );
}

BOOL IsWindowExist( QWORD qwWindowID )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_ISWINDOWEXIST, &stParameter );    
}

QWORD GetTopWindowID( void )
{
    return ExecuteSystemCall( SYSCALL_GETTOPWINDOWID, NULL );     
}

BOOL MoveWindowToTop( QWORD qwWindowID )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_MOVEWINDOWTOTOP, &stParameter );      
}

BOOL IsInTitleBar( QWORD qwWindowID, int iX, int iY )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) iX;
    PARAM( 2 ) = ( QWORD ) iY;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_ISINTITLEBAR, &stParameter );     
}

BOOL IsInCloseButton( QWORD qwWindowID, int iX, int iY )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) iX;
    PARAM( 2 ) = ( QWORD ) iY;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_ISINCLOSEBUTTON, &stParameter );     
}

BOOL MoveWindow( QWORD qwWindowID, int iX, int iY )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) iX;
    PARAM( 2 ) = ( QWORD ) iY;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_MOVEWINDOW, &stParameter );      
}

BOOL ResizeWindow( QWORD qwWindowID, int iX, int iY, int iWidth, int iHeight )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) iX;
    PARAM( 2 ) = ( QWORD ) iY;
    PARAM( 3 ) = ( QWORD ) iWidth;
    PARAM( 4 ) = ( QWORD ) iHeight;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_RESIZEWINDOW, &stParameter );    
}

BOOL GetWindowArea( QWORD qwWindowID, RECT* pstArea )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) pstArea;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_GETWINDOWAREA, &stParameter );    
}

BOOL UpdateScreenByID( QWORD qwWindowID )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_UPDATESCREENBYID, &stParameter );      
}

BOOL UpdateScreenByWindowArea( QWORD qwWindowID, const RECT* pstArea )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) pstArea;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_UPDATESCREENBYWINDOWAREA, &stParameter );     
}

BOOL UpdateScreenByScreenArea( const RECT* pstArea )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pstArea;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_UPDATESCREENBYSCREENAREA, &stParameter );      
}

BOOL SendEventToWindow( QWORD qwWindowID, const EVENT* pstEvent )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) pstEvent;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_SENDEVENTTOWINDOW, &stParameter );      
}

BOOL ReceiveEventFromWindowQueue( QWORD qwWindowID, EVENT* pstEvent )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) pstEvent;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_RECEIVEEVENTFROMWINDOWQUEUE, &stParameter );         
}

BOOL DrawWindowFrame( QWORD qwWindowID )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_DRAWWINDOWFRAME, &stParameter );      
}

BOOL DrawWindowBackground( QWORD qwWindowID )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_DRAWWINDOWBACKGROUND, &stParameter );     
}

BOOL DrawWindowTitle( QWORD qwWindowID, const char* pcTitle, BOOL bSelectedTitle )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) pcTitle;
    PARAM( 2 ) = ( QWORD ) bSelectedTitle;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_DRAWWINDOWTITLE, &stParameter );        
}

BOOL DrawButton( QWORD qwWindowID, RECT* pstButtonArea, COLOR stBackgroundColor, const char* pcText, COLOR stTextColor )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) pstButtonArea;
    PARAM( 2 ) = ( QWORD ) stBackgroundColor;
    PARAM( 3 ) = ( QWORD ) pcText;
    PARAM( 4 ) = ( QWORD ) stTextColor;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_DRAWBUTTON, &stParameter );      
}

BOOL DrawPixel( QWORD qwWindowID, int iX, int iY, COLOR stColor )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) iX;
    PARAM( 2 ) = ( QWORD ) iY;
    PARAM( 3 ) = ( QWORD ) stColor;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_DRAWPIXEL, &stParameter );     
}

BOOL DrawLine( QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) iX1;
    PARAM( 2 ) = ( QWORD ) iY1;
    PARAM( 3 ) = ( QWORD ) iX2;
    PARAM( 4 ) = ( QWORD ) iY2;
    PARAM( 5 ) = ( QWORD ) stColor;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_DRAWLINE, &stParameter );     
}

BOOL DrawRect( QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor, BOOL bFill )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) iX1;
    PARAM( 2 ) = ( QWORD ) iY1;
    PARAM( 3 ) = ( QWORD ) iX2;
    PARAM( 4 ) = ( QWORD ) iY2;
    PARAM( 5 ) = ( QWORD ) stColor;
    PARAM( 6 ) = ( QWORD ) bFill;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_DRAWRECT, &stParameter );       
}

BOOL DrawCircle( QWORD qwWindowID, int iX, int iY, int iRadius, COLOR stColor,
        BOOL bFill )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) iX;
    PARAM( 2 ) = ( QWORD ) iY;
    PARAM( 3 ) = ( QWORD ) iRadius;
    PARAM( 4 ) = ( QWORD ) stColor;
    PARAM( 5 ) = ( QWORD ) bFill;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_DRAWCIRCLE, &stParameter );      
}

BOOL DrawText( QWORD qwWindowID, int iX, int iY, COLOR stTextColor,
        COLOR stBackgroundColor, const char* pcString, int iLength )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) iX;
    PARAM( 2 ) = ( QWORD ) iY;
    PARAM( 3 ) = ( QWORD ) stTextColor;
    PARAM( 4 ) = ( QWORD ) stBackgroundColor;
    PARAM( 5 ) = ( QWORD ) pcString;
    PARAM( 6 ) = ( QWORD ) iLength;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_DRAWTEXT, &stParameter );     
}

void MoveCursor( int iX, int iY )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) iX;
    PARAM( 1 ) = ( QWORD ) iY;
    ExecuteSystemCall( SYSCALL_MOVECURSOR, &stParameter );        
}

void GetCursorPosition( int* piX, int* piY )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) piX;
    PARAM( 1 ) = ( QWORD ) piY;
    ExecuteSystemCall( SYSCALL_GETCURSORPOSITION, &stParameter );       
}

BOOL BitBlt( QWORD qwWindowID, int iX, int iY, COLOR* pstBuffer, int iWidth, int iHeight )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwWindowID;
    PARAM( 1 ) = ( QWORD ) iX;
    PARAM( 2 ) = ( QWORD ) iY;
    PARAM( 3 ) = ( QWORD ) pstBuffer;
    PARAM( 4 ) = ( QWORD ) iWidth;
    PARAM( 5 ) = ( QWORD ) iHeight;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_BITBLT, &stParameter );        
}

BOOL JPEGInit(JPEG *jpeg, BYTE* pbFileBuffer, DWORD dwFileSize)
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) jpeg;
    PARAM( 1 ) = ( QWORD ) pbFileBuffer;
    PARAM( 2 ) = ( QWORD ) dwFileSize;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_JPEGINIT, &stParameter );     
}

BOOL JPEGDecode(JPEG *jpeg, COLOR* rgb)
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) jpeg;
    PARAM( 1 ) = ( QWORD ) rgb;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_JPEGDECODE, &stParameter );       
}

BOOL ReadRTCTime( BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pbHour;
    PARAM( 1 ) = ( QWORD ) pbMinute;
    PARAM( 2 ) = ( QWORD ) pbSecond;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_READRTCTIME, &stParameter );        
}

BOOL ReadRTCDate( WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, BYTE* pbDayOfWeek )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pwYear;
    PARAM( 1 ) = ( QWORD ) pbMonth;
    PARAM( 2 ) = ( QWORD ) pbDayOfMonth;
    PARAM( 3 ) = ( QWORD ) pbDayOfWeek;
    return ( BOOL ) ExecuteSystemCall( SYSCALL_READRTCDATE, &stParameter );      
}

void SendSerialData( BYTE* pbBuffer, int iSize )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pbBuffer;
    PARAM( 1 ) = ( QWORD ) iSize;
    ExecuteSystemCall( SYSCALL_SENDSERIALDATA, &stParameter );
}

int ReceiveSerialData( BYTE* pbBuffer, int iSize )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = ( QWORD ) pbBuffer;
    PARAM( 1 ) = ( QWORD ) iSize;
    return ( int ) ExecuteSystemCall( SYSCALL_RECEIVESERIALDATA, &stParameter );
}

void ClearSerialFIFO( void )
{
    ExecuteSystemCall( SYSCALL_CLEARSERIALFIFO, NULL );
}

QWORD GetTotalRAMSize( void )
{
    return ExecuteSystemCall( SYSCALL_GETTOTALRAMSIZE, NULL );
}

QWORD GetTickCount( void )
{
    return ExecuteSystemCall( SYSCALL_GETTICKCOUNT, NULL );
}

void Sleep( QWORD qwMillisecond )
{
    PARAMETERTABLE stParameter;
    PARAM( 0 ) = qwMillisecond;
    ExecuteSystemCall( SYSCALL_SLEEP, &stParameter );    
}

BOOL IsGraphicMode( void )
{
    ExecuteSystemCall( SYSCALL_ISGRAPHICMODE, NULL );    
}