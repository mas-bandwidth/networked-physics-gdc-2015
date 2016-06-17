/*
    Networked Physics Demo

    Copyright © 2008 - 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
