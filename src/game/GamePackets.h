#ifndef GAME_PACKETS_H
#define GAME_PACKETS_H

#include "Stream.h"
#include "Packets.h"
#include "PacketFactory.h"

using namespace protocol;

enum PacketTypes
{
    PACKET_CONNECTION = CLIENT_SERVER_PACKET_CONNECTION,

    // ...

    NUM_PACKET_TYPES
};

// ...

class GamePacketFactory : public PacketFactory
{
    Allocator * m_allocator;

public:

    GamePacketFactory( Allocator & allocator )
        : PacketFactory( allocator, NUM_PACKET_TYPES )
    {
        m_allocator = &allocator;
    }

protected:

    Packet * CreateInternal( int type )
    {
        switch ( type )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_REQUEST:       return CORE_NEW( *m_allocator, ConnectionRequestPacket );
            case CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE:       return CORE_NEW( *m_allocator, ChallengeResponsePacket );

            case CLIENT_SERVER_PACKET_CONNECTION_DENIED:        return CORE_NEW( *m_allocator, ConnectionDeniedPacket );
            case CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE:     return CORE_NEW( *m_allocator, ConnectionChallengePacket );

            case CLIENT_SERVER_PACKET_READY_FOR_CONNECTION:     return CORE_NEW( *m_allocator, ReadyForConnectionPacket );
            case CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT:      return CORE_NEW( *m_allocator, DataBlockFragmentPacket );
            case CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK:  return CORE_NEW( *m_allocator, DataBlockFragmentAckPacket );
            case CLIENT_SERVER_PACKET_DISCONNECTED:             return CORE_NEW( *m_allocator, DisconnectedPacket );

            case PACKET_CONNECTION:                             return CORE_NEW( *m_allocator, ConnectionPacket );

            // ...

            default:
                return nullptr;
        }
    }
};

#endif
