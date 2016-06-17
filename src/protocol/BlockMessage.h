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

#ifndef PROTOCOL_BLOCK_MESSAGE_H
#define PROTOCOL_BLOCK_MESSAGE_H

#include "core/Core.h"
#include "protocol/Stream.h"
#include "protocol/Block.h"
#include "protocol/Message.h"

namespace protocol
{
    class BlockMessage : public Message
    {
    public:

        BlockMessage() : Message( BlockMessageType ) {}

        void Connect( Block & block )
        {
            CORE_ASSERT( block.IsValid() );
            core::Allocator * allocator = block.GetAllocator();
            CORE_ASSERT( allocator );
            uint8_t * data = block.GetData();
            int size = block.GetSize();
            m_block.Connect( *allocator, data, size );       // we now own the block data
            block.Disconnect();
        }

        PROTOCOL_SERIALIZE_OBJECT( stream )
        {
            serialize_block( stream, m_block, MaxSmallBlockSize );
        }

        Block & GetBlock() { return m_block; }

        void SetAllocator( core::Allocator & allocator )
        {
            m_block.SetAllocator( allocator );
        }

    private:

        Block m_block;

        BlockMessage( const BlockMessage & other );
        const BlockMessage & operator = ( const BlockMessage & other );
    };
}

#endif
