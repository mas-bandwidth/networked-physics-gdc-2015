#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include "clientServer/Server.h"
#include "GameContext.h"
#include "GamePackets.h"
#include "GameMessages.h"
#include "GameChannelStructure.h"
#include "network/BSDSocket.h"
#include "network/Simulator.h"

class GameServer : public clientServer::Server
{
public:

    GameServer( const clientServer::ServerConfig & config ) : Server( config )
    {
        CORE_ASSERT( config.serverData );

        SetContext( clientServer::CONTEXT_USER, config.serverData->GetData() );
    }

    double GetTime() const
    {
        return GetTimeBase().time;
    }

protected:

    void OnClientStateChange( int clientIndex, clientServer::ServerClientState previous, clientServer::ServerClientState current ) override
    {
        printf( "%.3f: Client %d state change: %s -> %s\n", GetTime(), clientIndex, GetServerClientStateName( previous ), GetServerClientStateName( current ) );
    }

    void OnClientDataReceived( int clientIndex, const protocol::Block & block ) override
    {
        printf( "%.3f: Client %d received client data: %d bytes\n", GetTime(), clientIndex, block.GetSize() );
    }

    void OnClientTimedOut( int clientIndex ) override
    {
        printf( "%.3f: Client %d timed out\n", GetTime(), clientIndex );
    }
};

GameServer * CreateGameServer( core::Allocator & allocator, int serverPort, int maxClients )
{
    auto packetFactory = CORE_NEW( allocator, GamePacketFactory, allocator );

    auto messageFactory = CORE_NEW( allocator, GameMessageFactory, allocator );

    auto channelStructure = CORE_NEW( allocator, GameChannelStructure, *messageFactory );

    network::BSDSocketConfig bsdSocketConfig;
    bsdSocketConfig.port = serverPort;
    bsdSocketConfig.maxPacketSize = 1200;
    bsdSocketConfig.packetFactory = packetFactory;
    auto networkInterface = CORE_NEW( allocator, network::BSDSocket, bsdSocketConfig );

    network::SimulatorConfig networkSimulatorConfig;
    networkSimulatorConfig.packetFactory = packetFactory;
    auto networkSimulator = CORE_NEW( allocator, network::Simulator, networkSimulatorConfig );

    const int serverDataSize = sizeof(GameContext) + 10 * 1024 + 11;
    auto serverData = CORE_NEW( allocator, protocol::Block, allocator, serverDataSize );
    {
        uint8_t * data = serverData->GetData();
        for ( int i = 0; i < serverDataSize; ++i )
            data[i] = ( 10 + i ) % 256;

        auto gameContext = (GameContext*) data;
        gameContext->value_min = -1 - ( rand() % 100000000 );
        gameContext->value_max = rand() % 1000000000;
    }

    clientServer::ServerConfig serverConfig;
    serverConfig.serverData = serverData;
    serverConfig.maxClients = maxClients;
    serverConfig.channelStructure = channelStructure;
    serverConfig.networkInterface = networkInterface;
    serverConfig.networkSimulator = networkSimulator;

    return CORE_NEW( allocator, GameServer, serverConfig );
}

void DestroyGameServer( core::Allocator & allocator, GameServer * server )
{
    CORE_ASSERT( server );

    clientServer::ServerConfig config = server->GetConfig();

    typedef network::Interface NetworkInterface;
    typedef network::Simulator NetworkSimulator;

    CORE_DELETE( allocator, GameServer, server );
    CORE_DELETE( allocator, ChannelStructure, config.channelStructure );
    CORE_DELETE( allocator, NetworkInterface, config.networkInterface );
    CORE_DELETE( allocator, NetworkSimulator, config.networkSimulator );

    // todo: delete packet factory

    // todo: delete message factory

    // todo: delete channel structure
}

#endif // #ifndef GAME_SERVER_H
