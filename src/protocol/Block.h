// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

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
