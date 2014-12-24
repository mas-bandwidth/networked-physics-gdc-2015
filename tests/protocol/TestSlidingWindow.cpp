#include "protocol/SlidingWindow.h"
#include "core/Memory.h"

struct TestEntry
{
    TestEntry() { sequence = 0xFFFF; }
    TestEntry( uint16_t s ) { sequence = s; }
    uint16_t sequence;
};

void test_real_sliding_window()
{
    printf( "test_real_sliding_window\n" );

    core::memory::initialize();
    {
        const int Size = 256;

        protocol::RealSlidingWindow<TestEntry> sliding_window( core::memory::default_allocator(), Size );

        // verify initial conditions

        CORE_CHECK( sliding_window.GetSize() == Size );
        CORE_CHECK( sliding_window.GetNumEntries() == 0 );
        CORE_CHECK( sliding_window.GetAck() == 0xFFFF );
        CORE_CHECK( sliding_window.GetSequence() == 0 );
        CORE_CHECK( sliding_window.IsEmpty() );
        CORE_CHECK( !sliding_window.IsFull() );
        CORE_CHECK( !sliding_window.IsValid( 0xFFFF ) );
        CORE_CHECK( !sliding_window.IsValid( 0 ) );
        CORE_CHECK( !sliding_window.IsValid( 1 ) );

        {
            TestEntry array[Size];
            int array_length;
            sliding_window.GetArray( array, array_length );
            CORE_CHECK( array_length == 0 );
        }

        // verify insert one entry

        sliding_window.Insert( TestEntry(0) );        

        CORE_CHECK( sliding_window.GetSize() == Size );
        CORE_CHECK( sliding_window.GetNumEntries() == 1 );
        CORE_CHECK( sliding_window.GetAck() == 0xFFFF );
        CORE_CHECK( sliding_window.GetSequence() == 1 );
        CORE_CHECK( sliding_window.GetBegin() == 0 );
        CORE_CHECK( sliding_window.GetEnd() == 1 );
        CORE_CHECK( !sliding_window.IsEmpty() );
        CORE_CHECK( !sliding_window.IsFull() );
        CORE_CHECK( !sliding_window.IsValid( 0xFFFF ) );
        CORE_CHECK( sliding_window.IsValid( 0 ) );
        CORE_CHECK( !sliding_window.IsValid( 1 ) );

        {
            TestEntry array[Size];
            int array_length;
            sliding_window.GetArray( array, array_length );
            CORE_CHECK( array[0].sequence == 0 );
            CORE_CHECK( array_length == 1 );
        }

        // ack that one entry and make sure everything is correct

        sliding_window.Ack( 0 );

        CORE_CHECK( sliding_window.GetSize() == Size );
        CORE_CHECK( sliding_window.GetNumEntries() == 0 );
        CORE_CHECK( sliding_window.GetAck() == 0 );
        CORE_CHECK( sliding_window.GetSequence() == 1 );
        CORE_CHECK( sliding_window.IsEmpty() );
        CORE_CHECK( !sliding_window.IsFull() );
        CORE_CHECK( !sliding_window.IsValid( 0xFFFF ) );
        CORE_CHECK( !sliding_window.IsValid( 0 ) );
        CORE_CHECK( !sliding_window.IsValid( 1 ) );
        CORE_CHECK( !sliding_window.IsValid( 2 ) );

        {
            TestEntry array[Size];
            int array_length;
            sliding_window.GetArray( array, array_length );
            CORE_CHECK( array_length == 0 );
        }

        // add 200 more entries and verify everything is correct

        for ( int i = 0; i < 200; ++i )
            sliding_window.Insert( TestEntry(i+1) );        

        CORE_CHECK( sliding_window.GetSize() == Size );
        CORE_CHECK( sliding_window.GetNumEntries() == 200 );
        CORE_CHECK( sliding_window.GetAck() == 0 );
        CORE_CHECK( sliding_window.GetSequence() == 1+200 );
        CORE_CHECK( sliding_window.GetBegin() == 1 );
        CORE_CHECK( sliding_window.GetEnd() == 201 );
        CORE_CHECK( !sliding_window.IsEmpty() );
        CORE_CHECK( !sliding_window.IsFull() );

        {
            TestEntry array[Size];
            int array_length;
            sliding_window.GetArray( array, array_length );
            CORE_CHECK( array_length == 200 );
            for ( int i = 0; i < 200; ++i )
                CORE_CHECK( array[i].sequence == i + 1 );
        }

        // ack 200 entries in a row and check at each iteration that everything is correct

        for ( int i = 0; i < 200; ++i )
        {
            sliding_window.Ack( i + 1 );

            CORE_CHECK( sliding_window.GetSize() == Size );
            CORE_CHECK( sliding_window.GetNumEntries() == 200 - (i+1) );
            CORE_CHECK( sliding_window.GetAck() == i+1 );
            CORE_CHECK( sliding_window.GetSequence() == 1+200 );
            CORE_CHECK( sliding_window.GetBegin() == i+2 );
            CORE_CHECK( sliding_window.GetEnd() == 201 );
            CORE_CHECK( sliding_window.IsEmpty() == ( i == 199 ) );
            CORE_CHECK( !sliding_window.IsFull() );

            {
                TestEntry array[Size];
                int array_length;
                sliding_window.GetArray( array, array_length );
                CORE_CHECK( array_length == 200 - (i+1) );
                for ( int j = 0; j < array_length; ++j )
                    CORE_CHECK( array[j].sequence == 2 + i + j );
            }
        }

        // add 100 entries across the sliding window array wrap and check that everything is OK

        for ( int i = 0; i < 100; ++i )
            sliding_window.Insert( TestEntry(200+i+1) );        

        CORE_CHECK( sliding_window.GetSize() == Size );
        CORE_CHECK( sliding_window.GetNumEntries() == 100 );
        CORE_CHECK( sliding_window.GetAck() == 200 );
        CORE_CHECK( sliding_window.GetSequence() == 1+200+100 );
        CORE_CHECK( sliding_window.GetBegin() == 201 );
        CORE_CHECK( sliding_window.GetEnd() == 301 );
        CORE_CHECK( !sliding_window.IsEmpty() );
        CORE_CHECK( !sliding_window.IsFull() );

        {
            TestEntry array[Size];
            int array_length;
            sliding_window.GetArray( array, array_length );
            CORE_CHECK( array_length == 100 );
            for ( int i = 0; i < 100; ++i )
                CORE_CHECK( array[i].sequence == i + 1 + 200 );
        }

        // now ack everything and verify it's empty again

        sliding_window.Ack( 300 );

        CORE_CHECK( sliding_window.GetSize() == Size );
        CORE_CHECK( sliding_window.GetNumEntries() == 0 );
        CORE_CHECK( sliding_window.GetAck() == 300 );
        CORE_CHECK( sliding_window.GetSequence() == 301 );
        CORE_CHECK( sliding_window.IsEmpty() );
        CORE_CHECK( !sliding_window.IsFull() );

        {
            TestEntry array[Size];
            int array_length;
            sliding_window.GetArray( array, array_length );
            CORE_CHECK( array_length == 0 );
        }

        // now insert size - 1 entries and verify it's full at the end

        for ( int i = 0; i < Size - 1; ++i )
            sliding_window.Insert( TestEntry(i) );

        CORE_CHECK( sliding_window.GetSize() == Size );
        CORE_CHECK( sliding_window.GetNumEntries() == Size - 1 );
        CORE_CHECK( sliding_window.GetAck() == 300 );
        CORE_CHECK( sliding_window.GetSequence() == 301 + Size - 1 );
        CORE_CHECK( !sliding_window.IsEmpty() );
        CORE_CHECK( sliding_window.IsFull() );
    }

    core::memory::shutdown();
}

// --------------------------------------------------------------------------------------------------------

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
