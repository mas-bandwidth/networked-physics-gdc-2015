#ifndef TEST_PACKETS_H
#define TEST_PACKETS_H

#include "protocol/Stream.h"
#include "protocol/PacketFactory.h"
#include "protocol/ConnectionPacket.h"

// TODO: WE SHOULDN'T NEED PROTOCOL PACKET DEFINITIONS IN NETWORKING!!!!

enum PacketTypes
{
    PACKET_CONNECTION = protocol::CONNECTION_PACKET,

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

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_int( stream, a, -10, 10 );
        serialize_int( stream, b, -10, 10 );
        serialize_int( stream, c, -10, 10 );
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

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_bits( stream, timestamp, 16 );
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

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_int( stream, x, -100, +100 );
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
