#ifndef GAME_CLIENT_H
#define GAME_CLIENT_H

#include "Client.h"
#include "GameMessages.h"
#include "GameContext.h"
#include "GamePackets.h"
#include "GameMessages.h"
#include "GameChannelStructure.h"
#include "BSDSocket.h"
#include "NetworkSimulator.h"

using namespace protocol;

class GameClient : public Client
{
public:

    GameClient( const ClientConfig & config ) : Client( config ) {}

    const GameContext * GetGameContext() const
    {
        const Block * block = GetServerData();
        if ( block )
            return (const GameContext*) block->GetData();
        else
            return nullptr;
    }

    uint16_t GetPort() const
    {
        auto socket = (BSDSocket*) GetConfig().networkInterface;
        CORE_ASSERT( socket );
        return socket->GetPort();
    }

    double GetTime() const
    {
        return GetTimeBase().time;
    }

protected:

    void OnConnect( const Address & address ) override
    {
        char buffer[256];
        printf( "%.2f: Client connecting to %s\n", GetTime(), address.ToString( buffer, sizeof( buffer ) ) );
    }

    void OnConnect( const char * hostname ) override
    {
        printf( "%.2f: Client connecting to %s\n", GetTime(), hostname );
    }

    void OnStateChange( ClientState previous, ClientState current ) override
    {
        printf( "%.2f: Client state change: %s -> %s\n", GetTime(), GetClientStateName( previous ), GetClientStateName( current ) );
    }

    void OnDisconnect() override
    {
        printf( "%.2f: Client disconnect\n", GetTime() );
    }

    void OnError( ClientError error, uint32_t extendedError ) override
    {
        printf( "%.2f: Client error: %s [%d]\n", GetTime(), GetClientErrorString( error ), extendedError );
    }

    void OnServerDataReceived( const Block & block ) override
    {
        printf( "%.2f: Client received server data: %d bytes\n", GetTime(), block.GetSize() );

        SetContext( CONTEXT_USER, block.GetData() );
    }
};

GameClient * CreateGameClient( Allocator & allocator, int clientPort = 0 )
{
    auto packetFactory = CORE_NEW( allocator, GamePacketFactory, allocator );

    auto messageFactory = CORE_NEW( allocator, GameMessageFactory, allocator );

    auto channelStructure = CORE_NEW( allocator, GameChannelStructure, *messageFactory );

    BSDSocketConfig bsdSocketConfig;
    bsdSocketConfig.port = clientPort;
    bsdSocketConfig.maxPacketSize = 1200;
    bsdSocketConfig.packetFactory = packetFactory;
    auto networkInterface = CORE_NEW( allocator, BSDSocket, bsdSocketConfig );

    NetworkSimulatorConfig networkSimulatorConfig;
    networkSimulatorConfig.packetFactory = packetFactory;
    auto networkSimulator = CORE_NEW( allocator, NetworkSimulator, networkSimulatorConfig );

    const int clientDataSize = 4096 + 21;
    auto clientData = CORE_NEW( allocator, Block, allocator, clientDataSize );
    {
        uint8_t * data = clientData->GetData();
        for ( int i = 0; i < clientDataSize; ++i )
            data[i] = ( 20 + i ) % 256;
    }

    ClientConfig clientConfig;
    clientConfig.clientData = clientData;
    clientConfig.channelStructure = channelStructure;        
    clientConfig.networkInterface = networkInterface;
    clientConfig.networkSimulator = networkSimulator;

    return CORE_NEW( allocator, GameClient, clientConfig );
}

void DestroyGameClient( Allocator & allocator, GameClient * client )
{
    CORE_ASSERT( client );

    ClientConfig config = client->GetConfig();

    CORE_DELETE( allocator, GameClient, client );
    CORE_DELETE( allocator, ChannelStructure, config.channelStructure );
    CORE_DELETE( allocator, NetworkInterface, config.networkInterface );
    CORE_DELETE( allocator, NetworkSimulator, config.networkSimulator );

    // todo: delete packet factory

    // todo: delete message factory

    // todo: delete channel structure
}

#endif
