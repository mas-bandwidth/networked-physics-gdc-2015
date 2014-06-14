/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_BITPACKER_H
#define PROTOCOL_BITPACKER_H

#include "Common.h"

namespace protocol
{
    class BitWriter
    {
    public:

        BitWriter( void * data, int bytes )
            : m_data( (uint32_t*)data ), m_numWords( bytes / 4 )
        {
            assert( data );
            assert( ( bytes % 4 ) == 0 );           // IMPORTANT: buffer size must be a multiple of four!
            m_numBits = m_numWords * 32;
            m_bitsWritten = 0;
            m_scratch = 0;
            m_bitIndex = 0;
            m_wordIndex = 0;
            m_overflow = false;
        }

        void WriteBits( uint32_t value, int bits )
        {
            assert( bits > 0 );
            assert( bits <= 32 );

//            if ( m_bitsWritten + bits > m_numBits )
//                printf( "bitpacker overflow: bits written = %d + %d, max %d\n", m_bitsWritten, bits, m_numBits );

            assert( m_bitsWritten + bits <= m_numBits );

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
                assert( m_wordIndex < m_numWords );
//                printf( "write word: %x\n", htonl( m_scratch >> 32 ) );
                m_data[m_wordIndex] = uint32_t( m_scratch >> 32 ); //htonl( uint32_t( m_scratch >> 32 ) );
                m_scratch <<= 32;
                m_bitIndex -= 32;
                m_wordIndex++;
            }

            m_bitsWritten += bits;
        }

        void WriteAlign()
        {
            const int remainderBits = m_bitsWritten % 8;
            if ( remainderBits != 0 )
            {
                uint32_t zero = 0;
                WriteBits( zero, 8 - remainderBits );
                assert( m_bitsWritten % 8 == 0 );
            }
        }

        int GetAlignBits() const
        {
            const int remainderBits = m_bitsWritten % 8;
            return remainderBits ? 8 - remainderBits : 0;
        }

        void WriteBytes( const uint8_t * data, int bytes )
        {
            assert( GetAlignBits() == 0 );
            if ( m_bitsWritten + bytes * 8 >= m_numBits )
            {
                m_overflow = true;
                return;
            }

            // write head bytes

            assert( m_bitIndex == 0 || m_bitIndex == 8 || m_bitIndex == 16 || m_bitIndex == 24 );

            int headBytes = ( 4 - m_bitIndex / 8 ) % 4;
            if ( headBytes > bytes )
                headBytes = bytes;
            for ( int i = 0; i < headBytes; ++i )
                WriteBits( data[i], 8 );
            if ( headBytes == bytes )
                return;

            assert( GetAlignBits() == 0 );

            // write words

            int numWords = ( bytes - headBytes ) / 4;
            if ( numWords > 0 )
            {
                assert( m_bitIndex == 0 );
                memcpy( &m_data[m_wordIndex], data + headBytes, numWords * 4 );
                m_bitsWritten += numWords * 32;
                m_wordIndex += numWords;
                m_scratch = 0;
            }

            assert( GetAlignBits() == 0 );

            // write tail

            int tailStart = headBytes + numWords * 4;
            int tailBytes = bytes - tailStart;
            assert( tailBytes >= 0 && tailBytes < 4 );
            for ( int i = 0; i < tailBytes; ++i )
                WriteBits( data[tailStart+i], 8 );

            assert( GetAlignBits() == 0 );

            assert( headBytes + numWords * 4 + tailBytes == bytes );
        }

        void FlushBits()
        {
            if ( m_bitIndex != 0 )
            {
//                printf( "write word: %x\n", htonl( m_scratch >> 32 ) );
                assert( m_wordIndex < m_numWords );
                if ( m_wordIndex >= m_numWords )
                {
                    m_overflow = true;
                    return;
                }
                m_data[m_wordIndex++] = uint32_t( m_scratch >> 32 );//htonl( uint32_t( m_scratch >> 32 ) );
            }
        }

        int GetBitsWritten() const
        {
            return m_bitsWritten;
        }

        int GetBitsAvailable() const
        {
            return m_numBits - m_bitsWritten;
        }

        const uint8_t * GetData() const
        {
            return (uint8_t*) m_data;
        }

        int GetBytesWritten() const
        {
            return m_wordIndex * 4;
        }

        int GetTotalBytes() const
        {
            return m_numWords * 4;
        }

        bool IsOverflow() const
        {
            return m_overflow;
        }

    private:

