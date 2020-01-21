#include "OSLibrary.h"

#define EVENT_USER_TESTMESSAGE          0x80000001

int Main( char* pcArgument )
{
    QWORD qwWindowID;
    int iMouseX, iMouseY;
    int iWindowWidth, iWindowHeight;
    EVENT stReceivedEvent;
    MOUSEEVENT* pstMouseEvent;
    KEYEVENT* pstKeyEvent;
    WINDOWEVENT* pstWindowEvent;
    int iY;
    char vcTempBuffer[ 1024 ];
    static int s_iWindowCount = 0;
    char* vpcEventString[] = { 
            "Unknown",
            "MOUSE_MOVE       ",
            "MOUSE_LBUTTONDOWN",
            "MOUSE_LBUTTONUP  ",
            "MOUSE_RBUTTONDOWN",
            "MOUSE_RBUTTONUP  ",
            "MOUSE_MBUTTONDOWN",
            "MOUSE_MBUTTONUP  ",
            "WINDOW_SELECT    ",
            "WINDOW_DESELECT  ",
            "WINDOW_MOVE      ",
            "WINDOW_RESIZE    ",
            "WINDOW_CLOSE     ",            
            "KEY_DOWN         ",
            "KEY_UP           " };
    RECT stButtonArea;
    QWORD qwFindWindowID;
    EVENT stSendEvent;
    int i;
    
    if( IsGraphicMode() == FALSE )
    {        
        printf("This task can run only GUI Mode...\n");
        return -1;
    }
    
    GetCursorPosition( &iMouseX, &iMouseY );

    iWindowWidth = 500;
    iWindowHeight = 200;
    
    sprintf( vcTempBuffer, "Hello World Window %d", s_iWindowCount++ );
    qwWindowID = CreateWindow( iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2,
        iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT, vcTempBuffer );
    if( qwWindowID == WINDOW_INVALIDID )
    {
        return -1;
    }
    
    iY = WINDOW_TITLEBAR_HEIGHT + 10;
    
    DrawRect( qwWindowID, 10, iY + 8, iWindowWidth - 10, iY + 70, WINDOW_COLOR_WHITE,
            FALSE );
    sprintf( vcTempBuffer, "GUI Event Information[Window ID: 0x%Q, User Mode:%s]", 
            qwWindowID, pcArgument );
    DrawText( qwWindowID, 20, iY, WINDOW_COLOR_WHITE, WINDOW_COLOR_BACKGROUND,
               vcTempBuffer, strlen( vcTempBuffer ) );    
    
    SetRectangleData( 10, iY + 80, iWindowWidth - 10, iWindowHeight - 10, 
            &stButtonArea );
    DrawButton( qwWindowID, &stButtonArea, WINDOW_COLOR_BUTTONINACTIVATEBACKGROUND,
            "User Message Send Button(Up)", WINDOW_COLOR_WHITE);
    ShowWindow( qwWindowID, TRUE );
    
    while( 1 )
    {
        if( ReceiveEventFromWindowQueue( qwWindowID, &stReceivedEvent ) == FALSE )
        {
            Sleep( 0 );
            continue;
        }
        
        DrawRect( qwWindowID, 11, iY + 20, iWindowWidth - 11, iY + 69, 
                   WINDOW_COLOR_BACKGROUND, TRUE );        
        
        switch( stReceivedEvent.qwType )
        {
        case EVENT_MOUSE_MOVE:
        case EVENT_MOUSE_LBUTTONDOWN:
        case EVENT_MOUSE_LBUTTONUP:            
        case EVENT_MOUSE_RBUTTONDOWN:
        case EVENT_MOUSE_RBUTTONUP:
        case EVENT_MOUSE_MBUTTONDOWN:
        case EVENT_MOUSE_MBUTTONUP:
            pstMouseEvent = &( stReceivedEvent.stMouseEvent );
    
            sprintf( vcTempBuffer, "Mouse Event: %s", 
                      vpcEventString[ stReceivedEvent.qwType ] );
            DrawText( qwWindowID, 20, iY + 20, WINDOW_COLOR_WHITE,
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen( vcTempBuffer ) );
            
            sprintf( vcTempBuffer, "Data: X = %d, Y = %d, Button = %X", 
                     pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY,
                     pstMouseEvent->bButtonStatus );
            DrawText( qwWindowID, 20, iY + 40, WINDOW_COLOR_WHITE,
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen( vcTempBuffer ) );
    
            if( stReceivedEvent.qwType == EVENT_MOUSE_LBUTTONDOWN )
            {
                if( IsInRectangle( &stButtonArea, pstMouseEvent->stPoint.iX, 
                                    pstMouseEvent->stPoint.iY ) == TRUE )
                {
                    DrawButton( qwWindowID, &stButtonArea, 
                      WINDOW_COLOR_BUTTONACTIVATEBACKGROUND, "User Message Send Button(Down)",
                      WINDOW_COLOR_WHITE);
                    UpdateScreenByID( qwWindowID );
                    
                    stSendEvent.qwType = EVENT_USER_TESTMESSAGE;
                    stSendEvent.vqwData[ 0 ] = qwWindowID;
                    stSendEvent.vqwData[ 1 ] = 0x1234;
                    stSendEvent.vqwData[ 2 ] = 0x5678;
                    
                    for( i = 0 ; i < s_iWindowCount ; i++ )
                    {
                        sprintf( vcTempBuffer, "Hello World Window %d", i );
                        qwFindWindowID = FindWindowByTitle( vcTempBuffer );
                        if( ( qwFindWindowID != WINDOW_INVALIDID ) &&
                            ( qwFindWindowID != qwWindowID ) )
                        {
                            SendEventToWindow( qwFindWindowID, &stSendEvent );
                        }
                    }
                }
            }
            else if( stReceivedEvent.qwType == EVENT_MOUSE_LBUTTONUP )
            {
                DrawButton( qwWindowID, &stButtonArea, 
                  WINDOW_COLOR_BUTTONINACTIVATEBACKGROUND, "User Message Send Button(Up)",
                  WINDOW_COLOR_WHITE);
            }            
            break;
    
        case EVENT_KEY_DOWN:
        case EVENT_KEY_UP:
            pstKeyEvent = &( stReceivedEvent.stKeyEvent );
    
            sprintf( vcTempBuffer, "Key Event: %s", 
                      vpcEventString[ stReceivedEvent.qwType ] );
            DrawText( qwWindowID, 20, iY + 20, WINDOW_COLOR_WHITE,
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen( vcTempBuffer ) );
            
            sprintf( vcTempBuffer, "Data: Key = %c, Flag = %X", 
                    pstKeyEvent->bASCIICode, pstKeyEvent->bFlags );
            DrawText( qwWindowID, 20, iY + 40, WINDOW_COLOR_WHITE,
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen( vcTempBuffer ) );
            break;
    
        case EVENT_WINDOW_SELECT:
        case EVENT_WINDOW_DESELECT:
        case EVENT_WINDOW_MOVE:
        case EVENT_WINDOW_RESIZE:
        case EVENT_WINDOW_CLOSE:
            pstWindowEvent = &( stReceivedEvent.stWindowEvent );
    
            sprintf( vcTempBuffer, "Window Event: %s", 
                      vpcEventString[ stReceivedEvent.qwType ] );
            DrawText( qwWindowID, 20, iY + 20, WINDOW_COLOR_WHITE,
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen( vcTempBuffer ) );
            
            sprintf( vcTempBuffer, "Data: X1 = %d, Y1 = %d, X2 = %d, Y2 = %d", 
                    pstWindowEvent->stArea.iX1, pstWindowEvent->stArea.iY1, 
                    pstWindowEvent->stArea.iX2, pstWindowEvent->stArea.iY2 );
            DrawText( qwWindowID, 20, iY + 40, WINDOW_COLOR_WHITE,
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen( vcTempBuffer ) );
            
            if( stReceivedEvent.qwType == EVENT_WINDOW_CLOSE )
            {
                DeleteWindow( qwWindowID );
                return 0;
            }
            break;
            
        default:
            sprintf( vcTempBuffer, "Unknown Event: 0x%X", stReceivedEvent.qwType );
            DrawText( qwWindowID, 20, iY + 20, WINDOW_COLOR_WHITE, WINDOW_COLOR_BACKGROUND,
                       vcTempBuffer, strlen( vcTempBuffer ) );
            
            sprintf( vcTempBuffer, "Data0 = 0x%Q, Data1 = 0x%Q, Data2 = 0x%Q",
                      stReceivedEvent.vqwData[ 0 ], stReceivedEvent.vqwData[ 1 ], 
                      stReceivedEvent.vqwData[ 2 ] );
            DrawText( qwWindowID, 20, iY + 40, WINDOW_COLOR_WHITE,
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen( vcTempBuffer ) );
            break;
        }
    
        ShowWindow( qwWindowID, TRUE );
    }

    return 0;
}

