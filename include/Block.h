/*
    Network Protocol Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_BLOCK_H
#define PROTOCOL_BLOCK_H

#include "Common.h"
#include "Allocator.h"

namespace protocol
{
    class Block
    {
        Allocator * m_allocator;
        uint8_t * m_data;
        int m_size;

        Block( const Block & other );
        Block & operator = ( const Block & other );

    public:

        Block()
        {
            m_allocator = nullptr;
            m_data = nullptr;
            m_size = 0;
        }

        Block( Allocator & allocator, int bytes )
        {
            PROTOCOL_ASSERT( bytes > 0 );
            m_allocator = &allocator;
            m_data = (uint8_t*) allocator.Allocate( bytes );
//            printf( "allocate block data %p (%d)\n", m_data, bytes );
            m_size = bytes;
            PROTOCOL_ASSERT( m_data );
        }

        ~Block()
        {
            Destroy();
        }

        void SetAllocator( Allocator & allocator )
        {
            m_allocator = &allocator;
        }

        void Connect( Allocator & allocator, uint8_t * data, int size )
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

        void Disconnect()
        {
            // disconnect this block from its data
            // call this when you are taking ownership of the block
            // data away from this block instance and giving it to
            // something else.
            m_data = nullptr;
            m_allocator = nullptr;
            m_size = 0;
        }

        void Destroy()
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

        uint8_t * GetData()
        {
            return m_data;
        }

        Allocator * GetAllocator() const
        {
            return m_allocator;
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
    };
}

#endif