        uint32_t * m_data;
        uint64_t m_scratch;
        int m_numBits;
        int m_numWords;
        int m_bitsWritten;
        int m_bitIndex;
        int m_wordIndex;
        bool m_overflow;
    };

    class BitReader
    {
    public:

        BitReader( const void * data, int bytes )
            : m_data( (const uint32_t*)data ), m_numWords( bytes / 4 )
        {
            assert( data );
            assert( ( bytes % 4 ) == 0 );           // IMPORTANT: buffer size must be a multiple of four!
            m_numBits = m_numWords * 32;
            m_bitsRead = 0;
            m_bitIndex = 0;
            m_wordIndex = 0;
            m_scratch = m_data[0];//ntohl( m_data[0] );
            m_overflow = false;
//            printf( "read word = %x\n", m_data[0] );
        }

        uint32_t ReadBits( int bits )
        {
            assert( bits > 0 );
            assert( bits <= 32 );
            assert( m_bitsRead + bits <= m_numBits );

            if ( m_bitsRead + bits > m_numBits )
            {
                m_overflow = true;
                return 0;
            }

            m_bitsRead += bits;

            assert( m_bitIndex < 32 );

            if ( m_bitIndex + bits < 32 )
            {
                m_scratch <<= bits;
                m_bitIndex += bits;
            }
            else
            {
                m_wordIndex++;
                assert( m_wordIndex < m_numWords );
                const uint32_t a = 32 - m_bitIndex;
                const uint32_t b = bits - a;
                m_scratch <<= a;
                m_scratch |= m_data[m_wordIndex];//ntohl( m_data[m_wordIndex] );
//                printf( "read word = %x\n" );
                m_scratch <<= b;
                m_bitIndex = b;
            }

            const uint32_t output = uint32_t( m_scratch >> 32 );

            m_scratch &= 0xFFFFFFFF;

            return output;
        }

        void ReadAlign()
        {
            const int remainderBits = m_bitsRead % 8;
            if ( remainderBits != 0 )
            {
                uint32_t value = ReadBits( 8 - remainderBits );
                assert( value == 0 );
                assert( m_bitsRead % 8 == 0 );
            }
        }

        int GetAlignBits() const
        {
            const int remainderBits = m_bitsRead % 8;
            return remainderBits ? 8 - remainderBits : 0;
        }

        void ReadBytes( uint8_t * data, int bytes )
        {
            assert( GetAlignBits() == 0 );

            if ( m_bitsRead + bytes * 8 >= m_numBits )
            {
                memset( data, bytes, 0 );
                m_overflow = true;
                return;
            }

            // read head bytes

            assert( m_bitIndex == 0 || m_bitIndex == 8 || m_bitIndex == 16 || m_bitIndex == 24 );

            int headBytes = ( 4 - m_bitIndex / 8 ) % 4;
            if ( headBytes > bytes )
                headBytes = bytes;
            for ( int i = 0; i < headBytes; ++i )
                data[i] = ReadBits( 8 );
            if ( headBytes == bytes )
                return;

            assert( GetAlignBits() == 0 );

            // read words

            int numWords = ( bytes - headBytes ) / 4;
            if ( numWords > 0 )
            {
                assert( m_bitIndex == 0 );
                memcpy( data + headBytes, &m_data[m_wordIndex], numWords * 4 );
                m_bitsRead += numWords * 32;
                m_wordIndex += numWords;
                m_scratch = m_data[m_wordIndex];//ntohl( m_data[m_wordIndex] );
            }

            assert( GetAlignBits() == 0 );

            // write tail

            int tailStart = headBytes + numWords * 4;
            int tailBytes = bytes - tailStart;
            assert( tailBytes >= 0 && tailBytes < 4 );
            for ( int i = 0; i < tailBytes; ++i )
                data[tailStart+i] = ReadBits( 8 );

            assert( GetAlignBits() == 0 );

            assert( headBytes + numWords * 4 + tailBytes == bytes );
        }

        int GetBitsRead() const
        {
            return m_bitsRead;
        }

        int GetBitsRemaining() const
        {
            return m_numBits - m_bitsRead;
        }

        int GetTotalBits() const 
        {
            return m_numBits;
        }

        int GetTotalBytes() const
        {
            return m_numBits * 8;
        }

        bool IsOverflow() const
        {
            return m_overflow;
        }

    private:

        const uint32_t * m_data;
        uint64_t m_scratch;
        int m_numBits;
        int m_numWords;
        int m_bitsRead;
        int m_bitIndex;
        int m_wordIndex;
        bool m_overflow;
    };
}

#endif
