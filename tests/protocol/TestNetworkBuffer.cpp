#include "protocol/SequenceBuffer.h"
#include "core/Memory.h"

struct TestPacketData
{
    TestPacketData()
        : sequence(0xFFFF) {}

    TestPacketData( uint16_t _sequence )
        : sequence( _sequence ) {}

    uint32_t sequence : 16;                 // packet sequence #
};

void test_sequence_buffer()
{
    printf( "test_sequence_buffer\n" );

    core::memory::initialize();
    {
        const int size = 256;

        protocol::SequenceBuffer<TestPacketData> sequence_buffer( core::memory::default_allocator(), size );

        for ( int i = 0; i < size; ++i )
            CORE_CHECK( sequence_buffer.Find(i) == nullptr );

        for ( int i = 0; i <= size*4; ++i )
        {
            auto entry = sequence_buffer.Insert( i );
            entry->sequence = i;
            CORE_CHECK( sequence_buffer.GetSequence() == i + 1 );
        }

        for ( int i = 0; i <= size; ++i )
        {
            // note: outside bounds!
            auto entry = sequence_buffer.Insert( i );
            CORE_CHECK( !entry );
        }    

        int index = size*4;
        for ( int i = 0; i < size; ++i )
        {
            auto entry = sequence_buffer.Find( index );
            CORE_CHECK( entry );
            CORE_CHECK( entry->sequence == index );
            index--;
        }

        sequence_buffer.Reset();

        CORE_CHECK( sequence_buffer.GetSequence() == 0 );

        for ( int i = 0; i < size; ++i )
            CORE_CHECK( sequence_buffer.Find(i) == nullptr );
    }

    core::memory::shutdown();
}

void test_generate_ack_bits()
{
    printf( "test_generate_ack_bits\n" );

    core::memory::initialize();
    {
        const int size = 256;

        protocol::SequenceBuffer<TestPacketData> received_packets( core::memory::default_allocator(), size );

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
