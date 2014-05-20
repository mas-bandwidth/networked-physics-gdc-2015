/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_COMMON_H
#define PROTOCOL_COMMON_H

#include <cassert>
#include <string>
#include <vector>
#include <limits>
#include <memory>
#include <vector>
#include <thread>
#include <future>
#include <queue>
#include <chrono>
#include <map>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PLATFORM_WINDOWS  1
#define PLATFORM_MAC      2
#define PLATFORM_UNIX     3

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#else
#define PLATFORM PLATFORM_UNIX
#endif

#if PLATFORM == PLATFORM_WINDOWS

    #include <winsock2.h>
    #pragma comment( lib, "wsock32.lib" )

#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>

#else

    #error unknown platform!

#endif

namespace protocol
{
    using namespace std;

    struct TimeBase
    {
        TimeBase() : time(0), deltaTime(0) {}

        double time;                    // frame time. 0.0 is start of process
        double deltaTime;               // delta time this frame in seconds.
    };

    string format_string( const char * fmt_str, ... ) 
    {
        int final_n, n = 256;
        string str;
        unique_ptr<char[]> formatted;
        va_list ap;
        while(1) 
        {
            formatted.reset(new char[n]);
            strcpy(&formatted[0], fmt_str);
            va_start(ap, fmt_str);
            final_n = vsnprintf(&formatted[0], n, fmt_str, ap);
            va_end(ap);
            if (final_n < 0 || final_n >= n)
                n += abs(final_n - n + 1);
            else
                break;
        }
        return string(formatted.get());
    }

    class Object
    {  
    public:

        virtual ~Object() {}

        virtual void Serialize( class Stream & stream ) = 0;
    };

    template <typename T> class Factory
    {
    public:

        typedef function< shared_ptr<T>() > create_function;

        Factory()
        {
            max_type = 0;
        }

        void Register( int type, create_function const & function )
        {
            create_map[type] = function;
            max_type = max( type, max_type );
        }

        shared_ptr<T> Create( int type )
        {
            auto itor = create_map.find( type );
            if ( itor == create_map.end() )
                throw runtime_error( "invalid object type id in factory create" );
            else
                return itor->second();
        }

        int GetMaxType() const
        {
            return max_type;
        }

    private:

        int max_type;
        map<int,create_function> create_map;
    };

    bool sequence_greater_than( uint16_t s1, uint16_t s2 )
    {
        return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) || 
               ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
    }

    bool sequence_less_than( uint16_t s1, uint16_t s2 )
    {
        return sequence_greater_than( s2, s1 );
    }

    template <typename T> class SlidingWindow
    {
    public:

        SlidingWindow( int size )
        {
            assert( size > 0 );
            m_first_entry = true;
            m_sequence = 0;
            m_entries.resize( size );
        }

        void Reset()
        {
            m_first_entry = true;
            m_sequence = 0;
            auto size = m_entries.size();
            assert( size );
            m_entries.clear();
            m_entries.resize( size );
        }

        bool Insert( const T & entry )
        {
            assert( entry.valid );

            if ( m_first_entry )
            {
                m_sequence = entry.sequence;
                m_first_entry = false;
            }
            else if ( sequence_greater_than( entry.sequence, m_sequence ) )
            {
                m_sequence = entry.sequence;
            }
            else if ( sequence_less_than( entry.sequence, ( m_sequence - ( m_entries.size() - 1 ) ) ) )
            {
                return false;
            }

            const int index = entry.sequence % m_entries.size();
            assert( index >= 0 );
            assert( index < m_entries.size() );
            m_entries[index] = entry;

            return true;
        }

        T * InsertFast( uint16_t sequence )
        {
            if ( m_first_entry )
            {
                m_sequence = sequence;
                m_first_entry = false;
            }
            else if ( sequence_greater_than( sequence, m_sequence ) )
            {
                m_sequence = sequence;
            }
            else if ( sequence_less_than( sequence, ( m_sequence - ( m_entries.size() - 1 ) ) ) )
            {
                return nullptr;
            }

            const int index = sequence % m_entries.size();
            assert( index >= 0 );
            assert( index < m_entries.size() );
            auto entry = &m_entries[index];
            entry->valid = 1;
            entry->sequence = sequence;
            return entry;
        }

        bool HasSlotAvailable( uint16_t sequence ) const
        {
            const int index = sequence % m_entries.size();
            assert( index >= 0 );
            assert( index < m_entries.size() );
            return !m_entries[index].valid;
        }

        const T * Find( uint16_t sequence ) const
        {
            const int index = sequence % m_entries.size();
            assert( index >= 0 );
            assert( index < m_entries.size() );
            if ( m_entries[index].valid && m_entries[index].sequence == sequence )
                return &m_entries[index];
            else
                return NULL;
        }

        T * Find( uint16_t sequence )
        {
            const int index = sequence % m_entries.size();
            assert( index >= 0 );
            assert( index < m_entries.size() );
            if ( m_entries[index].valid && m_entries[index].sequence == sequence )
                return &m_entries[index];
            else
                return NULL;
        }

        uint16_t GetSequence() const 
        {
            return m_sequence;
        }

    private:

        bool m_first_entry;
        uint16_t m_sequence;
        vector<T> m_entries;
    };

    template <typename T> void GenerateAckBits( const SlidingWindow<T> & packets, 
                                                uint16_t & ack,
                                                uint32_t & ack_bits )
    {
        ack = packets.GetSequence();
        ack_bits = 0;
        for ( int i = 0; i < 32; ++i )
        {
            uint16_t sequence = ack - i;
            if ( packets.Find( sequence ) )
                ack_bits |= ( 1 << i );
        }
    }

    typedef vector<uint8_t> Block;

    enum
    {
        PROTOCOL_LITTLE_ENDIAN = 0x03020100,
        PROTOCOL_BIG_ENDIAN = 0x00010203
    };

    static const union { uint8_t bytes[4]; uint32_t value; } protocol_host_order = { { 0, 1, 2, 3 } };

    #define PROTOCOL_HOST_ORDER protocol_host_order.value

    
}

#endif
