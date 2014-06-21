#include "SlidingWindow.h"

using namespace protocol;

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
