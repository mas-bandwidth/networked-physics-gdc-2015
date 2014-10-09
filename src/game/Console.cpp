#include "Console.h"
#include "Global.h"
#include "Font.h"
#include "FontManager.h"
#include "core/Core.h"
#include "core/Memory.h"
#include <GLFW/glfw3.h>

Console::Console( core::Allocator & allocator )
{
    m_allocator = &allocator;
    m_active = false;
    m_justActivated = false;
    m_justDeactivated = false;
    memset( m_commandString, 0, sizeof( m_commandString ) );
    m_commandLength = 0;
}

Console::~Console()
{
    // ...
}

bool Console::KeyEvent( int key, int scancode, int action, int mods )
{
    if ( !m_active )
    {
        if ( key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS )
        {
            m_active = true;
            m_justActivated = true;
            return true;
        }

        return false;
    }
    else
    {
        if ( ( key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER ) && action == GLFW_PRESS )
        {
            ExecuteCommand( m_commandString );
            ClearCommandString();
        }
        else if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
        {
            ClearCommandString();
        }
        else if ( key == GLFW_KEY_BACKSPACE && ( action == GLFW_PRESS || action == GLFW_REPEAT ) )
        {
            Backspace();
        }
        else if ( key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS )
        {
            ClearCommandString();
            m_active = false;
            m_justDeactivated = true;
        }
    }

    return true;
}

bool Console::CharEvent( unsigned int code )
{
    if ( m_justDeactivated )
    {
        m_justDeactivated = false;
        return true;
    }

    if ( !IsActive() )
        return false;

    if ( code >= 255 )
        return false;

    if ( m_justActivated )
    {
        m_justActivated = false;
        return true;
    }

    if ( m_commandLength < sizeof( m_commandString ) - 1 )
        m_commandString[m_commandLength++] = (char) code;

    return true;
}

void Console::Render()
{
    if ( !m_active )
        return;

    if ( m_commandString[0] == '\0' )
        return;

    Font * font = global.fontManager->GetFont( "Console" );
    if ( !font )
        return;

    font->Begin();
    font->DrawText( 5, 2, m_commandString, Color(0,0,0,1.0) );
    font->End();
}

void Console::ExecuteCommand( const char * string )
{
    if ( strcmp( string, "quit" ) == 0 ||
         strcmp( string, "exit" ) == 0 )
        global.quit = true;
}

void Console::ClearCommandString()
{
    m_commandLength = 0;
    memset( m_commandString, 0, sizeof( m_commandString ) );
}

void Console::Backspace()
{
    if ( m_commandLength > 0 )
        m_commandString[--m_commandLength] = '\0';
}
