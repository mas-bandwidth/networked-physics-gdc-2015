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
//                cout << "write word: " << htonl( m_scratch >> 32 ) << endl;
                m_data[m_wordIndex++] = htonl( uint32_t( m_scratch >> 32 ) );
                m_scratch <<= 32;
                m_bitIndex -= 32;
            }

            m_bitsWritten += bits;
        }

        void FlushBits()
        {
            if ( m_bitIndex != 0 )
            {
//                cout << "write word: " << htonl( m_scratch >> 32 ) << endl;
                assert( m_wordIndex < m_numWords );
                m_data[m_wordIndex++] = htonl( uint32_t( m_scratch >> 32 ) );
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

        int GetBytes() const
        {
            return m_wordIndex * 4;
        }

        bool InOverflow() const
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
            m_wordIndex = 1;
            m_scratch = ntohl( m_data[0] );
            m_overflow = false;
//            cout << "read word = " << m_data[0] << endl;
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

            if ( m_bitIndex + bits <= 32 )
            {
                m_scratch <<= bits;
                m_bitIndex += bits;
            }
            else
            {
                assert( m_wordIndex < m_numWords );
                const uint32_t a = 32 - m_bitIndex;
                const uint32_t b = bits - a;
                m_scratch <<= a;
//                cout << "read word = " << m_data[m_wordIndex] << endl;
                m_scratch |= ntohl( m_data[m_wordIndex++] );
                m_scratch <<= b;
                m_bitIndex = b;
            }

            const uint32_t output = uint32_t( m_scratch >> 32 );

            m_scratch &= 0xFFFFFFFF;

            return output;
        }

        int GetBitsRead() const
        {
            return m_bitsRead;
        }

        int GetBitsRemaining() const
        {
            return m_numBits - m_bitsRead;
        }

        bool InOverflow() const
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
