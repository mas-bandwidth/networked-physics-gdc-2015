// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#include "BitPacker.h"

namespace protocol
{
    BitWriter::BitWriter( void * data, int bytes )
        : m_data( (uint32_t*)data ), m_numWords( bytes / 4 )
    {
        CORE_ASSERT( data );
        CORE_ASSERT( ( bytes % 4 ) == 0 );           // IMPORTANT: buffer size must be a multiple of four!
        m_numBits = m_numWords * 32;
        m_bitsWritten = 0;
        m_scratch = 0;
        m_bitIndex = 0;
        m_wordIndex = 0;
        m_overflow = false;
    }

    void BitWriter::WriteBits( uint32_t value, int bits )
    {
        CORE_ASSERT( bits > 0 );
        CORE_ASSERT( bits <= 32 );
        CORE_ASSERT( m_bitsWritten + bits <= m_numBits );

        if ( m_bitsWritten + bits > m_numBits )
        {
            m_overflow = true;
            return;
        }

        value &= ( uint64_t( 1 ) << bits ) - 1;

        m_scratch |= uint64_t( value ) << ( 64 - m_bitIndex - bits );

        m_bitIndex += bits;

        if ( m_bitIndex >= 32 )
        {
            CORE_ASSERT( m_wordIndex < m_numWords );
            m_data[m_wordIndex] = core::host_to_network( uint32_t( m_scratch >> 32 ) );
            m_scratch <<= 32;
            m_bitIndex -= 32;
            m_wordIndex++;
        }

        m_bitsWritten += bits;
    }

    void BitWriter::WriteAlign()
    {
        const int remainderBits = m_bitsWritten % 8;
        if ( remainderBits != 0 )
        {
            uint32_t zero = 0;
            WriteBits( zero, 8 - remainderBits );
            CORE_ASSERT( m_bitsWritten % 8 == 0 );
        }
    }

    void BitWriter::WriteBytes( const uint8_t * data, int bytes )
    {
        CORE_ASSERT( GetAlignBits() == 0 );
        if ( m_bitsWritten + bytes * 8 >= m_numBits )
        {
            m_overflow = true;
            return;
        }

        // write head bytes

        CORE_ASSERT( m_bitIndex == 0 || m_bitIndex == 8 || m_bitIndex == 16 || m_bitIndex == 24 );

        int headBytes = ( 4 - m_bitIndex / 8 ) % 4;
        if ( headBytes > bytes )
            headBytes = bytes;
        for ( int i = 0; i < headBytes; ++i )
            WriteBits( data[i], 8 );
        if ( headBytes == bytes )
            return;

        CORE_ASSERT( GetAlignBits() == 0 );

        // write words

        int numWords = ( bytes - headBytes ) / 4;
        if ( numWords > 0 )
        {
            CORE_ASSERT( m_bitIndex == 0 );
            memcpy( &m_data[m_wordIndex], data + headBytes, numWords * 4 );
            m_bitsWritten += numWords * 32;
            m_wordIndex += numWords;
            m_scratch = 0;
        }

        CORE_ASSERT( GetAlignBits() == 0 );

        // write tail

        int tailStart = headBytes + numWords * 4;
        int tailBytes = bytes - tailStart;
        CORE_ASSERT( tailBytes >= 0 && tailBytes < 4 );
        for ( int i = 0; i < tailBytes; ++i )
            WriteBits( data[tailStart+i], 8 );

        CORE_ASSERT( GetAlignBits() == 0 );

        CORE_ASSERT( headBytes + numWords * 4 + tailBytes == bytes );
    }

    void BitWriter::FlushBits()
    {
        if ( m_bitIndex != 0 )
        {
            CORE_ASSERT( m_wordIndex < m_numWords );
            if ( m_wordIndex >= m_numWords )
            {
                m_overflow = true;
                return;
            }
            m_data[m_wordIndex++] = core::host_to_network( uint32_t( m_scratch >> 32 ) );
        }
    }

