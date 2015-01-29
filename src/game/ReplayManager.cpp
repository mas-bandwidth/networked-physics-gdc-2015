#include "ReplayManager.h"

#ifdef CLIENT

#include "Global.h"
#include "CommandLine.h"
#include "DemoManager.h"
#include "InputManager.h"
#include "core/Memory.h"
#include "protocol/Stream.h"
#include "protocol/Object.h"
#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define CAPTURE 1

const int MaxReplayMessageSize = 8 * 1024;

#if CAPTURE
const int NumPixelBufferObjects = 2;
#endif // #if CAPTURE

enum ReplayMessages
{
    REPLAY_RANDOM_SEED,
    REPLAY_COMMAND_LINE,
    REPLAY_KEY_EVENT,
    REPLAY_CHAR_EVENT,
    REPLAY_UPDATE,
    REPLAY_END,
    REPLAY_NUM_MESSAGES
};

struct ReplayRandomSeed : public protocol::Object
{
    uint32_t seed;

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_uint32( stream, seed );
    }
};

struct ReplayCommandLine : public protocol::Object
{
    char commandLine[CommandLineBufferSize];

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_string( stream, commandLine, sizeof( commandLine ) );
    }
};

struct ReplayKeyEvent : public protocol::Object
{
    int key;
    int scancode;
    int action;
    int mods;

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_int32( stream, key );
        serialize_int32( stream, scancode );
        serialize_int32( stream, action );
        serialize_int32( stream, mods );
    }
};

struct ReplayCharEvent : public protocol::Object
{
    uint32_t code;

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_uint32( stream, code );
    }
};

struct ReplayUpdate : public protocol::Object
{
    double time;
    float deltaTime;

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_double( stream, time );
        serialize_float( stream, deltaTime );
    }
};

struct ReplayInternal
{
    bool recording;
    bool playback;
    FILE * file;

#if CAPTURE
    int index;
    uint64_t frame;
    GLuint pboIds[NumPixelBufferObjects];
#endif // #if CAPTURE

    ReplayInternal()
    {
        recording = false;
        playback = false;
        file = nullptr;
#if CAPTURE
        frame = 0;
        index = 0;
        memset( pboIds, 0, sizeof( pboIds ) );
#endif // #if CAPTURE
    }
};

ReplayManager::ReplayManager( core::Allocator & allocator )
{
    m_allocator = &allocator;

    m_internal = CORE_NEW( allocator, ReplayInternal );

#if CAPTURE

    const int dataSize = global.displayWidth * global.displayHeight * 3;

    glGenBuffers( NumPixelBufferObjects, m_internal->pboIds );

    for ( int i = 0; i < NumPixelBufferObjects; ++i )
    {
        glBindBuffer( GL_PIXEL_UNPACK_BUFFER, m_internal->pboIds[i] );
        glBufferData( GL_PIXEL_UNPACK_BUFFER, dataSize, 0, GL_STREAM_DRAW );
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );

#endif // #if CAPTURE
}

ReplayManager::~ReplayManager()
{
    CORE_ASSERT( m_allocator );
    CORE_ASSERT( m_internal );

    Stop();

#if CAPTURE

    for ( int i = 0; i < NumPixelBufferObjects; ++i )
    {
        glDeleteBuffers( 1, &m_internal->pboIds[i] );
        m_internal->pboIds[i] = 0;
    }

    CORE_DELETE( *m_allocator, ReplayInternal, m_internal );

#endif // #if CAPTURE
}

void ReplayManager::StartRecording( const char filename[] )
{
    printf( "%.3f: Replay recording started: \"%s\"\n", global.timeBase.time, filename );

    Stop();

    CORE_ASSERT( m_internal->file == nullptr );

    m_internal->file = fopen( filename, "wb" );
    if ( !m_internal->file )
    {
        printf( "%.3f: error: could not open replay file for write\n", global.timeBase.time );
        Stop();
    }

    if ( fwrite( "REPLAY", 6, 1, m_internal->file ) != 1 )
    {
        printf( "%.3f: error: failed to write replay file header\n", global.timeBase.time );
        Stop();
        return;
    }

    m_internal->recording = true;
}

void ReplayManager::StartPlayback( const char filename[] )
{
    printf( "%.3f: Replay playback started: \"%s\"\n", global.timeBase.time, filename );

    Stop();

    CORE_ASSERT( m_internal->file == nullptr );

    m_internal->playback = true;

    m_internal->file = fopen( filename, "r" );
    if ( !m_internal->file )
    {
        printf( "%.3f: error: replay file does not exist\n", global.timeBase.time );
        Stop();
        return;
    }

    char header[6];
    fread( header, 6, 1, m_internal->file );
    if ( header[0] != 'R' ||
         header[1] != 'E' ||
         header[2] != 'P' ||
         header[3] != 'L' ||
         header[4] != 'A' ||
         header[5] != 'Y' )
    {
        printf( "%.3f: error: replay file is missing header\n", global.timeBase.time );
        Stop();
        return;
    }

    global.demoManager->UnloadDemo();
}

