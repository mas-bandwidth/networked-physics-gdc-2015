#include "Common.h"
#include "Network.h"
#include <time.h>
#include <stdlib.h>

const float TickRate = 60;
const int ServerPort = 10000;

#ifdef CLIENT

// ===================================================================================================================
//                                                       CLIENT
// ===================================================================================================================

#include <GL/glew.h>
#include <GLUT/glut.h>
#include "GameClient.h"
#include "BSDSocket.h"
#include "NetworkSimulator.h"

using namespace protocol;

GameClient * client = nullptr;

TimeBase timeBase;

void init()
{
    glClearColor( 0.5, 0.5, 0.5, 0.0 );
    glEnable( GL_DEPTH_TEST );
    glShadeModel( GL_SMOOTH );
}

void display()
{
    client->Update( timeBase );

    if ( client->HasError() )
        exit( 1 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    glLoadIdentity();

    /*
    glBegin( GL_TRIANGLES );
        glVertex3f( 0.0, 0.0, -10.0 );
        glVertex3f( 1.0, 0.0, -10.0 );
        glVertex3f( 0.0, 1.0, -10.0 );
    glEnd();
    */

    glutSwapBuffers();

    timeBase.time += timeBase.deltaTime;
}

void reshape( int w, int h )
{
    glViewport( 0, 0, w, h );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glFrustum( -0.1, 0.1, -float(h)/(10.0*float(w)), float(h)/(10.0*float(w)), 0.5, 1000.0 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
}

int main( int argc, char ** argv )
{
    srand( time( nullptr ) );

    memory::initialize();

    if ( !InitializeNetwork() )
    {
        printf( "%.2f: Failed to initialize network!\n", timeBase.time );
        return 1;
    }

    PROTOCOL_ASSERT( IsNetworkInitialized() );

    glutInit( &argc, argv );

    glutInitDisplayMode( GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA );

    glutInitWindowSize( 1000, 600 );

    glutInitWindowPosition( 220, 150 );

    glutCreateWindow( "Client" );

    glutReshapeFunc( reshape );
    glutDisplayFunc( display );
    glutIdleFunc( display );

    init();

    client = CreateGameClient( memory::default_allocator() );

    if ( !client )
    {
        printf( "%.2f: Failed to create game client!\n", timeBase.time );
        return 1;
    }

    printf( "%.2f: Started game client on port %d\n", timeBase.time, client->GetPort() );

    Address address( "::1" );
    address.SetPort( ServerPort );

    client->Connect( address );


    timeBase.deltaTime = 1.0 / TickRate;

    glutMainLoop();

    DestroyGameClient( memory::default_allocator(), client );

    ShutdownNetwork();

    memory::shutdown();

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

    TimeBase timeBase;

    timeBase.deltaTime = 1.0 / TickRate;

    memory::initialize();
    
    if ( !InitializeNetwork() )
    {
        printf( "%.2f: Failed to initialize network!\n", timeBase.time );
        return 1;
    }

    auto server = CreateGameServer( memory::default_allocator(), ServerPort, MaxClients );

    if ( !server )
    {
        printf( "%.2f: Failed to not create server on port %d\n", timeBase.time, ServerPort );
        return 1;
    }
    
    printf( "%.2f: Started game server on port %d\n", timeBase.time, ServerPort );

    while ( true )
    {
        // ...

        // todo: rather than sleeping for MS we want a signal that comes in every n millis instead
        // so we maintain a steady tick rate. how best to do this on linux and mac? (need both...)

        // todo: want to detect the CTRL^C signal and actually break outta here

        server->Update( timeBase );

        sleep_milliseconds( timeBase.deltaTime * 1000 );

        timeBase.time += timeBase.deltaTime;
    }

    printf( "%.2f: Shutting down game server\n", timeBase.time );

    DestroyGameServer( memory::default_allocator(), server );

    ShutdownNetwork();

    memory::shutdown();

    return 0;
}

// ===================================================================================================================

#endif
