#ifndef GAME_CONSOLE_H
#define GAME_CONSOLE_H

namespace core { class Allocator; }

struct ConsoleInternal;

class Console
{
public:

    Console( core::Allocator & allocator );

    ~Console();

    bool KeyEvent( int key, int scancode, int action, int mods );

    bool CharEvent( unsigned int code );

    void Render();

    void ExecuteCommand( const char * string );

    bool IsActive() const;

private:

    core::Allocator * m_allocator;

    ConsoleInternal * m_internal;
};

#endif // #ifndef GAME_CONSOLE_H
