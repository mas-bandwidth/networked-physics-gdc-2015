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

#ifndef PROTOCOL_BLOCK_H
#define PROTOCOL_BLOCK_H

#include "core/Core.h"
#include "core/Allocator.h"

namespace protocol
{
    class Block
    {
    public:

        Block();

        Block( core::Allocator & allocator, int bytes );

        ~Block();

        void Connect( core::Allocator & allocator, uint8_t * data, int size );

        void Disconnect();

        void Destroy();

        void SetAllocator( core::Allocator & allocator )
        {
            m_allocator = &allocator;
        }

        core::Allocator * GetAllocator() const
        {
            return m_allocator;
        }

        uint8_t * GetData()
        {
            return m_data;
        }

        const uint8_t * GetData() const
        {
            return m_data;
        }

        int GetSize() const
        {
            return m_size;
        }

        bool IsValid() const
        {
            return m_allocator && m_data;
        }

    private:

        core::Allocator * m_allocator;
        uint8_t * m_data;
        int m_size;

        Block( const Block & other );
        Block & operator = ( const Block & other );
    };
}

#endif
