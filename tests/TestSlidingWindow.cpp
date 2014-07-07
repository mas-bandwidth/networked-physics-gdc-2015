#include "SlidingWindow.h"
#include "Memory.h"

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

    memory::initialize();
    {
        const int size = 256;

        SlidingWindow<TestPacketData> slidingWindow( memory::default_allocator(), size );

        for ( int i = 0; i < size; ++i )
            PROTOCOL_CHECK( slidingWindow.Find(i) == nullptr );

        for ( int i = 0; i <= size*4; ++i )
        {
            slidingWindow.Insert( i );
            PROTOCOL_CHECK( slidingWindow.GetSequence() == i + 1 );
        }

        for ( int i = 0; i <= size; ++i )
        {
            // note: outside bounds of sliding window!
            bool insert_succeeded = slidingWindow.Insert( i );
            PROTOCOL_CHECK( !insert_succeeded );
        }    

        int index = size*4;
        for ( int i = 0; i < size; ++i )
        {
            auto entry = slidingWindow.Find( index );
            PROTOCOL_CHECK( entry );
            PROTOCOL_CHECK( entry->valid );
            PROTOCOL_CHECK( entry->sequence == index );
            index--;
        }

        slidingWindow.Reset();

        PROTOCOL_CHECK( slidingWindow.GetSequence() == 0 );

        for ( int i = 0; i < size; ++i )
            PROTOCOL_CHECK( slidingWindow.Find(i) == nullptr );
    }

    memory::shutdown();
}

void test_generate_ack_bits()
{
    printf( "test_generate_ack_bits\n" );

    memory::initialize();
    {
        const int size = 256;

        SlidingWindow<TestPacketData> received_packets( memory::default_allocator(), size );

        uint16_t ack = -1;
        uint32_t ack_bits = -1;

        GenerateAckBits( received_packets, ack, ack_bits );
        PROTOCOL_CHECK( ack == 0xFFFF );
        PROTOCOL_CHECK( ack_bits == 0 );

        for ( int i = 0; i <= size; ++i )
            received_packets.Insert( i );

        GenerateAckBits( received_packets, ack, ack_bits );
        PROTOCOL_CHECK( ack == size );
        PROTOCOL_CHECK( ack_bits == 0xFFFFFFFF );

        received_packets.Reset();
        uint16_t input_acks[] = { 1, 5, 9, 11 };
        int input_num_acks = sizeof( input_acks ) / sizeof( uint16_t );
        for ( int i = 0; i < input_num_acks; ++i )
            received_packets.Insert( input_acks[i] );

        GenerateAckBits( received_packets, ack, ack_bits );

        PROTOCOL_CHECK( ack == 11 );
        PROTOCOL_CHECK( ack_bits == ( 1 | (1<<(11-9)) | (1<<(11-5)) | (1<<(11-1)) ) );
    }

    memory::shutdown();
}
