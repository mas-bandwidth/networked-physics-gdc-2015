#include "Common.h"

using namespace protocol;

void test_sequence()
{
    printf( "test_sequence\n" );

    assert( sequence_greater_than( 0, 0 ) == false );
    assert( sequence_greater_than( 1, 0 ) == true );
    assert( sequence_greater_than( 0, -1 ) == true );

    assert( sequence_less_than( 0, 0 ) == false );
    assert( sequence_less_than( 0, 1 ) == true );
    assert( sequence_less_than( -1, 0 ) == true );
}
