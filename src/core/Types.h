// Core Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <stdint.h>

namespace core
{
    class Allocator;

    template<typename T> struct Array
    {
        Array( Allocator & a );
        ~Array();

        Array( const Array & other );
        Array & operator = ( const Array & other );
        
        T & operator [] ( uint32_t i );
        const T & operator [] ( uint32_t i ) const;

        Allocator * m_allocator;
        uint32_t m_size;
        uint32_t m_capacity;
        T * m_data;
    };

    template <typename T> struct Queue
    {
        Queue( Allocator & a );

        T & operator [] ( uint32_t i );
        const T & operator [] ( uint32_t i ) const;

        Array<T> m_data;
        uint32_t m_size;
        uint32_t m_offset;
    };

    template<typename T> struct Hash
    {
        Hash( Allocator & a );
        
        struct Entry 
        {
            uint64_t key;
            uint32_t next;
            T value;
        };

        Array<uint32_t> _hash;
        Array<Entry> _data;
    };
}

#endif