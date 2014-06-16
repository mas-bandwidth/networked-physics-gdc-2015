#include "Common.h"
#include "Packet.h"

using namespace protocol;

void test_sequence()
{
    printf( "test_sequence\n" );

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
    printf( "test_sliding_window\n" );

    const int size = 256;

    SlidingWindow<TestPacketData> slidingWindow( size );

    for ( int i = 0; i < size; ++i )
        assert( slidingWindow.Find(i) == nullptr );

    for ( int i = 0; i <= size*4; ++i )
    {
        slidingWindow.Insert( i );
        assert( slidingWindow.GetSequence() == i + 1 );
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
    printf( "test_generate_ack_bits\n" );

    const int size = 256;

    SlidingWindow<TestPacketData> received_packets( size );

    uint16_t ack = -1;
    uint32_t ack_bits = -1;

    GenerateAckBits( received_packets, ack, ack_bits );
    assert( ack == 0xFFFF );
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

    test_sequence();
    test_sliding_window();
    test_generate_ack_bits();
    test_factory();

    return 0;
}