    BitReader::BitReader( const void * data, int bytes )
        : m_data( (const uint32_t*)data ), m_numWords( bytes / 4 )
    {
        CORE_ASSERT( data );
        CORE_ASSERT( ( bytes % 4 ) == 0 );           // IMPORTANT: buffer size must be a multiple of four!
        m_numBits = m_numWords * 32;
        m_bitsRead = 0;
        m_bitIndex = 0;
        m_wordIndex = 0;
        m_scratch = core::network_to_host( m_data[0] );
        m_overflow = false;
    }

    uint32_t BitReader::ReadBits( int bits )
    {
        CORE_ASSERT( bits > 0 );
        CORE_ASSERT( bits <= 32 );
        CORE_ASSERT( m_bitsRead + bits <= m_numBits );

        if ( m_bitsRead + bits > m_numBits )
        {
            m_overflow = true;
            return 0;
        }

        m_bitsRead += bits;

        CORE_ASSERT( m_bitIndex < 32 );

        if ( m_bitIndex + bits < 32 )
        {
            m_scratch <<= bits;
            m_bitIndex += bits;
        }
        else
        {
            m_wordIndex++;
            CORE_ASSERT( m_wordIndex < m_numWords );
            const uint32_t a = 32 - m_bitIndex;
            const uint32_t b = bits - a;
            m_scratch <<= a;
            m_scratch |= core::network_to_host( m_data[m_wordIndex] );
            m_scratch <<= b;
            m_bitIndex = b;
        }

        const uint32_t output = uint32_t( m_scratch >> 32 );

        m_scratch &= 0xFFFFFFFF;

        return output;
    }

    void BitReader::ReadAlign()
    {
        const int remainderBits = m_bitsRead % 8;
        if ( remainderBits != 0 )
        {
            #ifdef NDEBUG
            ReadBits( 8 - remainderBits );
            #else
            uint32_t value = ReadBits( 8 - remainderBits );
            CORE_ASSERT( value == 0 );
            CORE_ASSERT( m_bitsRead % 8 == 0 );
            #endif
        }
    }

    void BitReader::ReadBytes( uint8_t * data, int bytes )
    {
        CORE_ASSERT( GetAlignBits() == 0 );

        if ( m_bitsRead + bytes * 8 >= m_numBits )
        {
            memset( data, bytes, 0 );
            m_overflow = true;
            return;
        }

        // read head bytes

        CORE_ASSERT( m_bitIndex == 0 || m_bitIndex == 8 || m_bitIndex == 16 || m_bitIndex == 24 );

        int headBytes = ( 4 - m_bitIndex / 8 ) % 4;
        if ( headBytes > bytes )
            headBytes = bytes;
        for ( int i = 0; i < headBytes; ++i )
            data[i] = ReadBits( 8 );
        if ( headBytes == bytes )
            return;

        CORE_ASSERT( GetAlignBits() == 0 );

        // read words

        int numWords = ( bytes - headBytes ) / 4;
        if ( numWords > 0 )
        {
            CORE_ASSERT( m_bitIndex == 0 );
            memcpy( data + headBytes, &m_data[m_wordIndex], numWords * 4 );
            m_bitsRead += numWords * 32;
            m_wordIndex += numWords;
            m_scratch = core::network_to_host( m_data[m_wordIndex] );
        }

        CORE_ASSERT( GetAlignBits() == 0 );

        // write tail

        int tailStart = headBytes + numWords * 4;
        int tailBytes = bytes - tailStart;
        CORE_ASSERT( tailBytes >= 0 && tailBytes < 4 );
        for ( int i = 0; i < tailBytes; ++i )
            data[tailStart+i] = ReadBits( 8 );

        CORE_ASSERT( GetAlignBits() == 0 );

        CORE_ASSERT( headBytes + numWords * 4 + tailBytes == bytes );
    }
}
