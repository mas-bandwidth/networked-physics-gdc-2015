#include "protocol/RingBuffer.h"
#include "core/Memory.h"

struct TestEntry
{
    TestEntry() { sequence = 0xFFFF; }
    TestEntry( uint16_t s ) { sequence = s; }
    uint16_t sequence;
};

void test_ring_buffer()
{
    printf( "test_ring_buffer\n" );

    core::memory::initialize();
    {
        const int Size = 256;

        protocol::RingBuffer<TestEntry> ring_buffer( core::memory::default_allocator(), Size );

        // verify initial conditions

        CORE_CHECK( ring_buffer.GetSize() == Size );
        CORE_CHECK( ring_buffer.GetNumEntries() == 0 );
        CORE_CHECK( ring_buffer.GetAck() == 0xFFFF );
        CORE_CHECK( ring_buffer.GetSequence() == 0 );
        CORE_CHECK( ring_buffer.IsEmpty() );
        CORE_CHECK( !ring_buffer.IsFull() );
        CORE_CHECK( !ring_buffer.IsValid( 0xFFFF ) );
        CORE_CHECK( !ring_buffer.IsValid( 0 ) );
        CORE_CHECK( !ring_buffer.IsValid( 1 ) );

        {
            TestEntry array[Size];
            int array_length;
            ring_buffer.GetArray( array, array_length );
            CORE_CHECK( array_length == 0 );
        }

        // verify insert one entry

        ring_buffer.Insert( TestEntry(0) );        

        CORE_CHECK( ring_buffer.GetSize() == Size );
        CORE_CHECK( ring_buffer.GetNumEntries() == 1 );
        CORE_CHECK( ring_buffer.GetAck() == 0xFFFF );
        CORE_CHECK( ring_buffer.GetSequence() == 1 );
        CORE_CHECK( ring_buffer.GetBegin() == 0 );
        CORE_CHECK( ring_buffer.GetEnd() == 1 );
        CORE_CHECK( !ring_buffer.IsEmpty() );
        CORE_CHECK( !ring_buffer.IsFull() );
        CORE_CHECK( !ring_buffer.IsValid( 0xFFFF ) );
        CORE_CHECK( ring_buffer.IsValid( 0 ) );
        CORE_CHECK( !ring_buffer.IsValid( 1 ) );

        {
            TestEntry array[Size];
            int array_length;
            ring_buffer.GetArray( array, array_length );
            CORE_CHECK( array[0].sequence == 0 );
            CORE_CHECK( array_length == 1 );
        }

        // ack that one entry and make sure everything is correct

        ring_buffer.Ack( 0 );

        CORE_CHECK( ring_buffer.GetSize() == Size );
        CORE_CHECK( ring_buffer.GetNumEntries() == 0 );
        CORE_CHECK( ring_buffer.GetAck() == 0 );
        CORE_CHECK( ring_buffer.GetSequence() == 1 );
        CORE_CHECK( ring_buffer.IsEmpty() );
        CORE_CHECK( !ring_buffer.IsFull() );
        CORE_CHECK( !ring_buffer.IsValid( 0xFFFF ) );
        CORE_CHECK( !ring_buffer.IsValid( 0 ) );
        CORE_CHECK( !ring_buffer.IsValid( 1 ) );
        CORE_CHECK( !ring_buffer.IsValid( 2 ) );

        {
            TestEntry array[Size];
            int array_length;
            ring_buffer.GetArray( array, array_length );
            CORE_CHECK( array_length == 0 );
        }

        // add 200 more entries and verify everything is correct

        for ( int i = 0; i < 200; ++i )
            ring_buffer.Insert( TestEntry(i+1) );        

        CORE_CHECK( ring_buffer.GetSize() == Size );
        CORE_CHECK( ring_buffer.GetNumEntries() == 200 );
        CORE_CHECK( ring_buffer.GetAck() == 0 );
        CORE_CHECK( ring_buffer.GetSequence() == 1+200 );
        CORE_CHECK( ring_buffer.GetBegin() == 1 );
        CORE_CHECK( ring_buffer.GetEnd() == 201 );
        CORE_CHECK( !ring_buffer.IsEmpty() );
        CORE_CHECK( !ring_buffer.IsFull() );

        {
            TestEntry array[Size];
            int array_length;
            ring_buffer.GetArray( array, array_length );
            CORE_CHECK( array_length == 200 );
            for ( int i = 0; i < 200; ++i )
                CORE_CHECK( array[i].sequence == i + 1 );
        }

        // ack 200 entries in a row and check at each iteration that everything is correct

        for ( int i = 0; i < 200; ++i )
        {
            ring_buffer.Ack( i + 1 );

            CORE_CHECK( ring_buffer.GetSize() == Size );
            CORE_CHECK( ring_buffer.GetNumEntries() == 200 - (i+1) );
            CORE_CHECK( ring_buffer.GetAck() == i+1 );
            CORE_CHECK( ring_buffer.GetSequence() == 1+200 );
            CORE_CHECK( ring_buffer.GetBegin() == i+2 );
            CORE_CHECK( ring_buffer.GetEnd() == 201 );
            CORE_CHECK( ring_buffer.IsEmpty() == ( i == 199 ) );
            CORE_CHECK( !ring_buffer.IsFull() );

            {
                TestEntry array[Size];
                int array_length;
                ring_buffer.GetArray( array, array_length );
                CORE_CHECK( array_length == 200 - (i+1) );
                for ( int j = 0; j < array_length; ++j )
                    CORE_CHECK( array[j].sequence == 2 + i + j );
            }
        }

        // add 100 entries across the sliding window array wrap and check that everything is OK

        for ( int i = 0; i < 100; ++i )
            ring_buffer.Insert( TestEntry(200+i+1) );        

        CORE_CHECK( ring_buffer.GetSize() == Size );
        CORE_CHECK( ring_buffer.GetNumEntries() == 100 );
        CORE_CHECK( ring_buffer.GetAck() == 200 );
        CORE_CHECK( ring_buffer.GetSequence() == 1+200+100 );
        CORE_CHECK( ring_buffer.GetBegin() == 201 );
        CORE_CHECK( ring_buffer.GetEnd() == 301 );
        CORE_CHECK( !ring_buffer.IsEmpty() );
        CORE_CHECK( !ring_buffer.IsFull() );

        {
            TestEntry array[Size];
            int array_length;
            ring_buffer.GetArray( array, array_length );
            CORE_CHECK( array_length == 100 );
            for ( int i = 0; i < 100; ++i )
                CORE_CHECK( array[i].sequence == i + 1 + 200 );
        }

        // now ack everything and verify it's empty again

        ring_buffer.Ack( 300 );

        CORE_CHECK( ring_buffer.GetSize() == Size );
        CORE_CHECK( ring_buffer.GetNumEntries() == 0 );
        CORE_CHECK( ring_buffer.GetAck() == 300 );
        CORE_CHECK( ring_buffer.GetSequence() == 301 );
        CORE_CHECK( ring_buffer.IsEmpty() );
        CORE_CHECK( !ring_buffer.IsFull() );

        {
            TestEntry array[Size];
            int array_length;
            ring_buffer.GetArray( array, array_length );
            CORE_CHECK( array_length == 0 );
        }

        // now insert size - 1 entries and verify it's full at the end

        for ( int i = 0; i < Size - 1; ++i )
            ring_buffer.Insert( TestEntry(i) );

        CORE_CHECK( ring_buffer.GetSize() == Size );
        CORE_CHECK( ring_buffer.GetNumEntries() == Size - 1 );
        CORE_CHECK( ring_buffer.GetAck() == 300 );
        CORE_CHECK( ring_buffer.GetSequence() == 301 + Size - 1 );
        CORE_CHECK( !ring_buffer.IsEmpty() );
        CORE_CHECK( ring_buffer.IsFull() );
    }

    core::memory::shutdown();
}
