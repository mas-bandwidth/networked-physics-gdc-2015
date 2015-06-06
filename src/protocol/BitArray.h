// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef PROTOCOL_BIT_ARRAY_H
#define PROTOCOL_BIT_ARRAY_H

#include "core/Core.h"
#include "core/Allocator.h"

namespace protocol
{
    class BitArray
    {
    public:

        BitArray( core::Allocator & allocator, int size )
        {
            CORE_ASSERT( size > 0 );
            m_allocator = &allocator;
            m_size = size;
            m_bytes = 8 * ( ( size / 64 ) + ( ( size % 64 ) ? 1 : 0 ) );
            CORE_ASSERT( m_bytes > 0 );
            m_data = (uint64_t*) allocator.Allocate( m_bytes, alignof(uint64_t) );
            Clear();
        }

        ~BitArray()
        {
            CORE_ASSERT( m_data );
            CORE_ASSERT( m_allocator );
            m_allocator->Free( m_data );
            m_allocator = nullptr;
            m_data = nullptr;
        }

        void Clear()
        {
            CORE_ASSERT( m_data );
            memset( m_data, 0, m_bytes );
        }

        void SetBit( int index )
        {
            CORE_ASSERT( index >= 0 );
            CORE_ASSERT( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            CORE_ASSERT( bit_index >= 0 );
            CORE_ASSERT( bit_index < 64 );
            m_data[data_index] |= uint64_t(1) << bit_index;
        }

        void ClearBit( int index )
        {
            CORE_ASSERT( index >= 0 );
            CORE_ASSERT( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            m_data[data_index] &= ~( uint64_t(1) << bit_index );
        }

        uint64_t GetBit( int index ) const
        {
            CORE_ASSERT( index >= 0 );
            CORE_ASSERT( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            CORE_ASSERT( bit_index >= 0 );
            CORE_ASSERT( bit_index < 64 );
            return ( m_data[data_index] >> bit_index ) & 1;
        }

        int GetSize() const
        {
            return m_size;
        }

    private:

        core::Allocator * m_allocator;

        int m_size;
        int m_bytes;
        uint64_t * m_data;

        BitArray( const BitArray & other );
        BitArray & operator = ( const BitArray & other );
    };
}

#endif
