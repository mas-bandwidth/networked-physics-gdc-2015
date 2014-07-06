#include "BitPacker.h"

using namespace protocol;

void test_bitpacker()
{
    printf( "test_bitpacker\n" );

    const int BufferSize = 256;

    uint8_t buffer[256];

    BitWriter writer( buffer, BufferSize );

    check( writer.GetData() == buffer );
    check( writer.GetTotalBytes() == BufferSize );
    check( writer.GetBitsWritten() == 0 );
    check( writer.GetBytesWritten() == 0 );
    check( writer.GetBitsAvailable() == BufferSize * 8 );

    writer.WriteBits( 0, 1 );
    writer.WriteBits( 1, 1 );
    writer.WriteBits( 10, 8 );
    writer.WriteBits( 255, 8 );
    writer.WriteBits( 1000, 10 );
    writer.WriteBits( 50000, 16 );
    writer.WriteBits( 9999999, 32 );
    writer.FlushBits();

    const int bitsWritten = 1 + 1 + 8 + 8 + 10 + 16 + 32;

    check( writer.GetBytesWritten() == 3*4 );
    check( writer.GetTotalBytes() == BufferSize );
    check( writer.GetBitsWritten() == bitsWritten );
    check( writer.GetBitsAvailable() == BufferSize * 8 - bitsWritten );

    BitReader reader( buffer, BufferSize );

    check( reader.GetBitsRead() == 0 );
    check( reader.GetBitsRemaining() == BufferSize * 8 );

    uint32_t a = reader.ReadBits( 1 );
    uint32_t b = reader.ReadBits( 1 );
    uint32_t c = reader.ReadBits( 8 );
    uint32_t d = reader.ReadBits( 8 );
    uint32_t e = reader.ReadBits( 10 );
    uint32_t f = reader.ReadBits( 16 );
    uint32_t g = reader.ReadBits( 32 );

    check( a == 0 );
    check( b == 1 );
    check( c == 10 );
    check( d == 255 );
    check( e == 1000 );
    check( f == 50000 );
    check( g == 9999999 );

    check( reader.GetBitsRead() == bitsWritten );
    check( reader.GetBitsRemaining() == BufferSize * 8 - bitsWritten );
}
