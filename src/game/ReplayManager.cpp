#include "ReplayManager.h"

#ifdef CLIENT

ReplayManager::ReplayManager( core::Allocator & allocator )
{
    m_allocator = &allocator;
    m_recording = false;
    m_playback = false;
}

ReplayManager::~ReplayManager()
{
    // ...
}


void ReplayManager::StartRecording( const char filename[] )
{
    // todo: open file etc.
    m_recording = true;
}

void ReplayManager::StartPlayback( const char filename[] )
{
    // todo: load file
    m_playback = true;
}

bool ReplayManager::IsRecording() const
{
    return m_recording;
}

bool ReplayManager::IsPlayback() const
{
    return m_playback;
}

void ReplayManager::RecordRandomSeed( unsigned int seed )
{
    if ( !IsRecording() ) 
        return;

    // todo: record random seed
}

void ReplayManager::RecordCommandLine( const char * commandLine )
{
    if ( !IsRecording() )
        return;

    // todo: record command line strings
}

void ReplayManager::RecordKeyEvent( int key, int scancode, int action, int mods )
{
    if ( !IsRecording() )
        return;

    // todo: record key event data
}

void ReplayManager::RecordCharEvent( unsigned int code )
{
    if ( !IsRecording() )
        return;

    // todo: record char event
}

void ReplayManager::RecordUpdate( float deltaTime )
{
    if ( !IsRecording() )
        return;

    // todo: record update delta time
}

#endif // #ifdef CLIENT
