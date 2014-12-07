#include "InputManager.h"

#ifdef CLIENT

#include "DemoManager.h"
#include "ReplayManager.h"
#include "Console.h"
#include "Global.h"

InputManager::InputManager( core::Allocator & allocator )
{
    m_allocator = &allocator;
}

InputManager::~InputManager()
{
    // ...
}

void InputManager::KeyEvent( int key, int scancode, int action, int mods )
{
    global.replayManager->RecordKeyEvent( key, scancode, action, mods );

    if ( global.console->KeyEvent( key, scancode, action, mods ) )
        return;

    if ( global.demoManager->KeyEvent( key, scancode, action, mods ) )
        return;

    // ...
}

void InputManager::CharEvent( unsigned int code )
{
    global.replayManager->RecordCharEvent( code );

    if ( global.console->CharEvent( code ) )
        return;

    if ( global.demoManager->CharEvent( code ) )
        return;

    // ...
}

#endif // #ifdef CLIENT
