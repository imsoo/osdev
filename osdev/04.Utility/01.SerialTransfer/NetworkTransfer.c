#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DWORD               unsigned int
#define BYTE                unsigned char
#define MIN( x, y )         ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )

#define SERIAL_FIFOMAXSIZE  16

int main( int argc, char** argv )
{
    char vcFileName[ 256 ];
    char vcDataBuffer[ SERIAL_FIFOMAXSIZE ];
    struct sockaddr_in stSocketAddr;
    int iSocket;
    BYTE bAck;
    DWORD dwDataLength;
    DWORD dwSentSize;
    DWORD dwTemp;
    FILE* fp;
    
    if( argc < 2 )
    {
        fprintf( stderr, "Input File Name: " );
        gets( vcFileName );
    }
    else
    {
        strcpy( vcFileName, argv[ 1 ] );
    }

    fp = fopen( vcFileName, "rb" );
    if( fp == NULL )
    {
        fprintf( stderr, "%s File Open Error\n", vcFileName );
        return 0;
    }
    
    fseek( fp, 0, SEEK_END );
    dwDataLength = ftell( fp );
    fseek( fp, 0, SEEK_SET );
    fprintf( stderr, "File Name %s, Data Length %d Byte\n", vcFileName, 
            dwDataLength );
    
    stSocketAddr.sin_family = AF_INET;
    stSocketAddr.sin_port = htons( 4444 );
    stSocketAddr.sin_addr.s_addr = inet_addr( "127.0.0.1" );

    iSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if( connect( iSocket, ( struct sockaddr* ) &stSocketAddr, 
                 sizeof( stSocketAddr ) ) == -1 )
    {
        fprintf( stderr, "Socket Connect Error, IP 127.0.0.1, Port 4444\n" );
        return 0;
    }
    else
    {
        fprintf( stderr, "Socket Connect Success, IP 127.0.0.1, Port 4444\n" );
    }
    
	// getchar();
	
    if( send( iSocket, &dwDataLength, 4, 0 ) != 4 )
    {
        fprintf( stderr, "Data Length Send Fail, [%d] Byte\n", dwDataLength );
        return 0;
    }
    else
    {
        fprintf( stderr, "Data Length Send Success, [%d] Byte\n", dwDataLength );
    }
    if( recv( iSocket, &bAck, 1, 0 ) != 1 )
    {
        fprintf( stderr, "Ack Receive Error\n" );
        return 0;
    }
    
    fprintf( stderr, "Now Data Transfer..." );
    dwSentSize = 0;
    while( dwSentSize < dwDataLength )
    {
        dwTemp = MIN( dwDataLength - dwSentSize, SERIAL_FIFOMAXSIZE );
        dwSentSize += dwTemp;
        
        if( fread( vcDataBuffer, 1, dwTemp, fp ) != dwTemp )
        {
            fprintf( stderr, "File Read Error\n" );
            return 0;
        }
        
        if( send( iSocket, vcDataBuffer, dwTemp, 0 ) != dwTemp )
        {
            fprintf( stderr, "Socket Send Error\n" );
            return 0;
        }
        
        if( recv( iSocket, &bAck, 1, 0 ) != 1 )
        {
            fprintf( stderr, "Ack Receive Error\n" );
            return 0;
        }
        fprintf( stderr, "#" );
    }
    
    fclose( fp );
    close( iSocket );
    
    fprintf( stderr, "\nSend Complete. Total Size [%d] Byte\n", dwSentSize );
    fprintf( stderr, "Press Enter Key To Exit\n" );
    getchar();
    
    return 0;
}
