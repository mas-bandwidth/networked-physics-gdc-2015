/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_BLOCK_MESSAGE_H
#define PROTOCOL_BLOCK_MESSAGE_H

#include "Common.h"
#include "Stream.h"
#include "Block.h"
#include "Message.h"

namespace protocol
{
    class BlockMessage : public Message
    {
    public:

        BlockMessage() : Message( BlockMessageType ) {}

        void Connect( Block & block )
        {
            assert( block.IsValid() );
            Allocator * allocator = block.GetAllocator();
            assert( allocator );
            uint8_t * data = block.GetData();
            int size = block.GetSize();
            m_block.Connect( *allocator, data, size );       // we now own the block data
            block.Disconnect();
        }

        template <typename Stream> void Serialize( Stream & stream )
        { 
            serialize_block( stream, m_block, MaxSmallBlockSize );
        }

        void SerializeRead( ReadStream & stream )
        {
            Serialize( stream );
        }

        void SerializeWrite( WriteStream & stream )
        {
            Serialize( stream );
        }

        void SerializeMeasure( MeasureStream & stream )
        {
            Serialize( stream );
        }

        Block & GetBlock() { return m_block; }

        void SetAllocator( Allocator & allocator )
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
