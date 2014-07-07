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

void test_endian()
{
    printf( "test_endian\n" );

    union
    {
        uint8_t bytes[4];
        uint32_t num;
    } x;

    #if PROTOCOL_ENDIAN == PROTOCOL_LITTLE_ENDIAN

        x.bytes[0] = 7; 
        x.bytes[1] = 5; 
        x.bytes[2] = 3; 
        x.bytes[3] = 1;
    
    #elif SOPHIST_endian == SOPHIST_big_endian
    
        x.bytes[0] = 1; 
        x.bytes[1] = 3; 
        x.bytes[2] = 5; 
        x.bytes[3] = 7;
    
    #else
    
        #error endianness is not known!

    #endif

    check( x.num == 0x01030507 );
}
