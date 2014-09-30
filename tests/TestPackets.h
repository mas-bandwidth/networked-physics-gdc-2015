#ifndef TEST_PACKETS_H
#define TEST_PACKETS_H

#include "protocol/Stream.h"
#include "protocol/Packets.h"
#include "protocol/PacketFactory.h"

enum PacketTypes
{
    PACKET_CONNECTION = protocol::CLIENT_SERVER_PACKET_CONNECTION,

    PACKET_CONNECT,
    PACKET_UPDATE,
    PACKET_DISCONNECT,

    NUM_PACKET_TYPES
};

struct ConnectPacket : public protocol::Packet
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

    void SerializeRead( protocol::ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( protocol::WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( protocol::MeasureStream & stream )
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

struct UpdatePacket : public protocol::Packet
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

    void SerializeRead( protocol::ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( protocol::WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( protocol::MeasureStream & stream )
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

struct DisconnectPacket : public protocol::Packet
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

    void SerializeRead( protocol::ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( protocol::WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( protocol::MeasureStream & stream )
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

class TestPacketFactory : public protocol::PacketFactory
{
    core::Allocator * m_allocator;

public:

    TestPacketFactory( core::Allocator & allocator )
        : PacketFactory( allocator, NUM_PACKET_TYPES )
    {
        m_allocator = &allocator;
    }

protected:

    protocol::Packet * CreateInternal( int type )
    {
        switch ( type )
        {
            // todo: maybe remove the whole CLIENT_SERVER prefix. Just PACKET_ should do (protocol::)

            case protocol::CLIENT_SERVER_PACKET_CONNECTION_REQUEST:       return CORE_NEW( *m_allocator, protocol::ConnectionRequestPacket );
            case protocol::CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE:       return CORE_NEW( *m_allocator, protocol::ChallengeResponsePacket );

            case protocol::CLIENT_SERVER_PACKET_CONNECTION_DENIED:        return CORE_NEW( *m_allocator, protocol::ConnectionDeniedPacket );
            case protocol::CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE:     return CORE_NEW( *m_allocator, protocol::ConnectionChallengePacket );

            case protocol::CLIENT_SERVER_PACKET_READY_FOR_CONNECTION:     return CORE_NEW( *m_allocator, protocol::ReadyForConnectionPacket );
            case protocol::CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT:      return CORE_NEW( *m_allocator, protocol::DataBlockFragmentPacket );
            case protocol::CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK:  return CORE_NEW( *m_allocator, protocol::DataBlockFragmentAckPacket );
            case protocol::CLIENT_SERVER_PACKET_DISCONNECTED:             return CORE_NEW( *m_allocator, protocol::DisconnectedPacket );

            // todo: don't like the aliasing here
            case PACKET_CONNECTION:     return CORE_NEW( *m_allocator, protocol::ConnectionPacket );

            case PACKET_CONNECT:        return CORE_NEW( *m_allocator, ConnectPacket );
            case PACKET_UPDATE:         return CORE_NEW( *m_allocator, UpdatePacket );
            case PACKET_DISCONNECT:     return CORE_NEW( *m_allocator, DisconnectPacket );

            default:
                return nullptr;
        }
    }
};

#endif
