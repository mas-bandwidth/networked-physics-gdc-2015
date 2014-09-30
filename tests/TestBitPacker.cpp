#include "protocol/BitPacker.h"
#include <stdio.h>

void test_bitpacker()
{
    printf( "test_bitpacker\n" );

    const int BufferSize = 256;

    uint8_t buffer[256];

    protocol::BitWriter writer( buffer, BufferSize );

    CORE_CHECK( writer.GetData() == buffer );
    CORE_CHECK( writer.GetTotalBytes() == BufferSize );
    CORE_CHECK( writer.GetBitsWritten() == 0 );
    CORE_CHECK( writer.GetBytesWritten() == 0 );
    CORE_CHECK( writer.GetBitsAvailable() == BufferSize * 8 );

    writer.WriteBits( 0, 1 );
    writer.WriteBits( 1, 1 );
    writer.WriteBits( 10, 8 );
    writer.WriteBits( 255, 8 );
    writer.WriteBits( 1000, 10 );
    writer.WriteBits( 50000, 16 );
    writer.WriteBits( 9999999, 32 );
    writer.FlushBits();

    const int bitsWritten = 1 + 1 + 8 + 8 + 10 + 16 + 32;

    CORE_CHECK( writer.GetBytesWritten() == 3*4 );
    CORE_CHECK( writer.GetTotalBytes() == BufferSize );
    CORE_CHECK( writer.GetBitsWritten() == bitsWritten );
    CORE_CHECK( writer.GetBitsAvailable() == BufferSize * 8 - bitsWritten );

    protocol::BitReader reader( buffer, BufferSize );

    CORE_CHECK( reader.GetBitsRead() == 0 );
    CORE_CHECK( reader.GetBitsRemaining() == BufferSize * 8 );

    uint32_t a = reader.ReadBits( 1 );
    uint32_t b = reader.ReadBits( 1 );
    uint32_t c = reader.ReadBits( 8 );
    uint32_t d = reader.ReadBits( 8 );
    uint32_t e = reader.ReadBits( 10 );
    uint32_t f = reader.ReadBits( 16 );
    uint32_t g = reader.ReadBits( 32 );

    CORE_CHECK( a == 0 );
    CORE_CHECK( b == 1 );
    CORE_CHECK( c == 10 );
    CORE_CHECK( d == 255 );
    CORE_CHECK( e == 1000 );
    CORE_CHECK( f == 50000 );
    CORE_CHECK( g == 9999999 );

    CORE_CHECK( reader.GetBitsRead() == bitsWritten );
    CORE_CHECK( reader.GetBitsRemaining() == BufferSize * 8 - bitsWritten );
}
