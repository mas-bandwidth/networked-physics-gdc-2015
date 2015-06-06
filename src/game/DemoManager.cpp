#include "DemoManager.h"

#ifdef CLIENT

#include "Demo.h"
#include "StoneDemo.h"
#include "CubesDemo.h"
#include "LockstepDemo.h"
#include "SnapshotDemo.h"
#include "CompressionDemo.h"
#include "DeltaDemo.h"
#include "SyncDemo.h"
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
    else if ( strcmp( name, "compression" ) == 0 )
    {
        m_demo = CORE_NEW( *m_allocator, CompressionDemo, *m_allocator );
    }
    else if ( strcmp( name, "delta" ) == 0 )
    {
        m_demo = CORE_NEW( *m_allocator, DeltaDemo, *m_allocator );
    }
    else if ( strcmp( name, "sync" ) == 0 )
    {
        m_demo = CORE_NEW( *m_allocator, SyncDemo, *m_allocator );
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

    printf( "reload: %s\n", demo_name );

    UnloadDemo();

    return LoadDemo( demo_name );
}

void DemoManager::ResetDemo()
{
    if ( m_demo )
    {
        m_demo->Shutdown();
        m_demo->Initialize();
    }
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

    if ( key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS && mods == 0 )
    {
        if ( m_demo )
        {
            ResetDemo();
            return true;
        }
    }

    if ( m_demo && action == GLFW_PRESS && mods == 0 )
    {
        const int num_modes = m_demo->GetNumModes();

        if ( key == GLFW_KEY_1 )
        {
            if ( num_modes >= 1 )
            {
                m_demo->SetMode( 0 );
                ResetDemo();
            }
        }
        else if ( key == GLFW_KEY_2 )
        {
            if ( num_modes >= 2 )
            {
                m_demo->SetMode( 1 );
                ResetDemo();
            }
        }
        else if ( key == GLFW_KEY_3 )
        {
            if ( num_modes >= 3 )
            {
                m_demo->SetMode( 2 );
                ResetDemo();
            }
        }
        else if ( key == GLFW_KEY_4 )
        {
            if ( num_modes >= 4 )
            {
                m_demo->SetMode( 3 );
                ResetDemo();
            }
        }
        else if ( key == GLFW_KEY_5 )
        {
            if ( num_modes >= 5 )
            {
                m_demo->SetMode( 4 );
                ResetDemo();
            }
        }
        else if ( key == GLFW_KEY_6 )
        {
            if ( num_modes >= 6 )
            {
                m_demo->SetMode( 5 );
                ResetDemo();
            }
        }
        else if ( key == GLFW_KEY_7 )
        {
            if ( num_modes >= 7 )
            {
                m_demo->SetMode( 6 );
                ResetDemo();
            }
        }
        else if ( key == GLFW_KEY_8 )
        {
            if ( num_modes >= 8 )
            {
                m_demo->SetMode( 7 );
                ResetDemo();
            }
        }
        else if ( key == GLFW_KEY_9 )
        {
            if ( num_modes >= 9 )
            {
                m_demo->SetMode( 8 );
                ResetDemo();
            }
        }
        else if ( key == GLFW_KEY_0 )
        {
            if ( num_modes >= 10 )
            {
                m_demo->SetMode( 9 );
                ResetDemo();
            }
        }
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
