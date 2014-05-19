#include "Common.h"
#include "Packet.h"

using namespace std;
using namespace protocol;

void test_sequence()
{
    cout << "test_sequence" << endl;

    assert( sequence_greater_than( 0, 0 ) == false );
    assert( sequence_greater_than( 1, 0 ) == true );
    assert( sequence_greater_than( 0, -1 ) == true );

    assert( sequence_less_than( 0, 0 ) == false );
    assert( sequence_less_than( 0, 1 ) == true );
    assert( sequence_less_than( -1, 0 ) == true );
}

struct TestPacketData
{
    TestPacketData()
        : valid(0), sequence(0) {}

    TestPacketData( uint16_t _sequence )
        : valid(1), sequence( _sequence ) {}

    uint32_t valid : 1;                     // is this packet entry valid?
    uint32_t sequence : 16;                 // packet sequence #
};

void test_sliding_window()
{
    cout << "test_sliding_window" << endl;

    const int size = 256;

    SlidingWindow<TestPacketData> slidingWindow( size );

    for ( int i = 0; i < size; ++i )
        assert( slidingWindow.Find(i) == nullptr );

    for ( int i = 0; i <= size*4; ++i )
    {
        slidingWindow.Insert( i );
        assert( slidingWindow.GetSequence() == i );
    }

    for ( int i = 0; i <= size; ++i )
    {
        // note: outside bounds of sliding window!
        bool insert_succeeded = slidingWindow.Insert( i );
        assert( !insert_succeeded );
    }    

    int index = size*4;
    for ( int i = 0; i < size; ++i )
    {
        auto entry = slidingWindow.Find( index );
        assert( entry );
        assert( entry->valid );
        assert( entry->sequence == index );
        index--;
    }

    slidingWindow.Reset();

    assert( slidingWindow.GetSequence() == 0 );

    for ( int i = 0; i < size; ++i )
        assert( slidingWindow.Find(i) == nullptr );
}

void test_generate_ack_bits()
{
    cout << "test_generate_ack_bits" << endl;

    const int size = 256;

    SlidingWindow<TestPacketData> received_packets( size );

    uint16_t ack = -1;
    uint32_t ack_bits = -1;

    GenerateAckBits( received_packets, ack, ack_bits );
    assert( ack == 0 );
    assert( ack_bits == 0 );

    for ( int i = 0; i <= size; ++i )
        received_packets.Insert( i );

    GenerateAckBits( received_packets, ack, ack_bits );
    assert( ack == size );
    assert( ack_bits == 0xFFFFFFFF );

    received_packets.Reset();
    uint16_t input_acks[] = { 1, 5, 9, 11 };
    int input_num_acks = sizeof( input_acks ) / sizeof( uint16_t );
    for ( int i = 0; i < input_num_acks; ++i )
        received_packets.Insert( input_acks[i] );

    GenerateAckBits( received_packets, ack, ack_bits );

    assert( ack == 11 );
    assert( ack_bits == ( 1 | (1<<(11-9)) | (1<<(11-5)) | (1<<(11-1)) ) );
}

enum PacketType
{
    PACKET_Connect,
    PACKET_Update,
    PACKET_Disconnect
};

struct ConnectPacket : public Packet
{
    ConnectPacket() : Packet( PACKET_Connect ) {}
    void Serialize( Stream & stream ) {}
};

struct UpdatePacket : public Packet
{
    UpdatePacket() : Packet( PACKET_Update ) {}
    void Serialize( Stream & stream ) {}
};

struct DisconnectPacket : public Packet
{
    DisconnectPacket() : Packet( PACKET_Disconnect ) {}
    void Serialize( Stream & stream ) {}
};

class PacketFactory : public Factory<Packet>
{
public:
    PacketFactory()
    {
        Register( PACKET_Connect, [] { return make_shared<ConnectPacket>(); } );
        Register( PACKET_Update, [] { return make_shared<UpdatePacket>(); } );
        Register( PACKET_Disconnect, [] { return make_shared<DisconnectPacket>(); } );
    }
};

void test_factory()
{
    cout << "test_factory" << endl;

    PacketFactory packetFactory;

    auto connectPacket = packetFactory.Create( PACKET_Connect );
    auto updatePacket = packetFactory.Create( PACKET_Update );
    auto disconnectPacket = packetFactory.Create( PACKET_Disconnect );

    assert( connectPacket->GetType() == PACKET_Connect );
    assert( updatePacket->GetType() == PACKET_Update );
    assert( disconnectPacket->GetType() == PACKET_Disconnect );
}

int main()
{
    try
    {
        test_sequence();
        test_sliding_window();
        test_generate_ack_bits();
        test_factory();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
