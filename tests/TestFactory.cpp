#include "Factory.h"
#include "Packet.h"

using namespace protocol;

enum PacketType
{
    PACKET_CONNECT,
    PACKET_UPDATE,
    PACKET_DISCONNECT
};

struct ConnectPacket : public Packet
{
    ConnectPacket() : Packet( PACKET_CONNECT ) {}
    void SerializeRead( ReadStream & stream ) {}
    void SerializeWrite( WriteStream & stream ) {}
    void SerializeMeasure( MeasureStream & stream ) {}
};

struct UpdatePacket : public Packet
{
    UpdatePacket() : Packet( PACKET_UPDATE ) {}
    void SerializeRead( ReadStream & stream ) {}
    void SerializeWrite( WriteStream & stream ) {}
    void SerializeMeasure( MeasureStream & stream ) {}
};

struct DisconnectPacket : public Packet
{
    DisconnectPacket() : Packet( PACKET_DISCONNECT ) {}
    void SerializeRead( ReadStream & stream ) {}
    void SerializeWrite( WriteStream & stream ) {}
    void SerializeMeasure( MeasureStream & stream ) {}
};

class PacketFactory : public Factory<Packet>
{
public:
    PacketFactory()
    {
        Register( PACKET_CONNECT,    [] { return new ConnectPacket();    } );
        Register( PACKET_UPDATE,     [] { return new UpdatePacket();     } );
        Register( PACKET_DISCONNECT, [] { return new DisconnectPacket(); } );
    }
};

void test_factory()
{
    printf( "test_factory\n" );

    PacketFactory packetFactory;

    auto connectPacket = packetFactory.Create( PACKET_CONNECT );
    auto updatePacket = packetFactory.Create( PACKET_UPDATE );
    auto disconnectPacket = packetFactory.Create( PACKET_DISCONNECT );

    assert( connectPacket->GetType() == PACKET_CONNECT );
    assert( updatePacket->GetType() == PACKET_UPDATE );
    assert( disconnectPacket->GetType() == PACKET_DISCONNECT );

    delete connectPacket;
    delete updatePacket;
    delete disconnectPacket;
}

int main()
{
    srand( time( nullptr ) );

    test_factory();

    return 0;
}
