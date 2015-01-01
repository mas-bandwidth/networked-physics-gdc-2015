#include "protocol/SlidingWindow.h"
#include "core/Memory.h"

struct TestEntry
{
    TestEntry() { sequence = 0xFFFF; }
    TestEntry( uint16_t s ) { sequence = s; }
    uint16_t sequence;
};

void test_sliding_window()
{
    printf( "test_sliding_window\n" );

    core::memory::initialize();
    {
        const int Size = 256;

        protocol::SlidingWindow<TestEntry> sliding_window( core::memory::default_allocator(), Size );

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
