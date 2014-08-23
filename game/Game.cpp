#include "Common.h"
#include "Network.h"
#include <time.h>
#include <stdlib.h>
#include "GameClient.h"
#include "BSDSocket.h"
#include "NetworkSimulator.h"

#ifdef CLIENT
#include <GL/glew.h>
#include <GLUT/glut.h>
#endif

using namespace protocol;

#ifdef CLIENT

void init()
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
}

void display()
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glLoadIdentity();

    glBegin( GL_TRIANGLES );
        glVertex3f(0.0, 0.0, -10.0);
        glVertex3f(1.0, 0.0, -10.0);
        glVertex3f(0.0, 1.0, -10.0);
    glEnd();

    glutSwapBuffers();
}

void reshape( int w, int h )
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.1, 0.1, -float(h)/(10.0*float(w)), float(h)/(10.0*float(w)), 0.5, 1000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main( int argc, char ** argv )
{
    srand( time( nullptr ) );

    if ( !InitializeNetwork() )
    {
        printf( "failed to initialize network\n" );
        return 1;
    }

    PROTOCOL_ASSERT( IsNetworkInitialized() );

    glutInit( &argc, argv );

/*
    GLenum err = glewInit();
    if ( GLEW_OK != err )
    {
        printf( "error: glewInit failed\n" );
        return 1;
    }
*/

    glutInitDisplayMode( GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA );

    glutInitWindowSize( 1000, 600 );

    glutInitWindowPosition( 220, 150 );

    glutCreateWindow( "Client" );

    // passes reshape and display functions to the OpenGL machine for callback
    glutReshapeFunc( reshape );
    glutDisplayFunc( display );
    glutIdleFunc( display );

    init();

    // Starts the program.
    glutMainLoop();

    ShutdownNetwork();

    return 0;
}

#else

#include "GameServer.h"
#include "GamePackets.h"
#include "GameMessages.h"
#include "GameChannelStructure.h"

struct ServerInfo
{
    Address address;
    GameServer * server;
    Block * serverData;
    NetworkInterface * networkInterface;
    NetworkSimulator * networkSimulator;
};

int main( int argc, char ** argv )
{
    srand( time( nullptr ) );

    memory::initialize();
    {
        if ( !InitializeNetwork() )
        {
            printf( "Failed to initialize network!\n" );
            return 1;
        }

        // todo: move all these guys into the game server derived class -- make it easy to create

        GameMessageFactory messageFactory( memory::default_allocator() );

        GameChannelStructure channelStructure( messageFactory );

        GamePacketFactory packetFactory( memory::default_allocator() );

        const int ServerPort = 10000;
        const int MaxClients = 16;
        const float TickRate = 60;

        ServerInfo serverInfo;   

        serverInfo.address = Address( "::1" );
        serverInfo.address.SetPort( ServerPort );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = ServerPort;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;
        serverInfo.networkInterface = PROTOCOL_NEW( memory::default_allocator(), BSDSocket, bsdSocketConfig );

        NetworkSimulatorConfig networkSimulatorConfig;
        networkSimulatorConfig.packetFactory = &packetFactory;
        serverInfo.networkSimulator = PROTOCOL_NEW( memory::default_allocator(), NetworkSimulator, networkSimulatorConfig );

        const int serverDataSize = sizeof(GameContext) + 10 * 1024 + 11;
        serverInfo.serverData = PROTOCOL_NEW( memory::default_allocator(), Block, memory::default_allocator(), serverDataSize );
        {
            uint8_t * data = serverInfo.serverData->GetData();
            for ( int i = 0; i < serverDataSize; ++i )
                data[i] = ( 10 + i ) % 256;

            auto gameContext = (GameContext*) data;
            gameContext->value_min = -1 - ( rand() % 100000000 );
            gameContext->value_max = rand() % 1000000000;
        }

        ServerConfig serverConfig;
        serverConfig.serverData = serverInfo.serverData;
        serverConfig.maxClients = MaxClients;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverInfo.networkInterface;
        serverConfig.networkSimulator = serverInfo.networkSimulator;

        // todo: end stuff to move inside game server class

        serverInfo.server = PROTOCOL_NEW( memory::default_allocator(), GameServer, serverConfig );

        printf( "Started game server on port %d\n", ServerPort );

        TimeBase timeBase;
        timeBase.deltaTime = 1.0 / TickRate;

        while ( true )
        {
            // ...

            // todo: rather than sleeping for MS we want a signal that comes in every n millis instead
            // so we maintain a steady tick rate. how best to do this on linux and mac? (need both...)

            sleep_milliseconds( timeBase.deltaTime * 1000 );

            timeBase.time += timeBase.deltaTime;
        }

        ShutdownNetwork();
    }

    memory::shutdown();

    return 0;
}

#endif
