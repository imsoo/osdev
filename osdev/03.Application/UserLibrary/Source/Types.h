#ifndef __TYPES_H__
#define __TYPES_H__

#define BYTE    unsigned char
#define WORD    unsigned short
#define DWORD   unsigned int
#define QWORD   unsigned long
#define BOOL    unsigned char

#define TRUE    1
#define FALSE   0
#define NULL    0

// Console
#define stderr  2
#define stdout  1
#define stdin   0
#define CONSOLE_WIDTH                       80
#define CONSOLE_HEIGHT                      25

// Key
#define KEY_FLAGS_UP             0x00
#define KEY_FLAGS_DOWN           0x01
#define KEY_FLAGS_EXTENDEDKEY    0x02

#define KEY_NONE        0x00
#define KEY_ENTER       '\n'
#define KEY_TAB         '\t'
#define KEY_ESC         0x1B
#define KEY_BACKSPACE   0x08

#define KEY_CTRL        0x81
#define KEY_LSHIFT      0x82
#define KEY_RSHIFT      0x83
#define KEY_PRINTSCREEN 0x84
#define KEY_LALT        0x85
#define KEY_CAPSLOCK    0x86
#define KEY_F1          0x87
#define KEY_F2          0x88
#define KEY_F3          0x89
#define KEY_F4          0x8A
#define KEY_F5          0x8B
#define KEY_F6          0x8C
#define KEY_F7          0x8D
#define KEY_F8          0x8E
#define KEY_F9          0x8F
#define KEY_F10         0x90
#define KEY_NUMLOCK     0x91
#define KEY_SCROLLLOCK  0x92
#define KEY_HOME        0x93
#define KEY_UP          0x94
#define KEY_PAGEUP      0x95
#define KEY_LEFT        0x96
#define KEY_CENTER      0x97
#define KEY_RIGHT       0x98
#define KEY_END         0x99
#define KEY_DOWN        0x9A
#define KEY_PAGEDOWN    0x9B
#define KEY_INS         0x9C
#define KEY_DEL         0x9D
#define KEY_F11         0x9E
#define KEY_F12         0x9F
#define KEY_PAUSE       0xA0

// Task
#define TASK_FLAGS_HIGHEST            0
#define TASK_FLAGS_HIGH               1
#define TASK_FLAGS_MEDIUM             2
#define TASK_FLAGS_LOW                3
#define TASK_FLAGS_LOWEST             4
#define TASK_FLAGS_WAIT               0xFF          

#define TASK_FLAGS_ENDTASK            0x8000000000000000
#define TASK_FLAGS_SYSTEM             0x4000000000000000
#define TASK_FLAGS_PROCESS            0x2000000000000000
#define TASK_FLAGS_THREAD             0x1000000000000000
#define TASK_FLAGS_IDLE               0x0800000000000000
#define TASK_FLAGS_USERLEVEL          0x0400000000000000

#define GETPRIORITY( x )            ( ( x ) & 0xFF )
#define SETPRIORITY( x, priority )  ( ( x ) = ( ( x ) & 0xFFFFFFFFFFFFFF00 ) | \
        ( priority ) )
#define GETTCBOFFSET( x )           ( ( x ) & 0xFFFFFFFF )

#define TASK_INVALIDID              0xFFFFFFFFFFFFFFFF

#define TASK_LOADBALANCINGID        0xFF

// File
#define EOF (-1)
#define FILESYSTEM_MAXFILENAMELENGTH        24

#define SEEK_SET                            0
#define SEEK_CUR                            1
#define SEEK_END                            2

#define size_t      DWORD       
#define dirent      DirectoryEntryStruct
#define d_name      vcFileName

// GUI
typedef WORD    COLOR;

#define RGB( r, g, b )      ( ( ( BYTE )( r ) >> 3 ) << 11 | \
                ( ( ( BYTE )( g ) >> 2 ) ) << 5 |  ( ( BYTE )( b ) >> 3 ) )

#define WINDOW_TITLEMAXLENGTH       40

#define WINDOW_INVALIDID            0xFFFFFFFFFFFFFFFF

#define WINDOW_FLAGS_SHOW               0x00000001
#define WINDOW_FLAGS_DRAWFRAME          0x00000002
#define WINDOW_FLAGS_DRAWTITLE          0x00000004
#define WINDOW_FLAGS_RESIZABLE          0x00000008
#define WINDOW_FLAGS_DEFAULT        ( WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | \
                                      WINDOW_FLAGS_DRAWTITLE )

#define WINDOW_TITLEBAR_HEIGHT      21
#define WINDOW_XBUTTON_SIZE         19
#define WINDOW_WIDTH_MIN            ( WINDOW_XBUTTON_SIZE * 2 + 30 )
#define WINDOW_HEIGHT_MIN           ( WINDOW_TITLEBAR_HEIGHT + 30 )

// Window Color
#define WINDOW_COLOR_FRAME                  RGB( 50, 50, 50 )
#define WINDOW_COLOR_BACKGROUND             RGB( 30, 30, 30 )
#define WINDOW_COLOR_FRAMETEXT              RGB(255, 255, 255)
#define WINDOW_COLOR_WHITE                  RGB(255, 255, 255)

#define WINDOW_COLOR_TITLEBARABACKGROUND    RGB( 45, 45, 48 )
#define WINDOW_COLOR_TITLEBARACTIVATETEXT   RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARINACTIVATETEXT RGB(100, 100, 100)


