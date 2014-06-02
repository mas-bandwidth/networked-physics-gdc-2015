#include "NetworkInterface.h"

using namespace std;
using namespace protocol;

class TestInterface : public NetworkInterface
{
public:

    virtual void SendPacket( const Address & address, shared_ptr<Packet> packet )
    {
        packet->SetAddress( address );
        packet_queue.push( packet );
    }

    virtual void SendPacket( const string & address, uint16_t port, shared_ptr<Packet> packet )
    {
        assert( false );        // not supported
    }

    virtual shared_ptr<Packet> ReceivePacket()
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
    void Serialize( Stream & stream ) {}
};

class TestPacketB : public Packet
{
public:
    TestPacketB() : Packet( PACKET_B ) {}
    void Serialize( Stream & stream ) {}
};

class TestPacketC : public Packet
{
public:
    TestPacketC() : Packet( PACKET_C ) {}
    void Serialize( Stream & stream ) {}
};

void test_network_interface()
{
    cout << "test_network_interface" << endl;

    TestInterface interface;

    Address address( 127,0,0,1, 1000 );

    interface.SendPacket( address, make_shared<TestPacketA>() );
    interface.SendPacket( address, make_shared<TestPacketB>() );
    interface.SendPacket( address, make_shared<TestPacketC>() );

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
}

int main()
{
    srand( time( NULL ) );

    try
    {
        test_network_interface();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