void ReplayManager::Stop()
{
    if ( m_internal->recording )
        printf( "%.3f: Replay recording stopped.\n", global.timeBase.time );
    else if ( m_internal->playback )
        printf( "%.3f: Replay playback stopped.\n", global.timeBase.time );
    else
        return;

    if ( m_internal->file )
    {
        if ( m_internal->recording )
        {
            int end = REPLAY_END;
            fwrite( &end, sizeof(end), 1, m_internal->file );
        }

        fclose( m_internal->file );
        m_internal->file = nullptr;
    }

    m_internal->recording = false;
    m_internal->playback = false;
}

bool ReplayManager::IsRecording() const
{
    return m_internal->recording;
}

bool ReplayManager::IsPlayback() const
{
    return m_internal->playback;
}

void ReplayManager::RecordRandomSeed( unsigned int seed )
{
    if ( !IsRecording() ) 
        return;

    ReplayRandomSeed message;
    message.seed = seed;

    RecordMessage( REPLAY_RANDOM_SEED, message );
}

void ReplayManager::RecordCommandLine( const char * commandLine )
{
    CORE_ASSERT( commandLine );

    if ( !IsRecording() )
        return;

    ReplayCommandLine message;
    strncpy( message.commandLine, commandLine, sizeof( message.commandLine ) );
    message.commandLine[CommandLineBufferSize-1] = '\0';
    
    RecordMessage( REPLAY_COMMAND_LINE, message );
}

void ReplayManager::RecordKeyEvent( int key, int scancode, int action, int mods )
{
    if ( !IsRecording() )
        return;

    ReplayKeyEvent message;
    message.key = key;
    message.scancode = scancode;
    message.action = action;
    message.mods = mods;

    RecordMessage( REPLAY_KEY_EVENT, message );
}

void ReplayManager::RecordCharEvent( unsigned int code )
{
    if ( !IsRecording() )
        return;

    ReplayCharEvent message;
    message.code = code;

    RecordMessage( REPLAY_CHAR_EVENT, message );
}

void ReplayManager::RecordUpdate()
{
    if ( !IsRecording() )
        return;

    ReplayUpdate message;
    message.time = global.timeBase.time;
    message.deltaTime = global.timeBase.deltaTime;

    RecordMessage( REPLAY_UPDATE, message );
}

void ReplayManager::RecordMessage( int type, protocol::Object & message )
{
    CORE_ASSERT( type >= 0 );
    CORE_ASSERT( type < REPLAY_NUM_MESSAGES );
    CORE_ASSERT( IsRecording() );
    CORE_ASSERT( m_internal->file );

    uint8_t buffer[MaxReplayMessageSize];
    protocol::WriteStream stream( buffer, sizeof( buffer ) );
    message.SerializeWrite( stream );
    stream.Flush();
    if ( stream.IsOverflow() )
        return;

    fwrite( &type, sizeof(type), 1, m_internal->file );

    int bytes = stream.GetBytesProcessed();
    fwrite( &bytes, sizeof(bytes), 1, m_internal->file );

    fwrite( buffer, bytes, 1, m_internal->file );
}

void ReplayManager::UpdatePlayback()
{
    if ( !m_internal->playback )
        return;

    CORE_ASSERT( m_internal->file );

    while ( true )
    {
        int type;
        if ( fread( &type, sizeof(type), 1, m_internal->file ) != 1 )
        {
            printf( "%.3f: error: failed to read replay message type\n", global.timeBase.time );
            Stop();
            break;
        }

        if ( type == REPLAY_END )
        {
            Stop();
            return;
        }

        int bytes;
        if ( fread( &bytes, sizeof(bytes), 1, m_internal->file ) != 1 )
        {
            printf( "%.3f: error: failed to read replay message size\n", global.timeBase.time );
            Stop();
            break;
        }

        if ( bytes <= 0 || bytes > MaxReplayMessageSize )
        {
            printf( "%.3f: error: replay message size out of bounds (%d)\n", global.timeBase.time, bytes );
            Stop();
            break;
        }

        uint8_t buffer[MaxReplayMessageSize];
        if ( fread( buffer, bytes, 1, m_internal->file ) != 1 )
        {
            printf( "%.3f: error: failed to read replay message data (type=%d, bytes=%d)\n", global.timeBase.time, type, bytes );
            Stop();
            break;
        }

        protocol::ReadStream stream( buffer, MaxReplayMessageSize );

        switch ( type )
        {
            case REPLAY_RANDOM_SEED:
            {
                ReplayRandomSeed message;

                message.SerializeRead( stream );

                srand( message.seed );
            }
            break;

            case REPLAY_COMMAND_LINE:
            {
                ReplayCommandLine message;

                message.SerializeRead( stream );

                StoreCommandLine( message.commandLine );

                ProcessCommandLine();
            }
            break;

            case REPLAY_KEY_EVENT:
            {
                ReplayKeyEvent message;

                message.SerializeRead( stream );

                global.inputManager->KeyEvent( message.key, message.scancode, message.action, message.mods );
            }
            break;

            case REPLAY_CHAR_EVENT:
            {
                ReplayCharEvent message;

                message.SerializeRead( stream );

                global.inputManager->CharEvent( message.code );
            }
            break;

            case REPLAY_UPDATE:
            {
                ReplayUpdate message;

                message.SerializeRead( stream );

                global.timeBase.time = message.time;
                global.timeBase.deltaTime = message.deltaTime;

                return;
            }
            break;

            default:
            {
                printf( "%.3f: error: unknown replay message type %d\n", global.timeBase.time, type );
                Stop();
                return;
            }
        }
    }
}

