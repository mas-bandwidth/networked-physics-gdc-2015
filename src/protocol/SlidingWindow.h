// Protocol Library - Copyright (c) 2008-2015, The Network Protocol Company, Inc.

#ifndef SLIDING_WINDOW_H
#define SLIDING_WINDOW_H

#include "core/Core.h"
#include "core/Allocator.h"

namespace protocol
{
    template <typename T> class RealSlidingWindow
    {
    public:

        RealSlidingWindow( core::Allocator & allocator, int size )
        {
            CORE_ASSERT( size > 0 );
            m_size = size;
            m_sequence = 0;     // not a valid entry. insertion point for next sequence.
            m_ack = 0xFFFF;     // not a valid entry. last "acked" sequence number.
            m_allocator = &allocator;
            m_entries = (T*) allocator.Allocate( sizeof(T) * size, alignof(T) );
            Reset();
        }

        ~RealSlidingWindow()
        {
            CORE_ASSERT( m_entries );
            CORE_ASSERT( m_allocator );
            m_allocator->Free( m_entries );
            m_allocator = nullptr;
            m_entries = nullptr;
        }

        void Reset()
        {
            m_sequence = 0;
            m_ack = 0xFFFF;
        }

        void Insert( const T & entry )
        {
            CORE_ASSERT( !IsFull() );
            m_entries[m_sequence%m_size] = entry;
            m_sequence++;
        }

        void Ack( uint16_t ack )
        {
            if ( !core::sequence_greater_than( ack, m_ack ) )
                return;
            m_ack = ack;
            if ( core::sequence_greater_than( m_ack, m_sequence - 1 ) )
                m_ack = m_sequence - 1;
        }

        bool IsValid( uint16_t sequence ) const
        {
            return core::sequence_greater_than( sequence, m_ack ) && core::sequence_less_than( sequence, m_sequence );
        }

        const T * Get( uint16_t sequence ) const
        {
            CORE_ASSERT( IsValid() );
            return m_entries[sequence%m_size];
        }

        uint16_t GetSequence() const 
        {
            return m_sequence;
        }

        uint16_t GetAck() const
        {
            return m_ack;
        }

        uint16_t GetBegin() const
        {
            // first potentially valid sequence.
            return m_ack + 1;
        }

        uint16_t GetEnd() const
        {
            // iterate while != this.
            return m_sequence;
        }

        void GetArray( T * array, int & length ) const
        {
            length = GetNumEntries();
            CORE_ASSERT( length >= 0 );
            CORE_ASSERT( length <= m_size );
            const uint16_t begin = GetBegin();
            for ( int i = 0; i < length; ++i )
            {
                const int index = ( begin + i ) % m_size;
                array[i] = m_entries[index];
            }
        }

        int GetNumEntries() const
        {
            int first_sequence = GetBegin();
            int last_sequence = GetEnd();
            if ( first_sequence == last_sequence )
                return 0;
            if ( first_sequence > last_sequence )
                last_sequence += m_size;
            return last_sequence - first_sequence;
        }

        bool IsEmpty() const
        {
            return GetBegin() == GetEnd();
        }

        bool IsFull() const
        {
            return ( m_sequence % m_size ) == ( m_ack % m_size );
        }

        int GetSize() const
        {
            return m_size;
        }

    private:

        core::Allocator * m_allocator;

        uint16_t m_sequence;
        uint16_t m_ack;
        int m_size;
        T * m_entries;

        RealSlidingWindow( const RealSlidingWindow<T> & other );
        RealSlidingWindow<T> & operator = ( const RealSlidingWindow<T> & other );
    };

    template <typename T> class SlidingWindow
    {
    public:

        SlidingWindow( core::Allocator & allocator, int size )
        {
            CORE_ASSERT( size > 0 );
            m_size = size;
            m_first_entry = true;
            m_sequence = 0;
            m_allocator = &allocator;
            m_entries = (T*) allocator.Allocate( sizeof(T) * size, alignof(T) );
            Reset();
        }

        ~SlidingWindow()
        {
            CORE_ASSERT( m_entries );
            CORE_ASSERT( m_allocator );
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
            CORE_ASSERT( entry.valid );

            if ( m_first_entry )
            {
                m_sequence = entry.sequence + 1;
                m_first_entry = false;
            }
            else if ( core::sequence_greater_than( entry.sequence + 1, m_sequence ) )
            {
                m_sequence = entry.sequence + 1;
            }
            else if ( core::sequence_less_than( entry.sequence, m_sequence - m_size ) )
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
            else if ( core::sequence_greater_than( sequence + 1, m_sequence ) )
            {
                m_sequence = sequence + 1;
            }
            else if ( core::sequence_less_than( sequence, m_sequence - m_size ) )
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
            CORE_ASSERT( index >= 0 );
            CORE_ASSERT( index < m_size );
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

        core::Allocator * m_allocator;

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
