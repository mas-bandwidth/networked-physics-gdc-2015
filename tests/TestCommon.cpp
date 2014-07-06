#include "Common.h"

using namespace protocol;

void test_sequence()
{
    printf( "test_sequence\n" );

    check( sequence_greater_than( 0, 0 ) == false );
    check( sequence_greater_than( 1, 0 ) == true );
    check( sequence_greater_than( 0, -1 ) == true );

    check( sequence_less_than( 0, 0 ) == false );
    check( sequence_less_than( 0, 1 ) == true );
    check( sequence_less_than( -1, 0 ) == true );
}
