#include "protocol/SlidingWindow.h"
#include "core/Memory.h"

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

    core::memory::initialize();
    {
        const int size = 256;

        protocol::SlidingWindow<TestPacketData> slidingWindow( core::memory::default_allocator(), size );

        for ( int i = 0; i < size; ++i )
            CORE_CHECK( slidingWindow.Find(i) == nullptr );

        for ( int i = 0; i <= size*4; ++i )
        {
            slidingWindow.Insert( i );
            CORE_CHECK( slidingWindow.GetSequence() == i + 1 );
        }

        for ( int i = 0; i <= size; ++i )
        {
            // note: outside bounds of sliding window!
            bool insert_succeeded = slidingWindow.Insert( i );
            CORE_CHECK( !insert_succeeded );
        }    

        int index = size*4;
        for ( int i = 0; i < size; ++i )
        {
            auto entry = slidingWindow.Find( index );
            CORE_CHECK( entry );
            CORE_CHECK( entry->valid );
            CORE_CHECK( entry->sequence == index );
            index--;
        }

        slidingWindow.Reset();

        CORE_CHECK( slidingWindow.GetSequence() == 0 );

        for ( int i = 0; i < size; ++i )
            CORE_CHECK( slidingWindow.Find(i) == nullptr );
    }

    core::memory::shutdown();
}

void test_generate_ack_bits()
{
    printf( "test_generate_ack_bits\n" );

    core::memory::initialize();
    {
        const int size = 256;

        protocol::SlidingWindow<TestPacketData> received_packets( core::memory::default_allocator(), size );

        uint16_t ack = -1;
        uint32_t ack_bits = -1;

        GenerateAckBits( received_packets, ack, ack_bits );
        CORE_CHECK( ack == 0xFFFF );
        CORE_CHECK( ack_bits == 0 );

        for ( int i = 0; i <= size; ++i )
            received_packets.Insert( i );

        GenerateAckBits( received_packets, ack, ack_bits );
        CORE_CHECK( ack == size );
        CORE_CHECK( ack_bits == 0xFFFFFFFF );

        received_packets.Reset();
        uint16_t input_acks[] = { 1, 5, 9, 11 };
        int input_num_acks = sizeof( input_acks ) / sizeof( uint16_t );
        for ( int i = 0; i < input_num_acks; ++i )
            received_packets.Insert( input_acks[i] );

        GenerateAckBits( received_packets, ack, ack_bits );

        CORE_CHECK( ack == 11 );
        CORE_CHECK( ack_bits == ( 1 | (1<<(11-9)) | (1<<(11-5)) | (1<<(11-1)) ) );
    }

    core::memory::shutdown();
}
