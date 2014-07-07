#ifndef TEST_PACKETS_H
#define TEST_PACKETS_H

#include "Stream.h"
#include "Packets.h"
#include "PacketFactory.h"

using namespace protocol;

enum PacketType
{
    PACKET_CONNECTION,
    PACKET_CONNECT,
    PACKET_UPDATE,
    PACKET_DISCONNECT
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
    ChannelStructure * m_channelStructure = nullptr;

public:

    TestPacketFactory( Allocator & allocator, ChannelStructure * channelStructure = nullptr )
        : PacketFactory( allocator )
    {
        m_channelStructure = channelStructure;
 
        Register( PACKET_CONNECTION, [&allocator, this] { PROTOCOL_ASSERT( m_channelStructure ); return PROTOCOL_NEW( allocator, ConnectionPacket, PACKET_CONNECTION, m_channelStructure ); } );
        Register( PACKET_CONNECT,    [&allocator] { return PROTOCOL_NEW( allocator, ConnectPacket );    } );
        Register( PACKET_UPDATE,     [&allocator] { return PROTOCOL_NEW( allocator, UpdatePacket );     } );
        Register( PACKET_DISCONNECT, [&allocator] { return PROTOCOL_NEW( allocator, DisconnectPacket ); } );
    }
};

#endif
