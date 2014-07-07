#include "BitPacker.h"
#include <stdio.h>

using namespace protocol;

void test_bitpacker()
{
    printf( "test_bitpacker\n" );

    const int BufferSize = 256;

    uint8_t buffer[256];

    BitWriter writer( buffer, BufferSize );

    PROTOCOL_CHECK( writer.GetData() == buffer );
    PROTOCOL_CHECK( writer.GetTotalBytes() == BufferSize );
    PROTOCOL_CHECK( writer.GetBitsWritten() == 0 );
    PROTOCOL_CHECK( writer.GetBytesWritten() == 0 );
    PROTOCOL_CHECK( writer.GetBitsAvailable() == BufferSize * 8 );

    writer.WriteBits( 0, 1 );
    writer.WriteBits( 1, 1 );
    writer.WriteBits( 10, 8 );
    writer.WriteBits( 255, 8 );
    writer.WriteBits( 1000, 10 );
    writer.WriteBits( 50000, 16 );
    writer.WriteBits( 9999999, 32 );
    writer.FlushBits();

    const int bitsWritten = 1 + 1 + 8 + 8 + 10 + 16 + 32;

    PROTOCOL_CHECK( writer.GetBytesWritten() == 3*4 );
    PROTOCOL_CHECK( writer.GetTotalBytes() == BufferSize );
    PROTOCOL_CHECK( writer.GetBitsWritten() == bitsWritten );
    PROTOCOL_CHECK( writer.GetBitsAvailable() == BufferSize * 8 - bitsWritten );

    BitReader reader( buffer, BufferSize );

    PROTOCOL_CHECK( reader.GetBitsRead() == 0 );
    PROTOCOL_CHECK( reader.GetBitsRemaining() == BufferSize * 8 );

    uint32_t a = reader.ReadBits( 1 );
    uint32_t b = reader.ReadBits( 1 );
    uint32_t c = reader.ReadBits( 8 );
    uint32_t d = reader.ReadBits( 8 );
    uint32_t e = reader.ReadBits( 10 );
    uint32_t f = reader.ReadBits( 16 );
    uint32_t g = reader.ReadBits( 32 );

    PROTOCOL_CHECK( a == 0 );
    PROTOCOL_CHECK( b == 1 );
    PROTOCOL_CHECK( c == 10 );
    PROTOCOL_CHECK( d == 255 );
    PROTOCOL_CHECK( e == 1000 );
    PROTOCOL_CHECK( f == 50000 );
    PROTOCOL_CHECK( g == 9999999 );

    PROTOCOL_CHECK( reader.GetBitsRead() == bitsWritten );
    PROTOCOL_CHECK( reader.GetBitsRemaining() == BufferSize * 8 - bitsWritten );
}
