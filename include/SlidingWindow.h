/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef SLIDING_WINDOW_H
#define SLIDING_WINDOW_H

#include "Common.h"
#include "Allocator.h"

namespace protocol
{
    template <typename T> class SlidingWindow
    {
    public:

        SlidingWindow( Allocator & allocator, int size )
        {
            assert( size > 0 );
            m_size = size;
            m_first_entry = true;
            m_sequence = 0;
            m_allocator = &allocator;
            m_entries = (T*) allocator.Allocate( sizeof(T) * size, alignof(T) );
            Reset();
        }

        ~SlidingWindow()
        {
            assert( m_entries );
            assert( m_allocator );
            m_allocator->Free( m_entries );
            m_allocator = nullptr;
            m_entries = nullptr;
        }

        void Reset()
        {
            m_first_entry = true;
            m_sequence = 0;
            for ( int i = 0; i < m_size; ++i )
                m_entries[i] = T();
        }

        bool Insert( const T & entry )
        {
            assert( entry.valid );

            if ( m_first_entry )
            {
                m_sequence = entry.sequence + 1;
                m_first_entry = false;
            }
            else if ( sequence_greater_than( entry.sequence + 1, m_sequence ) )
            {
                m_sequence = entry.sequence + 1;
            }
            else if ( sequence_less_than( entry.sequence, m_sequence - m_size ) )
            {
                return false;
            }

            const int index = entry.sequence % m_size;

            m_entries[index] = entry;

            return true;
        }

        T * InsertFast( uint16_t sequence )
        {
            if ( m_first_entry )
            {
                m_sequence = sequence + 1;
                m_first_entry = false;
            }
            else if ( sequence_greater_than( sequence + 1, m_sequence ) )
            {
                m_sequence = sequence + 1;
            }
            else if ( sequence_less_than( sequence, m_sequence - m_size ) )
            {
                return nullptr;
            }

            const int index = sequence % m_size;
            auto entry = &m_entries[index];
            entry->valid = 1;
            entry->sequence = sequence;
            return entry;
        }

        bool HasSlotAvailable( uint16_t sequence ) const
        {
            const int index = sequence % m_size;
            return !m_entries[index].valid;
        }

        int GetIndex( uint16_t sequence ) const
        {
            const int index = sequence % m_size;
            return index;
        }

        const T * Find( uint16_t sequence ) const
        {
            const int index = sequence % m_size;
            if ( m_entries[index].valid && m_entries[index].sequence == sequence )
                return &m_entries[index];
            else
                return nullptr;
        }

        T * Find( uint16_t sequence )
        {
            const int index = sequence % m_size;
            if ( m_entries[index].valid && m_entries[index].sequence == sequence )
                return &m_entries[index];
            else
                return nullptr;
        }

        T * GetAtIndex( int index )
        {
            assert( index >= 0 );
            assert( index < m_size );
            return m_entries[index].valid ? &m_entries[index] : nullptr;
        }

        uint16_t GetSequence() const 
        {
            return m_sequence;
        }

        int GetSize() const
        {
            return m_size;
        }

    private:

        Allocator * m_allocator;

        bool m_first_entry;
        uint16_t m_sequence;
        int m_size;
        T * m_entries;

        SlidingWindow( const SlidingWindow<T> & other );
        SlidingWindow<T> & operator = ( const SlidingWindow<T> & other );
    };

    template <typename T> void GenerateAckBits( const SlidingWindow<T> & packets, 
                                                uint16_t & ack,
                                                uint32_t & ack_bits )
    {
        ack = packets.GetSequence() - 1;
        ack_bits = 0;
        for ( int i = 0; i < 32; ++i )
        {
            uint16_t sequence = ack - i;
            if ( packets.Find( sequence ) )
                ack_bits |= ( 1 << i );
        }
    }
}

#endif
