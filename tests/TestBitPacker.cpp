#include "BitPacker.h"

using namespace std;
using namespace protocol;

void test_bitpacker()
{
    cout << "test_bitpacker" << endl;

    const int BufferSize = 256;

    uint8_t writeBuffer[256];

    BitWriter writer( writeBuffer, BufferSize );

    assert( writer.GetData() == writeBuffer );
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

    // todo: bit reader... etc.

    //uint8_t readBuffer[BufferSize];
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
