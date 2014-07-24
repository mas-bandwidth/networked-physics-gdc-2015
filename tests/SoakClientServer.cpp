#include "Connection.h"
#include "TestMessages.h"
#include "TestPackets.h"
#include "TestChannelStructure.h"
#include "ReliableMessageChannel.h"
#include "NetworkSimulator.h"
#include "BSDSocket.h"
#include "Client.h"
#include "Server.h"
#include "Network.h"
#include <time.h>
#include <chrono>
#include <thread>

using namespace protocol;

struct ClientInfo
{
    Client * client;
    Block * clientData;
    NetworkInterface * networkInterface;
};

struct ServerInfo
{
    Address address;
    Server * server;
    Block * serverData;
    NetworkInterface * networkInterface;
};

void soak_test()
{
#if PROFILE
    printf( "[profile client server]\n" );
#else
    printf( "[soak client server]\n" );
#endif

    // configu

    const int NumServers = 8;
    const int NumClients = 256;
    const int NumClientsPerServer = 16;
    const int BaseServerPort = 10000;
    const int BaseClientPort = 20000;

    TestMessageFactory messageFactory( memory::default_allocator() );

    TestChannelStructure channelStructure( messageFactory );

    TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

    // create a bunch of servers starting at port 1000

    ServerInfo serverInfo[NumServers];

    for( int i = 0; i < NumServers; ++i )
    {
        serverInfo[i].address = Address( "::1" );
        serverInfo[i].address.SetPort( BaseServerPort + i );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = BaseServerPort + i;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;
        serverInfo[i].networkInterface = PROTOCOL_NEW( memory::default_allocator(), BSDSocket, bsdSocketConfig );

        const int serverDataSize = 10 + 256 * i + 11 + i;
        serverInfo[i].serverData = PROTOCOL_NEW( memory::default_allocator(), Block, memory::default_allocator(), serverDataSize );
        {
            uint8_t * data = serverInfo[i].serverData->GetData();
            for ( int j = 0; j < serverDataSize; ++j )
                data[j] = ( 10 + i + j ) % 256;
        }

        ServerConfig serverConfig;
        serverConfig.serverData = serverInfo[i].serverData;
        serverConfig.maxClients = NumClientsPerServer;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverInfo[i].networkInterface;

        serverInfo[i].server = PROTOCOL_NEW( memory::default_allocator(), Server, serverConfig );
    }

    ClientInfo clientInfo[NumClients];

    for( int i = 0; i < NumClients; ++i )
    {
        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = BaseClientPort + i;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;
        clientInfo[i].networkInterface = PROTOCOL_NEW( memory::default_allocator(), BSDSocket, bsdSocketConfig );

        const int clientDataSize = 10 + 64 * i + 21 + i;
        clientInfo[i].clientData = PROTOCOL_NEW( memory::default_allocator(), Block, memory::default_allocator(), clientDataSize );
        {
            uint8_t * data = clientInfo[i].clientData->GetData();
            for ( int j = 0; j < clientDataSize; ++j )
                data[j] = ( 20 + i + j ) % 256;
        }

        ClientConfig clientConfig;
        clientConfig.clientData = clientInfo[i].clientData;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = clientInfo[i].networkInterface;

        clientInfo[i].client = PROTOCOL_NEW( memory::default_allocator(), Client, clientConfig );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 1.0 / 60.0;

    while ( true )
    //for ( int iteration = 0; iteration < 1024; ++iteration )
    {
        for ( int i = 0; i < NumServers; ++i )
        {
            serverInfo[i].server->Update( timeBase );
        }

        for ( int i = 0; i < NumClients; ++i )
        {
            clientInfo[i].client->Update( timeBase );

            if ( clientInfo[i].client->IsConnected() )
            {
                if ( ( rand() % 100 ) == 0 )
                {
                    printf( "disconnect client %d\n", i );
                    clientInfo[i].client->Disconnect();
                }
            }

            if ( clientInfo[i].client->GetState() == CLIENT_STATE_DISCONNECTED )
            {
                if ( ( rand() % 200 ) == 0 )
                {
                    const int serverIndex = rand() % NumServers;
                    printf( "connect client %d to server %d\n", i, serverIndex );
                    clientInfo[i].client->Connect( serverInfo[serverIndex].address );                    
                }
            }
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

        timeBase.time += timeBase.deltaTime;
    }

    for ( int i = 0; i < NumServers; ++i )
    {
        PROTOCOL_DELETE( memory::default_allocator(), Server, serverInfo[i].server );
        PROTOCOL_DELETE( memory::default_allocator(), Block, serverInfo[i].serverData );
        PROTOCOL_DELETE( memory::default_allocator(), NetworkInterface, serverInfo[i].networkInterface );
    }

    for ( int i = 0; i < NumClients; ++i )
    {
        PROTOCOL_DELETE( memory::default_allocator(), Client, clientInfo[i].client );
        PROTOCOL_DELETE( memory::default_allocator(), Block, clientInfo[i].clientData );
        PROTOCOL_DELETE( memory::default_allocator(), NetworkInterface, clientInfo[i].networkInterface );
    }
}

int main()
{
    srand( time( nullptr ) );

    memory::initialize();

    srand( time( nullptr ) );

    if ( !InitializeNetwork() )
    {
        printf( "failed to initialize network\n" );
        return 1;
    }

    PROTOCOL_ASSERT( IsNetworkInitialized() );

    soak_test();

    ShutdownNetwork();

    memory::shutdown();

    return 0;
}
