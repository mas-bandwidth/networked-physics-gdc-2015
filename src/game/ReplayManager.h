#ifdef CLIENT

#ifndef REPLAY_MANAGER_H
#define REPLAY_MANAGER_H

namespace core { class Allocator; }

namespace protocol { class Object; }

struct ReplayInternal;

class ReplayManager
{
public:

    ReplayManager( core::Allocator & allocator );

    ~ReplayManager();

    void StartRecording( const char filename[] );

    void StartPlayback( const char filename[] );

    bool IsRecording() const;

    bool IsPlayback() const;

    void Stop();

    void RecordRandomSeed( unsigned int seed );

    void RecordCommandLine( const char * commandLine );

    void RecordKeyEvent( int key, int scancode, int action, int mods );

    void RecordCharEvent( unsigned int code );

    void RecordUpdate();

    void UpdatePlayback();

    void UpdateCapture();

private:

    void RecordMessage( int type, protocol::Object & object );

    ReplayInternal * m_internal;

    core::Allocator * m_allocator;
};

#endif // #ifndef GAME_REPLAY_MANAGER_H

#endif // #ifdef CLIENT
