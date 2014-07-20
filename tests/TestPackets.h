#ifndef TEST_PACKETS_H
#define TEST_PACKETS_H

#include "Stream.h"
#include "Packets.h"
#include "PacketFactory.h"

using namespace protocol;

enum PacketTypes
{
    PACKET_CONNECTION = CLIENT_SERVER_PACKET_CONNECTION,

    PACKET_CONNECT,
    PACKET_UPDATE,
    PACKET_DISCONNECT,

    NUM_PACKET_TYPES
};

struct ConnectPacket : public Packet
{
    int a,b,c;

    ConnectPacket() : Packet( PACKET_CONNECT )
    {
        a = 1;
        b = 2;
        c = 3;        
    }

    template <typename Stream> void Serialize( Stream & stream )
    {
        serialize_int( stream, a, -10, 10 );
        serialize_int( stream, b, -10, 10 );
        serialize_int( stream, c, -10, 10 );
    }

    void SerializeRead( ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( MeasureStream & stream )
    {
        Serialize( stream );
    }

    ConnectPacket & operator = ( const ConnectPacket & other )
    {
        a = other.a;
        b = other.b;
        c = other.c;
        return *this;
    }

    bool operator ==( const ConnectPacket & other ) const
    {
        return a == other.a && b == other.b && c == other.c;
    }

    bool operator !=( const ConnectPacket & other ) const
    {
        return !( *this == other );
    }
};

struct UpdatePacket : public Packet
{
    uint16_t timestamp;

    UpdatePacket() : Packet( PACKET_UPDATE )
    {
        timestamp = 0;
    }

    template <typename Stream> void Serialize( Stream & stream )
    {
        serialize_bits( stream, timestamp, 16 );
    }

    void SerializeRead( ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( MeasureStream & stream )
    {
        Serialize( stream );
    }

    UpdatePacket & operator = ( const UpdatePacket & other )
    {
        timestamp = other.timestamp;
        return *this;
    }

    bool operator ==( const UpdatePacket & other ) const
    {
        return timestamp == other.timestamp;
    }

    bool operator !=( const UpdatePacket & other ) const
    {
        return !( *this == other );
    }
};

struct DisconnectPacket : public Packet
{
    int x;

    DisconnectPacket() : Packet( PACKET_DISCONNECT )
    {
        x = 2;
    }

    template <typename Stream> void Serialize( Stream & stream )
    {
        serialize_int( stream, x, -100, +100 );
    }

    void SerializeRead( ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( MeasureStream & stream )
    {
        Serialize( stream );
    }

    DisconnectPacket & operator = ( const DisconnectPacket & other )
    {
        x = other.x;
        return *this;
    }

    bool operator ==( const DisconnectPacket & other ) const
    {
        return x == other.x;
    }

    bool operator !=( const DisconnectPacket & other ) const
    {
        return !( *this == other );
    }
};

class TestPacketFactory : public PacketFactory
{
    Allocator * m_allocator;
    ChannelStructure * m_channelStructure = nullptr;

public:

    TestPacketFactory( Allocator & allocator, ChannelStructure * channelStructure = nullptr )
        : PacketFactory( allocator, NUM_PACKET_TYPES )
    {
        m_allocator = &allocator;
        m_channelStructure = channelStructure;
    }

protected:

    Packet * CreateInternal( int type )
    {
        switch ( type )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_REQUEST:       return PROTOCOL_NEW( *m_allocator, ConnectionRequestPacket );
            case CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE:       return PROTOCOL_NEW( *m_allocator, ChallengeResponsePacket );
            case CLIENT_SERVER_PACKET_READY_FOR_CONNECTION:     return PROTOCOL_NEW( *m_allocator, ReadyForConnectionPacket );

            case CLIENT_SERVER_PACKET_CONNECTION_DENIED:        return PROTOCOL_NEW( *m_allocator, ConnectionDeniedPacket );
            case CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE:     return PROTOCOL_NEW( *m_allocator, ConnectionChallengePacket );
            case CLIENT_SERVER_PACKET_REQUEST_CLIENT_DATA:      return PROTOCOL_NEW( *m_allocator, RequestClientDataPacket );

            case CLIENT_SERVER_PACKET_DISCONNECTED:             return PROTOCOL_NEW( *m_allocator, DisconnectedPacket );
            case CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT:      return PROTOCOL_NEW( *m_allocator, DataBlockFragmentPacket );
            case CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK:  return PROTOCOL_NEW( *m_allocator, DataBlockFragmentAckPacket );

            case PACKET_CONNECTION:     return PROTOCOL_NEW( *m_allocator, ConnectionPacket, PACKET_CONNECTION, m_channelStructure );

            case PACKET_CONNECT:        return PROTOCOL_NEW( *m_allocator, ConnectPacket );
            case PACKET_UPDATE:         return PROTOCOL_NEW( *m_allocator, UpdatePacket );
            case PACKET_DISCONNECT:     return PROTOCOL_NEW( *m_allocator, DisconnectPacket );

            default:
                return nullptr;
        }
    }
};

#endif
