#ifndef GAME_CONSOLE_H
#define GAME_CONSOLE_H

namespace core { class Allocator; }

class Console
{
public:

    Console( core::Allocator & allocator );

    ~Console();

    bool KeyEvent( int key, int scancode, int action, int mods );

    bool CharEvent( unsigned int code );

    void Render();

    void ExecuteCommand( const char * string );

    bool IsActive() const { return m_active; }

private:

    void ClearCommandString();

    void Backspace();

    core::Allocator * m_allocator;
    bool m_active;
    bool m_justActivated;
    bool m_justDeactivated;
    int m_commandLength;
    char m_commandString[1024];
};

#endif // #ifndef GAME_CONSOLE_H
