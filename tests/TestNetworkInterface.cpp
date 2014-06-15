#include "NetworkInterface.h"

using namespace protocol;

class TestInterface : public NetworkInterface
{
public:

    TestInterface()
    {
        // ...
    }

    ~TestInterface()
    {
        while ( !packet_queue.empty() )
        {
            auto packet = packet_queue.front();
            packet_queue.pop();
            delete packet;
        }
    }

    virtual void SendPacket( const Address & address, Packet * packet )
    {
        packet->SetAddress( address );
        packet_queue.push( packet );
    }

    virtual Packet * ReceivePacket()
    {
        if ( packet_queue.empty() )
            return nullptr;
        auto packet = packet_queue.front();
        packet_queue.pop();
        return packet;
    }

    virtual void Update( const TimeBase & timeBase )
    {
        // ...
    }

    virtual uint32_t GetMaxPacketSize() const
    {
        return 0xFFFFFFFF;
    }

    PacketQueue packet_queue;
};

enum PacketType
{
    PACKET_A,
    PACKET_B,
    PACKET_C
};

class TestPacketA : public Packet
{
public:
    TestPacketA() : Packet( PACKET_A ) {}
    void SerializeRead( ReadStream & stream ) {}
    void SerializeWrite( WriteStream & stream ) {}
    void SerializeMeasure( MeasureStream & stream ) {}
};

class TestPacketB : public Packet
{
public:
    TestPacketB() : Packet( PACKET_B ) {}
    void SerializeRead( ReadStream & stream ) {}
    void SerializeWrite( WriteStream & stream ) {}
    void SerializeMeasure( MeasureStream & stream ) {}
};

class TestPacketC : public Packet
{
public:
    TestPacketC() : Packet( PACKET_C ) {}
    void SerializeRead( ReadStream & stream ) {}
    void SerializeWrite( WriteStream & stream ) {}
    void SerializeMeasure( MeasureStream & stream ) {}
};

void test_network_interface()
{
    printf( "test_network_interface\n" );

    TestInterface interface;

    Address address( 127,0,0,1, 1000 );

    interface.SendPacket( address, new TestPacketA() );
    interface.SendPacket( address, new TestPacketB() );
    interface.SendPacket( address, new TestPacketC() );

    auto connectPacket = interface.ReceivePacket();
    auto updatePacket = interface.ReceivePacket();
    auto disconnectPacket = interface.ReceivePacket();

    assert( connectPacket->GetType() == PACKET_A );
    assert( connectPacket->GetAddress() == address );

    assert( updatePacket->GetType() == PACKET_B );
    assert( updatePacket->GetAddress() == address );

    assert( disconnectPacket->GetType() == PACKET_C );
    assert( disconnectPacket->GetAddress() == address );

    assert( interface.ReceivePacket() == nullptr );

    delete connectPacket;
    delete updatePacket;
    delete disconnectPacket;
}

int main()
{
    srand( time( nullptr ) );

    test_network_interface();

    return 0;
}
