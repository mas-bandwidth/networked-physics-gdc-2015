/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#include "Block.h"
#include "Allocator.h"

namespace protocol
{
    Block::Block()
    {
        m_allocator = nullptr;
        m_data = nullptr;
        m_size = 0;
    }

    Block::Block( Allocator & allocator, int bytes )
    {
        PROTOCOL_ASSERT( bytes > 0 );
        m_allocator = &allocator;
        m_data = (uint8_t*) allocator.Allocate( bytes );
//            printf( "allocate block data %p (%d)\n", m_data, bytes );
        m_size = bytes;
        PROTOCOL_ASSERT( m_data );
    }

    Block::~Block()
    {
        Destroy();
    }

    void Block::Connect( Allocator & allocator, uint8_t * data, int size )
    {
        // connect this block class to the memory passed in
        // this block class now *owns* the block data and 
        // will free it when the message is destroyed.
        PROTOCOL_ASSERT( m_data == nullptr );
        PROTOCOL_ASSERT( data );
        PROTOCOL_ASSERT( size > 0 );
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
        m_data = nullptr;
        m_allocator = nullptr;
        m_size = 0;
    }

    void Block::Destroy()
    {
        if ( !m_data )
            return;
//          printf( "free block data %p (%d)\n", m_data, m_size );
        PROTOCOL_ASSERT( m_allocator );
        PROTOCOL_ASSERT( m_size > 0 );
        m_allocator->Free( m_data );
        m_data = nullptr;
        m_size = 0;
        m_allocator = nullptr;
    }
}