#if 0
void BaseGUITask( void )
{
    QWORD qwWindowID;
    int iMouseX, iMouseY;
    int iWindowWidth, iWindowHeight;
    EVENT stReceivedEvent;
    MOUSEEVENT* pstMouseEvent;
    KEYEVENT* pstKeyEvent;
    WINDOWEVENT* pstWindowEvent;

    if( IsGraphicMode() == FALSE )
    {        
        printf( "This task can run only GUI mode~!!!\n" );
        return ;
    }
    
    GetCursorPosition( &iMouseX, &iMouseY );

    iWindowWidth = 500;
    iWindowHeight = 200;
    
    qwWindowID = CreateWindow( iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2,
        iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE,
         "Hello World Window" );
    if( qwWindowID == WINDOW_INVALIDID )
    {
        return ;
    }
    
    while( 1 )
    {
        if( ReceiveEventFromWindowQueue( qwWindowID, &stReceivedEvent ) == FALSE )
        {
            Sleep( 0 );
            continue;
        }
        
        switch( stReceivedEvent.qwType )
        {
        case EVENT_MOUSE_MOVE:
        case EVENT_MOUSE_LBUTTONDOWN:
        case EVENT_MOUSE_LBUTTONUP:            
        case EVENT_MOUSE_RBUTTONDOWN:
        case EVENT_MOUSE_RBUTTONUP:
        case EVENT_MOUSE_MBUTTONDOWN:
        case EVENT_MOUSE_MBUTTONUP:
            pstMouseEvent = &( stReceivedEvent.stMouseEvent );
            break;

        case EVENT_KEY_DOWN:
        case EVENT_KEY_UP:
            pstKeyEvent = &( stReceivedEvent.stKeyEvent );
            break;

        case EVENT_WINDOW_SELECT:
        case EVENT_WINDOW_DESELECT:
        case EVENT_WINDOW_MOVE:
        case EVENT_WINDOW_RESIZE:
        case EVENT_WINDOW_CLOSE:
            pstWindowEvent = &( stReceivedEvent.stWindowEvent );

            if( stReceivedEvent.qwType == EVENT_WINDOW_CLOSE )
            {
                DeleteWindow( qwWindowID );
                return ;
            }
            break;
            
        default:
            break;
        }
    }
}
#endif
