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

#include "protocol/Block.h"
#include "core/Allocator.h"

namespace protocol
{
    Block::Block()
    {
        m_allocator = NULL;
        m_data = NULL;
        m_size = 0;
    }

    Block::Block( core::Allocator & allocator, int bytes )
    {
        CORE_ASSERT( bytes > 0 );
        m_allocator = &allocator;
        m_data = (uint8_t*) allocator.Allocate( bytes );
//            printf( "allocate block data %p (%d)\n", m_data, bytes );
        m_size = bytes;
        CORE_ASSERT( m_data );
    }

    Block::~Block()
    {
        Destroy();
    }

    void Block::Connect( core::Allocator & allocator, uint8_t * data, int size )
    {
        // connect this block class to the memory passed in
        // this block class now *owns* the block data and 
        // will free it when the message is destroyed.
        CORE_ASSERT( m_data == nullptr );
        CORE_ASSERT( data );
        CORE_ASSERT( size > 0 );
        m_allocator = &allocator;
        m_data = data;
        m_size = size;
    }

    void Block::Disconnect()
    {
        // disconnect this block from its data
        // call this when you are taking ownership of the block
        // data away from this block instance and giving it to
        // something else.
        m_data = NULL;
        m_allocator = NULL;
        m_size = 0;
    }

    void Block::Destroy()
    {
        if ( !m_data )
            return;
//          printf( "free block data %p (%d)\n", m_data, m_size );
        CORE_ASSERT( m_allocator );
        CORE_ASSERT( m_size > 0 );
        m_allocator->Free( m_data );
        m_data = NULL;
        m_size = 0;
        m_allocator = NULL;
    }
}
