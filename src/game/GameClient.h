#ifndef GAME_CLIENT_H
#define GAME_CLIENT_H

#include "protocol/Client.h"
#include "network/BSDSocket.h"
#include "network/Simulator.h"
#include "GameMessages.h"
#include "GameContext.h"
#include "GamePackets.h"
#include "GameMessages.h"
#include "GameChannelStructure.h"

class GameClient : public protocol::Client
{
public:

    GameClient( const protocol::ClientConfig & config ) : Client( config ) {}

    const GameContext * GetGameContext() const
    {
        const protocol::Block * block = GetServerData();
        if ( block )
            return (const GameContext*) block->GetData();
        else
            return nullptr;
    }

    uint16_t GetPort() const
    {
        auto socket = (network::BSDSocket*) GetConfig().networkInterface;
        CORE_ASSERT( socket );
        return socket->GetPort();
    }

    double GetTime() const
    {
        return GetTimeBase().time;
    }

protected:

    void OnConnect( const network::Address & address ) override
    {
        char buffer[256];
        printf( "%.2f: Client connecting to %s\n", GetTime(), address.ToString( buffer, sizeof( buffer ) ) );
    }

    void OnConnect( const char * hostname ) override
    {
        printf( "%.2f: Client connecting to %s\n", GetTime(), hostname );
    }

    void OnStateChange( protocol::ClientState previous, protocol::ClientState current ) override
    {
        printf( "%.2f: Client state change: %s -> %s\n", GetTime(), GetClientStateName( previous ), GetClientStateName( current ) );
    }

    void OnDisconnect() override
    {
        printf( "%.2f: Client disconnect\n", GetTime() );
    }

    void OnError( protocol::ClientError error, uint32_t extendedError ) override
    {
        printf( "%.2f: Client error: %s [%d]\n", GetTime(), GetClientErrorString( error ), extendedError );
    }

    void OnServerDataReceived( const protocol::Block & block ) override
    {
        printf( "%.2f: Client received server data: %d bytes\n", GetTime(), block.GetSize() );

        SetContext( protocol::CONTEXT_USER, block.GetData() );
    }
};

GameClient * CreateGameClient( core::Allocator & allocator, int clientPort = 0 )
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

    protocol::ClientConfig clientConfig;
    clientConfig.clientData = clientData;
    clientConfig.channelStructure = channelStructure;        
    clientConfig.networkInterface = networkInterface;
    clientConfig.networkSimulator = networkSimulator;

    return CORE_NEW( allocator, GameClient, clientConfig );
}

void DestroyGameClient( core::Allocator & allocator, GameClient * client )
{
    CORE_ASSERT( client );

    protocol::ClientConfig config = client->GetConfig();

    // todo: hack
    typedef network::Interface NetworkInterface;
    typedef network::Simulator NetworkSimulator;

    CORE_DELETE( allocator, GameClient, client );
    CORE_DELETE( allocator, ChannelStructure, config.channelStructure );
    CORE_DELETE( allocator, NetworkInterface, config.networkInterface );
    CORE_DELETE( allocator, NetworkSimulator, config.networkSimulator );

    // todo: delete packet factory

    // todo: delete message factory

    // todo: delete channel structure
}

#endif
