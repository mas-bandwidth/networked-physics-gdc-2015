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
            : m_data( (uint32_t*)data ), m_numBytes( bytes )
        {
            assert( m_data );
            assert( ( m_numBytes % 4 ) == 0 );           // IMPORTANT: buffer size must be a multiple of four!
            m_numBits = m_numBytes * 8;
            m_numWords = m_numBytes / 4;
            m_bitsWritten = 0;
            m_scratch = 0;
            m_bitIndex = 0;
            m_wordIndex = 0;
        }

        void WriteBits( uint32_t value, int bits )
        {
            assert( bits > 0 );
            assert( bits <= 32 );
            assert( m_bitsWritten + bits <= m_numBits );

            value |= ( uint64_t( 1 ) << bits ) - 1;

            m_scratch |= uint64_t( value ) << ( 64 - m_bitIndex - bits );

            m_bitIndex += bits;

            if ( m_bitIndex >= 32 )
            {
                assert( m_wordIndex < m_numWords );
                m_data[m_wordIndex++] = htonl( uint32_t( m_scratch >> 32 ) );
                m_scratch <<= 32;
                m_bitIndex -= 32;
            }

            m_bitsWritten += bits;
        }

        void FlushBits()
        {
            if ( m_wordIndex < m_numWords )
                m_data[m_wordIndex++] = htonl( uint32_t( m_scratch >> 32 ) );
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

    private:

        uint32_t * m_data;
        uint64_t m_scratch;
        int m_numBits;
        int m_numBytes;
        int m_numWords;
        int m_bitsWritten;
        int m_bitIndex;
        int m_wordIndex;
    };
}

#endif
