#include "BitPacker.h"

using namespace std;
using namespace protocol;

void test_bitpacker()
{
    cout << "test_bitpacker" << endl;

    const int BufferSize = 256;

    uint8_t buffer[256];

    BitWriter writer( buffer, BufferSize );

    assert( writer.GetData() == buffer );
    assert( writer.GetBytes() == 0 );
    assert( writer.GetBitsWritten() == 0 );
    assert( writer.GetBitsAvailable() == BufferSize * 8 );

    writer.WriteBits( 0, 1 );
    writer.WriteBits( 1, 1 );
    writer.WriteBits( 10, 8 );
    writer.WriteBits( 255, 8 );
    writer.WriteBits( 1000, 10 );
    writer.WriteBits( 50000, 16 );
    writer.WriteBits( 9999999, 32 );
    writer.FlushBits();

    const int bitsWritten = 1 + 1 + 8 + 8 + 10 + 16 + 32;

    assert( writer.GetBytes() == 3 * 4 );
    assert( writer.GetBitsWritten() == bitsWritten );
    assert( writer.GetBitsAvailable() == BufferSize * 8 - bitsWritten );

    BitReader reader( buffer, BufferSize );

    assert( reader.GetBitsRead() == 0 );
    assert( reader.GetBitsRemaining() == BufferSize * 8 );

    uint32_t a = reader.ReadBits( 1 );
    uint32_t b = reader.ReadBits( 1 );
    uint32_t c = reader.ReadBits( 8 );
    uint32_t d = reader.ReadBits( 8 );
    uint32_t e = reader.ReadBits( 10 );
    uint32_t f = reader.ReadBits( 16 );
    uint32_t g = reader.ReadBits( 32 );

    assert( a == 0 );
    assert( b == 1 );
    assert( c == 10 );
    assert( d == 255 );
    assert( e == 1000 );
    assert( f == 50000 );
    assert( g == 9999999 );

    assert( reader.GetBitsRead() == bitsWritten );
    assert( reader.GetBitsRemaining() == BufferSize * 8 - bitsWritten );
}

int main()
{
    try
    {
        test_bitpacker();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
