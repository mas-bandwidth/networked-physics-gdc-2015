#include "Common.h"

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

// todo: simplify this down. don't need this much to test factory
/*
struct ConnectPacket : public Packet
{
    int a,b,c;

    ConnectPacket() : Packet( PACKET_Connect )
    {
        a = 1;
        b = 2;
        c = 3;        
    }

    void Serialize( Stream & stream )
    {
        serialize_int( stream, a, -10, 10 );
        serialize_int( stream, b, -10, 10 );
        serialize_int( stream, c, -10, 10 );
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

    UpdatePacket() : Packet( PACKET_Update )
    {
        timestamp = 0;
    }

    void Serialize( Stream & stream )
    {
        serialize_int( stream, timestamp, 0, 65535 );
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

    DisconnectPacket() : Packet( PACKET_Disconnect ) 
    {
        x = 2;
    }

    void Serialize( Stream & stream )
    {
        serialize_int( stream, x, -100, +100 );
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

enum PacketType
{
    PACKET_Connection,           // for connection class
    PACKET_Connect,
    PACKET_Update,
    PACKET_Disconnect
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
*/

void test_factory()
{
    cout << "test_factory" << endl;

    /*
    PacketFactory factory;

    auto connectPacket = factory.Create( PACKET_Connect );
    auto updatePacket = factory.Create( PACKET_Update );
    auto disconnectPacket = factory.Create( PACKET_Disconnect );

    assert( connectPacket->GetType() == PACKET_Connect );
    assert( updatePacket->GetType() == PACKET_Update );
    assert( disconnectPacket->GetType() == PACKET_Disconnect );
    */
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
