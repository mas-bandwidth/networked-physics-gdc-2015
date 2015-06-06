#ifdef CLIENT

#include "GameClient.h"
#include "Console.h"
#include "Global.h"
#include "network/BSDSocket.h"
#include "network/Simulator.h"
#include "GameMessages.h"
#include "GameContext.h"
#include "GamePackets.h"
#include "GameMessages.h"
#include "GameChannelStructure.h"

GameClient::GameClient( const clientServer::ClientConfig & config ) : Client( config ) 
{
    // ...
}

const GameContext * GameClient::GetGameContext() const
{
    const protocol::Block * block = GetServerData();
    if ( block )
        return (const GameContext*) block->GetData();
    else
        return nullptr;
}

uint16_t GameClient::GetPort() const
{
    auto socket = (network::BSDSocket*) GetConfig().networkInterface;
    CORE_ASSERT( socket );
    return socket->GetPort();
}

double GameClient::GetTime() const
{
    return GetTimeBase().time;
}

void GameClient::OnConnect( const network::Address & address )
{
    char buffer[256];
    printf( "%.3f: Client connecting to %s\n", GetTime(), address.ToString( buffer, sizeof( buffer ) ) );
}

void GameClient::OnConnect( const char * hostname )
{
    printf( "%.3f: Client connecting to %s\n", GetTime(), hostname );
}

void GameClient::OnStateChange( clientServer::ClientState previous, clientServer::ClientState current )
{
    printf( "%.3f: Client state change: %s -> %s\n", GetTime(), GetClientStateName( previous ), GetClientStateName( current ) );
}

void GameClient::OnDisconnect()
{
    printf( "%.3f: Client disconnect\n", GetTime() );
}

void GameClient::OnError( clientServer::ClientError error, uint32_t extendedError )
{
    printf( "%.3f: Client error: %s [%d]\n", GetTime(), GetClientErrorString( error ), extendedError );
}

void GameClient::OnServerDataReceived( const protocol::Block & block )
{
    printf( "%.3f: Client received server data: %d bytes\n", GetTime(), block.GetSize() );

    SetContext( clientServer::CONTEXT_USER, block.GetData() );
}

GameClient * CreateGameClient( core::Allocator & allocator, int clientPort )
{
    auto packetFactory = CORE_NEW( allocator, GamePacketFactory, allocator );

    auto messageFactory = CORE_NEW( allocator, GameMessageFactory, allocator );

    auto channelStructure = CORE_NEW( allocator, GameChannelStructure, *messageFactory );

    network::BSDSocketConfig bsdSocketConfig;
    bsdSocketConfig.port = clientPort;
    bsdSocketConfig.maxPacketSize = 1200;
    bsdSocketConfig.packetFactory = packetFactory;
    auto networkInterface = CORE_NEW( allocator, network::BSDSocket, bsdSocketConfig );

    network::SimulatorConfig networkSimulatorConfig;
    networkSimulatorConfig.packetFactory = packetFactory;
    auto networkSimulator = CORE_NEW( allocator, network::Simulator, networkSimulatorConfig );

    const int clientDataSize = 4096 + 21;
    auto clientData = CORE_NEW( allocator, protocol::Block, allocator, clientDataSize );
    {
        uint8_t * data = clientData->GetData();
        for ( int i = 0; i < clientDataSize; ++i )
            data[i] = ( 20 + i ) % 256;
    }

    clientServer::ClientConfig clientConfig;
    clientConfig.clientData = clientData;
    clientConfig.channelStructure = channelStructure;        
    clientConfig.networkInterface = networkInterface;
    clientConfig.networkSimulator = networkSimulator;

    return CORE_NEW( allocator, GameClient, clientConfig );
}

void DestroyGameClient( core::Allocator & allocator, GameClient * client )
{
    CORE_ASSERT( client );

    clientServer::ClientConfig config = client->GetConfig();

    typedef network::Interface NetworkInterface;
    typedef network::Simulator NetworkSimulator;

    CORE_DELETE( allocator, GameClient, client );
    CORE_DELETE( allocator, ChannelStructure, config.channelStructure );
    CORE_DELETE( allocator, NetworkInterface, config.networkInterface );
    CORE_DELETE( allocator, NetworkSimulator, config.networkSimulator );
}

CONSOLE_FUNCTION( connect )
{
    CORE_ASSERT( global.client );

    network::Address address( args );
    
    if ( address.GetPort() == 0 )
        address.SetPort( ServerPort );

    if ( !address.IsValid() )
    {
        printf( "%.3f: error - connect address \"%s\" is not valid\n", global.timeBase.time, args );
        global.client->Disconnect();
        return;
    }

    global.client->Connect( address );
}

CONSOLE_FUNCTION( disconnect )
{
    CORE_ASSERT( global.client );

    global.client->Disconnect();
}

#endif // #ifdef CLIENT
