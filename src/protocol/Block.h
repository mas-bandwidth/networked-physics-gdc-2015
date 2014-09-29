/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_BLOCK_H
#define PROTOCOL_BLOCK_H

#include "Common.h"

namespace protocol
{
    class Block
    {
    public:

        Block();

        Block( Allocator & allocator, int bytes );

        ~Block();

        void Connect( Allocator & allocator, uint8_t * data, int size );

        void Disconnect();

        void Destroy();

        void SetAllocator( Allocator & allocator )
        {
            m_allocator = &allocator;
        }

        Allocator * GetAllocator() const
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

        Allocator * m_allocator;
        uint8_t * m_data;
        int m_size;

        Block( const Block & other );
        Block & operator = ( const Block & other );
    };
}

#endif
