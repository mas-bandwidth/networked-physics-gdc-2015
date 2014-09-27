#include "Common.h"
#include "Network.h"
#include "Globals.h"
#include <time.h>
#include <stdlib.h>

const float TickRate = 60;
const int ServerPort = 10000;

#ifdef CLIENT

// ===================================================================================================================
//                                                       CLIENT
// ===================================================================================================================

#include "Font.h"
#include "Render.h"
#include "GameClient.h"
#include "BSDSocket.h"
#include "NetworkSimulator.h"
#include "ShaderManager.h"
#include "FontManager.h"

using namespace protocol;

GameClient * client = nullptr;

Font * font = nullptr;

FontManager * fontManager = nullptr;
ShaderManager * shaderManager = nullptr;

GLuint vertex_array = 0;

static void game_init()
{
    shaderManager = new ShaderManager( memory::default_allocator() );

    float vertices[] = 
    {
        0.0f,  0.5f,  0.0f,
        0.5f, -0.5f,  0.0f,
       -0.5f, -0.5f,  0.0f
    };

    GLuint vbo = 0;
    glGenBuffers( 1, &vbo );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBufferData( GL_ARRAY_BUFFER, 9 * sizeof (float), vertices, GL_STATIC_DRAW );

    glGenVertexArrays( 1, &vertex_array );
    glBindVertexArray( vertex_array );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );

    glEnable( GL_FRAMEBUFFER_SRGB );

    glClearColor( 0.25, 0.25, 0.25, 0.0 );

    check_opengl_error( "before font load" );

    {
        // todo: create the font with allocator
        const char font_filename[] = "data/fonts/Console.font";
        font = new Font( font_filename );
    }

    fontManager = new FontManager();

    check_opengl_error( "after font load" );

    client = CreateGameClient( memory::default_allocator() );

    if ( !client )
    {
        printf( "%.2f: error: failed to create game client!\n", globals.timeBase.time );
        exit( 1 );
    }

    printf( "%.2f: Started game client on port %d\n", globals.timeBase.time, client->GetPort() );

    Address address( "::1" );
    address.SetPort( ServerPort );

    client->Connect( address );

    globals.timeBase.deltaTime = 1.0 / TickRate;
}

static void game_update()
{
    client->Update( globals.timeBase );

    globals.timeBase.time += globals.timeBase.deltaTime;
}

static void game_render()
{
    check_opengl_error( "before render" );

    client->Update( globals.timeBase );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

/*
    if ( ( rand() % 100 ) == 0 )
        shaderManager->Reload();
*/

    GLuint shader_program = shaderManager->GetShader( "Triangle" );

    glUseProgram( shader_program );
    glBindVertexArray( vertex_array );
    glDrawArrays( GL_TRIANGLES, 0, 3 );

    /*
    font->DrawString( 10, 200, "Hello my baby. Hello my darling. Hello my ragtime doll" );
    */

    check_opengl_error( "after render" );
}

static void game_shutdown()
{
    DestroyGameClient( memory::default_allocator(), client );

    delete font;

    delete fontManager;
    delete shaderManager;
}

int main( int argc, char * argv[] )
{
    srand( time( nullptr ) );

    memory::initialize();

    if ( !InitializeNetwork() )
    {
        printf( "%.2f: Failed to initialize network!\n", globals.timeBase.time );
        return 1;
    }

    PROTOCOL_ASSERT( IsNetworkInitialized() );

    glfwInit();

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_SRGB_CAPABLE, GL_TRUE );
    glfwWindowHint( GLFW_RESIZABLE, GL_TRUE );

    GLFWwindow * window = glfwCreateWindow( 1200, 800, "Client", nullptr, nullptr );
    
    //GLFWwindow* window = glfwCreateWindow(800, 600, "Client", glfwGetPrimaryMonitor(), nullptr); // Fullscreen    

    glfwMakeContextCurrent( window );

    glewExperimental = GL_TRUE;
    glewInit();

    clear_opengl_error();

    if ( !GLEW_VERSION_4_1 )
    {
        printf( "error: OpenGL 4.1 is not supported\n" );
        exit(1);
    }

    game_init();

    while ( !glfwWindowShouldClose( window ) )
    {
        glfwPollEvents();

        if ( glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS )
            glfwSetWindowShouldClose( window, GL_TRUE );

        game_update();

        game_render();

        glfwSwapBuffers( window );
    }

    game_shutdown();

    ShutdownNetwork();

    // IMPORTANT: Disabled until we fix leak issue with game client/server objects in config¡
    //memory::shutdown();

    glfwTerminate();
    
    return 0;
}

// ===================================================================================================================

#else

// ===================================================================================================================
//                                                       SERVER
// ===================================================================================================================

#include "GameServer.h"

const int MaxClients = 16;

int main( int argc, char ** argv )
{
    srand( time( nullptr ) );

    globals.timeBase.deltaTime = 1.0 / TickRate;

    memory::initialize();
    
    if ( !InitializeNetwork() )
    {
        printf( "%.2f: Failed to initialize network!\n", globals.timeBase.time );
        return 1;
    }

    auto server = CreateGameServer( memory::default_allocator(), ServerPort, MaxClients );

    if ( !server )
    {
        printf( "%.2f: Failed to not create server on port %d\n", globals.timeBase.time, ServerPort );
        return 1;
    }
    
    printf( "%.2f: Started game server on port %d\n", globals.timeBase.time, ServerPort );

    while ( true )
    {
        // ...

        // todo: rather than sleeping for MS we want a signal that comes in every n millis instead
        // so we maintain a steady tick rate. how best to do this on linux and mac? (need both...)

        // todo: want to detect the CTRL^C signal and actually break outta here

        server->Update( globals.timeBase );

        sleep_milliseconds( globals.timeBase.deltaTime * 1000 );

        globals.timeBase.time += globals.timeBase.deltaTime;
    }

    printf( "%.2f: Shutting down game server\n", globals.timeBase.time );

    DestroyGameServer( memory::default_allocator(), server );

    ShutdownNetwork();

    memory::shutdown();

    return 0;
}

// ===================================================================================================================

#endif
