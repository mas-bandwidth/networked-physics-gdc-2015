#include "protocol/BitArray.h"
#include "core/Memory.h"

void test_bit_array()
{
    printf( "test_bit_array\n" );

    core::memory::initialize();
    {
        const int Size = 300;

        protocol::BitArray bit_array( core::memory::default_allocator(), Size );

        // verify initial conditions

        CORE_CHECK( bit_array.GetSize() == Size );

        for ( int i = 0; i < Size; ++i )
            CORE_CHECK( bit_array.GetBit(i) == 0 );

        // set every third bit and verify correct bits are set on read

        for ( int i = 0; i < Size; ++i )
        {
            if ( ( i % 3 ) == 0 )
                bit_array.SetBit( i );
        }

        for ( int i = 0; i < Size; ++i )
        {
            if ( ( i % 3 ) == 0 )
            {
                CORE_CHECK( bit_array.GetBit( i ) == 1 );
            }
            else
            {
                CORE_CHECK( bit_array.GetBit( i ) == 0 );
            }
        }

        // now clear every third bit to zero and verify all bits are zero

        for ( int i = 0; i < Size; ++i )
        {
            if ( ( i % 3 ) == 0 )
                bit_array.ClearBit( i );
        }

        for ( int i = 0; i < Size; ++i )
            CORE_CHECK( bit_array.GetBit(i) == 0 );

        // now set some more bits

        for ( int i = 0; i < Size; ++i )
        {
            if ( ( i % 10 ) == 0 )
                bit_array.SetBit( i );
        }

        for ( int i = 0; i < Size; ++i )
        {
            if ( ( i % 10 ) == 0 )
            {
                CORE_CHECK( bit_array.GetBit( i ) == 1 );
            }
            else
            {
                CORE_CHECK( bit_array.GetBit( i ) == 0 );
            }
        }

        // clear and verify all bits are zero

        bit_array.Clear();

        for ( int i = 0; i < Size; ++i )
            CORE_CHECK( bit_array.GetBit(i) == 0 );
    }

    core::memory::shutdown();
}
