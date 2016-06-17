/*
    Networked Physics Example

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

#ifndef CORE_QUEUE_H
#define CORE_QUEUE_H

#include "Array.h"
#include "Types.h"

namespace core
{
    namespace queue_internal
    {
        template<typename T> void increase_capacity( Queue<T> & q, uint32_t new_capacity )
        {
            uint32_t end = array::size( q.m_data );
            array::resize( q.m_data, new_capacity );
            if ( q.m_offset + q.m_size > end ) 
            {
                uint32_t end_items = end - q.m_offset;
                memmove( array::begin( q.m_data ) + new_capacity - end_items, array::begin( q.m_data ) + q.m_offset, end_items * sizeof(T) );
                q.m_offset += new_capacity - end;
            }
        }

        template<typename T> void grow( Queue<T> & q, uint32_t min_capacity = 0 )
        {
            uint32_t new_capacity = array::size( q.m_data ) * 2 + 8;
            if ( new_capacity < min_capacity )
                new_capacity = min_capacity;
            increase_capacity( q, new_capacity );
        }
    }

    namespace queue 
    {
        template<typename T> inline uint32_t size( const Queue<T> & q )
        {
            return q.m_size;
        }

        template<typename T> inline uint32_t space( const Queue<T> & q )
        {
            return array::size( q.m_data ) - q.m_size;
        }

        template<typename T> void reserve( Queue<T> & q, uint32_t size )
        {
            if ( size > q.m_size )
                queue_internal::increase_capacity( q, size );
        }

        template<typename T> inline void push_back( Queue<T> & q, const T & item )
        {
            if ( !space( q ) )
                queue_internal::grow( q );
            q[q.m_size++] = item;
        }

        template<typename T> inline void pop_back( Queue<T> & q )
        {
            --q.m_size;
        }
        
        template<typename T> inline void push_front( Queue<T> & q, const T & item )
        {
            if ( !space( q ) )
                queue_internal::grow( q );
            q.m_offset = ( q.m_offset - 1 + array::size( q.m_data ) ) % array::size( q.m_data );
            ++q.m_size;
            q[0] = item;
        }
        
        template<typename T> inline void pop_front( Queue<T> & q )
        {
            q.m_offset = ( q.m_offset + 1 ) % array::size( q.m_data );
            --q.m_size;
        }

        template<typename T> inline void consume( Queue<T> & q, uint32_t n )
        {
            q.m_offset = ( q.m_offset + n) % array::size( q.m_data );
            q.m_size -= n;
        }

        template<typename T> void clear( Queue<T> & q )
        {
            queue::consume( q, queue::size(q) );
        }

        template<typename T> void push( Queue<T> & q, const T * items, uint32_t n )
        {
            if ( space(q) < n )
                queue_internal::grow( q, size( q ) + n );
            const uint32_t size = array::size( q.m_data );
            const uint32_t insert = ( q.m_offset + q.m_size ) % size;
            uint32_t to_insert = n;
            if ( insert + to_insert > size )
                to_insert = size - insert;
            memcpy( array::begin(q.m_data) + insert, items, to_insert * sizeof(T) );
            q.m_size += to_insert;
            items += to_insert;
            n -= to_insert;
            memcpy( array::begin(q.m_data), items, n * sizeof(T) );
            q.m_size += n;
        }
 
        template<typename T> inline T * begin_front( Queue<T> & q )
        {
            return array::begin( q.m_data ) + q.m_offset;
        }

        template<typename T> inline const T * begin_front( const Queue<T> & q )
        {
            return array::begin( q.m_data ) + q.m_offset;
        }

        template<typename T> T * end_front( Queue<T> & q )
        {
            uint32_t end = q.m_offset + q.m_size;
            return end > array::size( q.m_data ) ? array::end( q.m_data ) : array::begin( q.m_data ) + end;
        }
        template<typename T> const T * end_front(const Queue<T> &q)
        {
            uint32_t end = q.m_offset + q.m_size;
            return end > array::size( q.m_data ) ? array::end( q.m_data ) : array::begin( q.m_data ) + end;
        }
    }

    template <typename T> inline Queue<T>::Queue( Allocator & allocator) : m_data( allocator ), m_size(0), m_offset(0) {}

    template <typename T> inline T & Queue<T>::operator [] ( uint32_t i )
    {
        return m_data[ ( i + m_offset ) % array::size( m_data ) ];
    }

    template <typename T> inline const T & Queue<T>::operator [] ( uint32_t i ) const
    {
        return m_data[ ( i + m_offset ) % array::size( m_data ) ];
    }
}

#endif
