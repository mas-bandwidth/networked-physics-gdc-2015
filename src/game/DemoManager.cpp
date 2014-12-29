#include "DemoManager.h"

#ifdef CLIENT

#include "Demo.h"
#include "StoneDemo.h"
#include "CubesDemo.h"
#include "LockstepDemo.h"
#include "SnapshotDemo.h"
#include "Render.h"
#include "Global.h"
#include "Console.h"
#include "core/Core.h"
#include "core/Memory.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <GLFW/glfw3.h>

DemoManager::DemoManager( core::Allocator & allocator )
{
    m_allocator = &allocator;
    m_demo = nullptr;
}

DemoManager::~DemoManager()
{
    UnloadDemo();
}

bool DemoManager::LoadDemo( const char * name )
{
    printf( "%.3f: Loading demo \"%s\"\n", global.timeBase.time, name );

    UnloadDemo();

    if ( strcmp( name, "stone" ) == 0 )
    {
        m_demo = CORE_NEW( *m_allocator, StoneDemo, *m_allocator );
    }
    else if ( strcmp( name, "cubes" ) == 0 )
    {
        m_demo = CORE_NEW( *m_allocator, CubesDemo, *m_allocator );
    }
    else if ( strcmp( name, "lockstep" ) == 0 )
    {
        m_demo = CORE_NEW( *m_allocator, LockstepDemo, *m_allocator );
    }
    else if ( strcmp( name, "snapshot" ) == 0 )
    {
        m_demo = CORE_NEW( *m_allocator, SnapshotDemo, *m_allocator );
    }
    else
    {
        printf( "%.3f: error - demo \"%s\" not found\n", global.timeBase.time, name );
    }

    if ( m_demo )
    {
        if ( m_demo->Initialize() )
            return true;

        printf( "%.3f: error - demo \"%s\" failed to initialize\n", global.timeBase.time, name );

        UnloadDemo();
    }

    return false;
}

void DemoManager::UnloadDemo()
{
    if ( m_demo )
    {
        CORE_DELETE( *m_allocator, Demo, m_demo );
        m_demo = nullptr;
    }
}

bool DemoManager::ReloadDemo()
{
    if ( !m_demo )
        return false;

    const char * demo_name = m_demo->GetName();          // IMPORTANT: Assumes demo get name returns const char* literal

    UnloadDemo();

    return LoadDemo( demo_name );
}

Demo * DemoManager::GetDemo()
{
    return m_demo;
}

extern void console_function_reload( const char * args );

bool DemoManager::KeyEvent( int key, int scancode, int action, int mods )
{
    if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && mods == 0 )
    {
        console_function_reload( "" );
        return true;
    }

    if ( m_demo )
        return m_demo->KeyEvent( key, scancode, action, mods );
    else
        return false;
}

bool DemoManager::CharEvent( unsigned int code )
{
    if ( m_demo )
        return m_demo->CharEvent( code );
    else
        return false;
}

CONSOLE_FUNCTION( load )
{
    CORE_ASSERT( global.demoManager );

    global.demoManager->LoadDemo( args );
}

CONSOLE_FUNCTION( unload )
{
    CORE_ASSERT( global.demoManager );

    global.demoManager->UnloadDemo();
}

#endif // #ifdef CLIENT
