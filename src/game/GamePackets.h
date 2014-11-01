#ifndef GAME_PACKETS_H
#define GAME_PACKETS_H

#include "protocol/Stream.h"
#include "protocol/PacketFactory.h"
#include "protocol/ConnectionPacket.h"
#include "clientServer/ClientServerPackets.h"

enum PacketTypes
{
    PACKET_CONNECTION = clientServer::CLIENT_SERVER_PACKET_CONNECTION,

    // ...

    NUM_PACKET_TYPES
};

// ...

class GamePacketFactory : public protocol::PacketFactory
{
    core::Allocator * m_allocator;

public:

    GamePacketFactory( core::Allocator & allocator )
        : PacketFactory( allocator, NUM_PACKET_TYPES )
    {
        m_allocator = &allocator;
    }

protected:

    protocol::Packet * CreateInternal( int type )
    {
        switch ( type )
        {
            // todo: remove the CLIENT_SERVER prefix?
            case clientServer::CLIENT_SERVER_PACKET_CONNECTION_REQUEST:       return CORE_NEW( *m_allocator, clientServer::ConnectionRequestPacket );
            case clientServer::CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE:       return CORE_NEW( *m_allocator, clientServer::ChallengeResponsePacket );

            case clientServer::CLIENT_SERVER_PACKET_CONNECTION_DENIED:        return CORE_NEW( *m_allocator, clientServer::ConnectionDeniedPacket );
            case clientServer::CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE:     return CORE_NEW( *m_allocator, clientServer::ConnectionChallengePacket );

            case clientServer::CLIENT_SERVER_PACKET_READY_FOR_CONNECTION:     return CORE_NEW( *m_allocator, clientServer::ReadyForConnectionPacket );
            case clientServer::CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT:      return CORE_NEW( *m_allocator, clientServer::DataBlockFragmentPacket );
            case clientServer::CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK:  return CORE_NEW( *m_allocator, clientServer::DataBlockFragmentAckPacket );
            case clientServer::CLIENT_SERVER_PACKET_DISCONNECTED:             return CORE_NEW( *m_allocator, clientServer::DisconnectedPacket );

            case PACKET_CONNECTION:                                           return CORE_NEW( *m_allocator, protocol::ConnectionPacket );

            // ...

            default:
                return nullptr;
        }
    }
};

#endif // #ifndef GAME_PACKETS_H
