#include "ReplayManager.h"

#ifdef CLIENT

#include "Global.h"
#include "CommandLine.h"
#include "core/Memory.h"
#include "protocol/Stream.h"
#include "protocol/Object.h"
#include <stdio.h>

enum ReplayMessages
{
    REPLAY_RANDOM_SEED,
    REPLAY_COMMAND_LINE,
    REPLAY_KEY_EVENT,
    REPLAY_CHAR_EVENT,
    REPLAY_UPDATE,
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

    PROTOCOL_SERIALIZE_OBJECT( ReplayCommandLine )
    {
        // todo
//        serialize_string( stream, commandLine );
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
    float deltaTime;

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        // todo: serialize float
//        serialize_float( stream, deltaTime );
    }
};

struct ReplayInternal
{
    bool recording;
    bool playback;
    FILE * file;

    ReplayInternal()
    {
        recording = false;
        playback = false;
        file = nullptr;
    }
};

ReplayManager::ReplayManager( core::Allocator & allocator )
{
    m_allocator = &allocator;
    m_internal = CORE_NEW( allocator, ReplayInternal );
}

ReplayManager::~ReplayManager()
{
    CORE_ASSERT( m_allocator );
    CORE_ASSERT( m_internal );

    if ( m_internal->file )
    {
        fclose( m_internal->file );
        m_internal->file = nullptr;
    }

    CORE_DELETE( *m_allocator, ReplayInternal, m_internal );
}

void ReplayManager::StartRecording( const char filename[] )
{
    printf( "%.3f: Replay recording started: \"%s\"\n", global.timeBase.time, filename );

    Stop();

    CORE_ASSERT( m_internal->file == nullptr );

    m_internal->file = fopen( filename, "wb" );
    if ( !m_internal->file )
        return;

    // todo: write file header ("REPLAY")

    m_internal->recording = true;
}

void ReplayManager::StartPlayback( const char filename[] )
{
    printf( "%.3f: Replay playback started: \"%s\"\n", global.timeBase.time, filename );

    Stop();

    // todo: open file for reading

    // todo: verify file header

    m_internal->playback = true;
}

void ReplayManager::Stop()
{
    if ( m_internal->recording )
        printf( "%.3f: Recording stopped\n", global.timeBase.time );
    else if ( m_internal->playback )
        printf( "%.3f: Playback stopped\n", global.timeBase.time );
    else
        return;

    if ( m_internal->file )
    {
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

void ReplayManager::RecordUpdate( float deltaTime )
{
    if ( !IsRecording() )
        return;

    ReplayUpdate message;
    message.deltaTime = deltaTime;

    RecordMessage( REPLAY_UPDATE, message );
}

void ReplayManager::RecordMessage( int type, protocol::Object & message )
{
    CORE_ASSERT( type >= 0 );
    CORE_ASSERT( type < REPLAY_NUM_MESSAGES );
    CORE_ASSERT( IsRecording() );
    CORE_ASSERT( m_internal->file );

    uint8_t buffer[8*1024];
    protocol::WriteStream stream( buffer, sizeof( buffer ) );
    message.SerializeWrite( stream );
    if ( stream.IsOverflow() )
        return;

    fwrite( &type, sizeof(type), 1, m_internal->file );

    int bytes = stream.GetBytesWritten();
    fwrite( &bytes, sizeof(bytes), 1, m_internal->file );

    fwrite( buffer, bytes, 1, m_internal->file );
}

#endif // #ifdef CLIENT
