#include "Console.h"
#include "Global.h"
#include "Font.h"
#include "FontManager.h"
#include "core/Core.h"
#include "core/Memory.h"
#include <GLFW/glfw3.h>

static const int MaxLine = 1024;
static const int CommandHistorySize = 256;

struct ConsoleInternal
{
    ConsoleInternal()
    {
        active = false;
        justActivated = false;
        justDeactivated = false;

        commandHistoryIsEmpty = true;
        commandLength = 0;
        commandHistoryIndex = 0;
        commandHistorySelection = -1;
        memset( commandString, 0, sizeof( commandString ) );
        memset( commandHistory, 0, sizeof( commandHistory ) );
    }

    bool active;
    bool justActivated;
    bool justDeactivated;

    bool commandHistoryIsEmpty;
    int commandLength;                  // todo: this is really the "cursor" position
    int commandHistoryIndex;
    int commandHistorySelection;
    char commandString[MaxLine];
    char commandHistory[CommandHistorySize][MaxLine];

    void ClearCommandString()
    {
        commandLength = 0;
        memset( commandString, 0, sizeof( commandString ) );
        commandHistorySelection = -1;
    }

    void Backspace()
    {
        if ( commandLength > 0 )
            commandString[--commandLength] = '\0';
    }

    void AddToCommandHistory( const char * string )
    {
        strncpy_safe( &commandHistory[commandHistoryIndex][0], string, MaxLine );
        commandHistoryIndex++;
        commandHistoryIndex %= CommandHistorySize;
        commandHistoryIsEmpty = false;
    }

    void NextCommandInHistory()
    {
        if ( commandHistoryIsEmpty )
            return;

        if ( commandHistorySelection == -1 )
            commandHistorySelection = commandHistoryIndex;

        for ( int i = 0; i < CommandHistorySize; ++i )
        {
            commandHistorySelection++;
            commandHistorySelection %= CommandHistorySize;

            if ( commandHistory[commandHistorySelection][0] != '\0' )
            {
                const char * command = &commandHistory[commandHistorySelection][0];
                memset( commandString, 0, sizeof( commandString ) );
                strncpy_safe( commandString, command, MaxLine );
                commandLength = strlen( commandString );
                return;
            }
        }
    }

    void PrevCommandInHistory()
    {
        if ( commandHistoryIsEmpty )
            return;

        if ( commandHistorySelection == -1 )
            commandHistorySelection = commandHistoryIndex;

        for ( int i = 0; i < CommandHistorySize; ++i )
        {
            commandHistorySelection--;
            if ( commandHistorySelection < 0 )
                commandHistorySelection += CommandHistorySize;

            if ( commandHistory[commandHistorySelection][0] != '\0' )
            {
                const char * command = &commandHistory[commandHistorySelection][0];
                memset( commandString, 0, sizeof( commandString ) );
                strncpy_safe( commandString, command, MaxLine );
                commandLength = strlen( commandString );
                return;
            }
        }
    }
};

Console::Console( core::Allocator & allocator )
{
    m_allocator = &allocator;
    m_internal = CORE_NEW( allocator, ConsoleInternal );
}

Console::~Console()
{
    CORE_DELETE( *m_allocator, ConsoleInternal, m_internal );
}

bool Console::KeyEvent( int key, int scancode, int action, int mods )
{
    if ( !IsActive() )
    {
        if ( key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS )
        {
            m_internal->active = true;
            m_internal->justActivated = true;
            return true;
        }

        return false;
    }
    else
    {
        if ( ( key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER ) && action == GLFW_PRESS )
        {
            ExecuteCommand( m_internal->commandString );
            m_internal->AddToCommandHistory( m_internal->commandString );
            m_internal->ClearCommandString();
        }
        else if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
        {
            m_internal->ClearCommandString();
        }
        else if ( key == GLFW_KEY_BACKSPACE && ( action == GLFW_PRESS || action == GLFW_REPEAT ) )
        {
            m_internal->Backspace();
        }
        else if ( key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS )
        {
            m_internal->ClearCommandString();
            m_internal->active = false;
            m_internal->justDeactivated = true;
        }
        else if ( key == GLFW_KEY_UP && ( action == GLFW_PRESS || action == GLFW_REPEAT ) )
        {
            m_internal->PrevCommandInHistory();
        }
        else if ( key == GLFW_KEY_DOWN && ( action == GLFW_PRESS || action == GLFW_REPEAT ) )
        {
            m_internal->NextCommandInHistory();
        }
    }

    return true;
}

bool Console::CharEvent( unsigned int code )
{
    if ( m_internal->justDeactivated )
    {
        m_internal->justDeactivated = false;
        return true;
    }

    if ( !IsActive() )
        return false;

    if ( code >= 255 )
        return false;

    if ( m_internal->justActivated )
    {
        m_internal->justActivated = false;
        return true;
    }

    if ( m_internal->commandLength < sizeof( m_internal->commandString ) - 1 )
        m_internal->commandString[m_internal->commandLength++] = (char) code;

    return true;
}

void Console::Render()
{
    if ( !m_internal->active )
        return;

    if ( m_internal->commandString[0] == '\0' )
        return;

    Font * font = global.fontManager->GetFont( "Console" );
    if ( !font )
        return;

    font->Begin();
    font->DrawText( 5, 2, m_internal->commandString, Color(0,0,0,1.0) );
    font->End();
}

void Console::ExecuteCommand( const char * string )
{
    if ( strcmp( string, "quit" ) == 0 || strcmp( string, "exit" ) == 0 )
        global.quit = true;

    // ...
}

bool Console::IsActive() const
{
    return m_internal->active;
}
