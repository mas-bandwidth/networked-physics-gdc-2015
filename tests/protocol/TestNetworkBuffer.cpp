#include "protocol/NetworkBuffer.h"
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

void test_network_buffer()
{
    printf( "test_network_buffer\n" );

    core::memory::initialize();
    {
        const int size = 256;

        protocol::NetworkBuffer<TestPacketData> network_buffer( core::memory::default_allocator(), size );

        for ( int i = 0; i < size; ++i )
            CORE_CHECK( network_buffer.Find(i) == nullptr );

        for ( int i = 0; i <= size*4; ++i )
        {
            network_buffer.Insert( i );
            CORE_CHECK( network_buffer.GetSequence() == i + 1 );
        }

        for ( int i = 0; i <= size; ++i )
        {
            // note: outside bounds of sliding window!
            bool insert_succeeded = network_buffer.Insert( i );
            CORE_CHECK( !insert_succeeded );
        }    

        int index = size*4;
        for ( int i = 0; i < size; ++i )
        {
            auto entry = network_buffer.Find( index );
            CORE_CHECK( entry );
            CORE_CHECK( entry->valid );
            CORE_CHECK( entry->sequence == index );
            index--;
        }

        network_buffer.Reset();

        CORE_CHECK( network_buffer.GetSequence() == 0 );

        for ( int i = 0; i < size; ++i )
            CORE_CHECK( network_buffer.Find(i) == nullptr );
    }

    core::memory::shutdown();
}

void test_generate_ack_bits()
{
    printf( "test_generate_ack_bits\n" );

    core::memory::initialize();
    {
        const int size = 256;

        protocol::NetworkBuffer<TestPacketData> received_packets( core::memory::default_allocator(), size );

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
