// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef PROTOCOL_BITPACKER_H
#define PROTOCOL_BITPACKER_H

#include "core/Core.h"

namespace protocol
{
    class BitWriter
    {
    public:

        BitWriter( void * data, int bytes );

        void WriteBits( uint32_t value, int bits );

        void WriteAlign();

        void WriteBytes( const uint8_t * data, int bytes );

        void FlushBits();

        int GetAlignBits() const
        {
            return ( 8 - m_bitsWritten % 8 ) % 8;
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

        BitReader( const void * data, int bytes );

        uint32_t ReadBits( int bits );

        void ReadAlign();

        void ReadBytes( uint8_t * data, int bytes );

        int GetAlignBits() const
        {
            return ( 8 - m_bitsRead % 8 ) % 8;
        }

        int GetBitsRead() const
        {
            return m_bitsRead;
        }

        int GetBytesRead() const
        {
            return ( m_wordIndex + 1 ) * 4;     // note: +1 so it matches bytes written
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