bool WriteTGA( const char filename[], int width, int height, uint8_t * ptr )
{
    FILE * file = fopen( filename, "wb" );
    if ( !file )
        return false;

    putc( 0, file );
    putc( 0, file );
    putc( 10, file );                        /* compressed RGB */
    putc( 0, file ); putc( 0, file );
    putc( 0, file ); putc( 0, file );
    putc( 0, file );
    putc( 0, file ); putc( 0, file );           /* X origin */
    putc( 0, file ); putc( 0, file );           /* y origin */
    putc( ( width & 0x00FF ),file );
    putc( ( width & 0xFF00 ) >> 8,file );
    putc( ( height & 0x00FF ), file );
    putc( ( height & 0xFF00 ) >> 8, file );
    putc( 24, file );                         /* 24 bit bitmap */
    putc( 0, file );

    for ( int y = 0; y < height; ++y )
    {
        uint8_t * line = ptr + width * 3 * y;
        uint8_t * end_of_line = line + width * 3;
        uint8_t * pixel = line;
        while ( true )
        {
            if ( pixel >= end_of_line )
                break;

            uint8_t * start = pixel;
            uint8_t * finish = pixel + 128 * 3;
            if ( finish > end_of_line )
                finish = end_of_line;
            uint32_t previous = ( pixel[0] << 16 ) | ( pixel[1] << 8 ) | pixel[2];
            pixel += 3;
            int counter = 1;

            // RLE packet
            while ( pixel < finish )
            {
                CORE_ASSERT( pixel < end_of_line );
                uint32_t current = ( pixel[0] << 16 ) | ( pixel[1] << 8 ) | pixel[2];
                if ( current != previous )
                    break;
                previous = current;
                pixel += 3;
                counter++;
            }
            if ( counter > 1 )
            {
                CORE_ASSERT( counter <= 128 );
                putc( uint8_t( counter - 1 ) | 128, file );
                putc( start[0], file );
                putc( start[1], file );
                putc( start[2], file );
                continue;
            }

            // RAW packet
            while ( pixel < finish )
            {
                CORE_ASSERT( pixel < end_of_line );
                uint32_t current = ( pixel[0] << 16 ) | ( pixel[1] << 8 ) | pixel[2];
                if ( current == previous )
                    break;
                previous = current;
                pixel += 3;
                counter++;
            }
            CORE_ASSERT( counter >= 1 );
            CORE_ASSERT( counter <= 128 );
            putc( uint8_t( counter - 1 ), file );
            fwrite( start, counter * 3, 1, file );
        }
    }

    fclose( file );

    return true;
}

void ReplayManager::UpdateCapture()
{
#if CAPTURE

    if ( !IsPlayback() )
        return;

    m_internal->index = ( m_internal->index + 1 ) % NumPixelBufferObjects;

    const int prevIndex = ( m_internal->index + NumPixelBufferObjects - 1 ) % NumPixelBufferObjects;

    glReadBuffer( GL_FRONT );

    glBindBufferARB( GL_PIXEL_PACK_BUFFER_ARB, m_internal->pboIds[m_internal->index] );

    glReadPixels( 0, 0, global.displayWidth, global.displayHeight, GL_BGR, GL_UNSIGNED_BYTE, 0 );
    
    if ( m_internal->frame > (unsigned) NumPixelBufferObjects )
    {
        glBindBufferARB( GL_PIXEL_PACK_BUFFER_ARB, m_internal->pboIds[prevIndex] );

        GLubyte * ptr = (GLubyte*) glMapBufferARB( GL_PIXEL_PACK_BUFFER_ARB,
                                                   GL_READ_ONLY_ARB );
        if ( ptr )
        {
            char filename[1024];
            sprintf( filename, "output/frame-%05d.tga", (unsigned int) ( m_internal->frame - NumPixelBufferObjects ) );
            WriteTGA( filename, global.displayWidth, global.displayHeight, ptr );
            glUnmapBufferARB( GL_PIXEL_PACK_BUFFER_ARB );
        }
    }

    glBindBufferARB( GL_PIXEL_PACK_BUFFER_ARB, 0 );

    m_internal->frame++;

#endif // #if CAPTURE
}

#endif // #ifdef CLIENT
