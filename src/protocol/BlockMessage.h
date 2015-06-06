// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

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
