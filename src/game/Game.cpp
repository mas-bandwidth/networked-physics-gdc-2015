#include "Common.h"
#include "Network.h"
#include "Global.h"
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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using glm::mat4;
using glm::vec3;
using glm::vec4;

using namespace protocol;

GameClient * client = nullptr;

GLuint vboHandles[2];
GLuint vaoHandle;

static void game_init()
{
    // todo: use allocator new instead of "new"
    global.fontManager = new FontManager( memory::default_allocator() );
    global.shaderManager = new ShaderManager( memory::default_allocator() );

    client = CreateGameClient( memory::default_allocator() );

    if ( !client )
    {
        printf( "%.2f: error: failed to create game client!\n", global.timeBase.time );
        exit( 1 );
    }

    printf( "%.2f: Started game client on port %d\n", global.timeBase.time, client->GetPort() );

    Address address( "::1" );
    address.SetPort( ServerPort );

    client->Connect( address );

    global.timeBase.deltaTime = 1.0 / TickRate;

    // ---------------------------------------------------

    glEnable( GL_FRAMEBUFFER_SRGB );

    glClearColor( 0.25, 0.25, 0.25, 0.0 );

    float positionData[] = 
    {
       -0.8f, -0.8f, 0.0f,
        0.8f, -0.8f, 0.0f,
        0.0f,  0.8f, 0.0f 
    };

    float colorData[] = 
    {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f 
    };

    glGenBuffers( 2, vboHandles );

    GLuint positionBufferHandle = vboHandles[0];
    GLuint colorBufferHandle = vboHandles[1];

    // Populate the position buffer
    glBindBuffer( GL_ARRAY_BUFFER, positionBufferHandle );
    glBufferData( GL_ARRAY_BUFFER, 9 * sizeof(float), positionData, GL_STATIC_DRAW );

    // Populate the color buffer
    glBindBuffer( GL_ARRAY_BUFFER, colorBufferHandle );
    glBufferData( GL_ARRAY_BUFFER, 9 * sizeof(float), colorData, GL_STATIC_DRAW );

    // Create and set-up the vertex array object
    glGenVertexArrays( 1, &vaoHandle );
    glBindVertexArray( vaoHandle );

    // Enable the vertex attribute arrays
    glEnableVertexAttribArray( 0 );  // Vertex position
    glEnableVertexAttribArray( 1 );  // Vertex color

    // Map index 0 to the position buffer
    glBindBuffer( GL_ARRAY_BUFFER, positionBufferHandle );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL );

    // Map index 1 to the color buffer
    glBindBuffer( GL_ARRAY_BUFFER, colorBufferHandle );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL );

    check_opengl_error( "after vertex buffer setup" );
}

static void game_update()
{
    client->Update( global.timeBase );

    global.timeBase.time += global.timeBase.deltaTime;
}

static void game_render()
{
    check_opengl_error( "before render" );

    client->Update( global.timeBase );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    if ( ( rand() % 100 ) == 0 )
    {
        global.fontManager->Reload();
        global.shaderManager->Reload();
    }

    // ---------------------

    GLuint shader_program = global.shaderManager->GetShader( "Triangle" );

    glUseProgram( shader_program );

    glBindAttribLocation( shader_program, 0, "VertexPosition" );
    glBindAttribLocation( shader_program, 1, "VertexColor" );

    mat4 rotationMatrix = glm::rotate( mat4(1.0f), (float)global.timeBase.time * 4, vec3(0.0f,0.0f,1.0f) );

    int location = glGetUniformLocation( shader_program, "RotationMatrix" );
    if ( location >= 0 )
        glUniformMatrix4fv( location, 1, GL_FALSE, &rotationMatrix[0][0] );

    glBindVertexArray( vaoHandle );

    glDrawArrays( GL_TRIANGLES, 0, 3 );

    // --------------------

    Font * font = global.fontManager->GetFont( "AnonymousPro" );
    if ( font )
        font->DrawString( 10, 200, "Hello" );

    check_opengl_error( "after render" );
}

static void game_shutdown()
{
    DestroyGameClient( memory::default_allocator(), client );

    delete global.fontManager;
    delete global.shaderManager;

    global = Global();
}

void framebuffer_size_callback( GLFWwindow* window, int width, int height )
{
    glViewport( 0, 0, width, height );
}

int main( int argc, char * argv[] )
{
    srand( time( nullptr ) );

    memory::initialize();

    if ( !InitializeNetwork() )
    {
        printf( "%.2f: Failed to initialize network!\n", global.timeBase.time );
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
    glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE );

    GLFWwindow * window = glfwCreateWindow( 1200, 800, "Client", nullptr, nullptr );
    
    //GLFWwindow* window = glfwCreateWindow(800, 600, "Client", glfwGetPrimaryMonitor(), nullptr); // Fullscreen    

    glfwSetFramebufferSizeCallback( window, framebuffer_size_callback );

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

    // IMPORTANT: Disabled until we fix leak issue with game client/server objects in config
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

    global.timeBase.deltaTime = 1.0 / TickRate;

    memory::initialize();
    
    if ( !InitializeNetwork() )
    {
        printf( "%.2f: Failed to initialize network!\n", global.timeBase.time );
        return 1;
    }

    auto server = CreateGameServer( memory::default_allocator(), ServerPort, MaxClients );

    if ( !server )
    {
        printf( "%.2f: Failed to not create server on port %d\n", global.timeBase.time, ServerPort );
        return 1;
    }
    
    printf( "%.2f: Started game server on port %d\n", global.timeBase.time, ServerPort );

    while ( true )
    {
        // ...

        // todo: rather than sleeping for MS we want a signal that comes in every n millis instead
        // so we maintain a steady tick rate. how best to do this on linux, mac and windows respectively?

        // todo: want to detect the CTRL^C signal and actually break outta here

        server->Update( global.timeBase );

        sleep_milliseconds( global.timeBase.deltaTime * 1000 );

        global.timeBase.time += global.timeBase.deltaTime;
    }

    printf( "%.2f: Shutting down game server\n", global.timeBase.time );

    DestroyGameServer( memory::default_allocator(), server );

    ShutdownNetwork();

    memory::shutdown();

    return 0;
}

// ===================================================================================================================

#endif