#define WINDOW_COLOR_TITLEBARBRIGHT1        RGB( 45, 45, 48 )
#define WINDOW_COLOR_TITLEBARBRIGHT2        RGB( 45, 45, 48 )
#define WINDOW_COLOR_TITLEBARUNDERLINE      RGB( 50, 50, 50 )

#define WINDOW_COLOR_BUTTONACTIVATEBACKGROUND   RGB( 75, 75, 75 )
#define WINDOW_COLOR_BUTTONINACTIVATEBACKGROUND RGB( 50, 50, 50 )

#define WINDOW_COLOR_BUTTONBRIGHT           RGB( 45, 45, 48 )
#define WINDOW_COLOR_BUTTONDARK             RGB( 45, 45, 48 )
#define WINDOW_COLOR_SYSTEMBACKGROUND       RGB( 232, 255, 232 )
#define WINDOW_COLOR_XBUTTONLINECOLOR       RGB(255, 255, 255)
#define WINDOW_BACKGROUNDWINDOWTITLE            "SYS_BACKGROUND"

// Event
#define EVENT_UNKNOWN                                   0
#define EVENT_MOUSE_MOVE                                1
#define EVENT_MOUSE_LBUTTONDOWN                         2
#define EVENT_MOUSE_LBUTTONUP                           3
#define EVENT_MOUSE_RBUTTONDOWN                         4
#define EVENT_MOUSE_RBUTTONUP                           5
#define EVENT_MOUSE_MBUTTONDOWN                         6
#define EVENT_MOUSE_MBUTTONUP                           7
#define EVENT_WINDOW_SELECT                             8
#define EVENT_WINDOW_DESELECT                           9
#define EVENT_WINDOW_MOVE                               10
#define EVENT_WINDOW_RESIZE                             11
#define EVENT_WINDOW_CLOSE                              12
#define EVENT_KEY_DOWN                                  13
#define EVENT_KEY_UP                                    14

#define FONT_ENGLISHWIDTH   8
#define FONT_ENGLISHHEIGHT  16

#define MIN( x, y )     ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )
#define MAX( x, y )     ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )

#pragma pack( push, 1 )

typedef struct KeyDataStruct
{
    BYTE bScanCode;
    BYTE bASCIICode;
    BYTE bFlags;
} KEYDATA;

typedef struct DirectoryEntryStruct
{
    char vcFileName[ FILESYSTEM_MAXFILENAMELENGTH ];
    DWORD dwFileSize;
    DWORD dwStartClusterIndex;
} DIRECTORYENTRY;

#pragma pack( pop )

typedef struct kFileHandleStruct
{
    int iDirectoryEntryOffset;
    DWORD dwFileSize;
    DWORD dwStartClusterIndex;
    DWORD dwCurrentClusterIndex;
    DWORD dwPreviousClusterIndex;
    DWORD dwCurrentOffset;
    DWORD dwFlags;
} FILEHANDLE;

typedef struct kDirectoryHandleStruct
{
    DIRECTORYENTRY* pstDirectoryBuffer;
    int iCurrentOffset;
} DIRECTORYHANDLE;

typedef struct kFileDirectoryHandleStruct
{
    BYTE bType;
    union
    {
        FILEHANDLE stFileHandle;
        DIRECTORYHANDLE stDirectoryHandle;
    };    
} FILE, DIR;

typedef struct kRectangleStruct
{
    int iX1;
    int iY1;
    int iX2;
    int iY2;
} RECT;

typedef struct kPointStruct
{
    int iX;
    int iY;
} POINT;

typedef struct kMouseEventStruct
{
    QWORD qwWindowID;
    POINT stPoint;
    BYTE bButtonStatus;
} MOUSEEVENT;

typedef struct kKeyEventStruct
{
    QWORD qwWindowID;
    BYTE bASCIICode;
    BYTE bScanCode;    
    BYTE bFlags;
} KEYEVENT;

typedef struct kWindowEventStruct
{
    QWORD qwWindowID;
    RECT stArea;
} WINDOWEVENT;

typedef struct kEventStruct
{
    QWORD qwType;
    union
    {
        MOUSEEVENT stMouseEvent;
        KEYEVENT stKeyEvent;
        WINDOWEVENT stWindowEvent;
        QWORD vqwData[ 3 ];
    };
} EVENT;

typedef struct{
    int elem; 
    unsigned short code[256];
    unsigned char  size[256];
    unsigned char  value[256];
}HUFF;

typedef struct{
    int width;
    int height;
    int mcu_width;
    int mcu_height;
    int max_h,max_v;
    int compo_count;
    int compo_id[3];
    int compo_sample[3];
    int compo_h[3];
    int compo_v[3];
    int compo_qt[3];
    int scan_count;
    int scan_id[3];
    int scan_ac[3];
    int scan_dc[3];
    int scan_h[3];  
    int scan_v[3];  
    int scan_qt[3]; 
    int interval;
    int mcu_buf[32*32*4]; 
    int *mcu_yuv[4];
    int mcu_preDC[3];
    int dqt[3][64];
    int n_dqt;
    HUFF huff[2][3];
    unsigned char *data;
    int data_index;
    int data_size;
    unsigned long bit_buff;
    int bit_remain;
}JPEG;

#endif /*TYPES_H_*/
