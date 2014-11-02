#ifndef GAME_CONSOLE_H
#define GAME_CONSOLE_H

namespace core { class Allocator; }

struct ConsoleInternal;

typedef void (*ConsoleFunction)( const char * args );

void RegisterConsoleFunction( const char * name, ConsoleFunction function );

struct ConsoleFunctionHelper
{
    ConsoleFunctionHelper( const char * name, ConsoleFunction function )
    {
        RegisterConsoleFunction( name, function );
    }
};

#define CONSOLE_FUNCTION( name )    void console_function_##name( const char * args ); static ConsoleFunctionHelper s_##name##_function_helper( #name, console_function_##name ); void console_function_##name( const char * args ) 

class Console
{
public:

    Console( core::Allocator & allocator );

    ~Console();

    bool KeyEvent( int key, int scancode, int action, int mods );

    bool CharEvent( unsigned int code );

    void ExecuteCommand( const char * string );

    bool IsActive() const;

    void Activate();

    #ifdef CLIENT
    void Render();
    #endif // #ifdef CLIENT

private:

    core::Allocator * m_allocator;
    ConsoleInternal * m_internal;
};

#endif // #ifndef GAME_CONSOLE_H
