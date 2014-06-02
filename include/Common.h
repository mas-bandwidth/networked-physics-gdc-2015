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

    template <uint32_t x> struct PopCount
    {
        enum {   a = x - ( ( x >> 1 )       & 0x55555555 ),
                 b =   ( ( ( a >> 2 )       & 0x33333333 ) + ( a & 0x33333333 ) ),
                 c =   ( ( ( b >> 4 ) + b ) & 0x0f0f0f0f ),
                 d =   c + ( c >> 8 ),
                 e =   d + ( d >> 16 ),

            result = e & 0x0000003f 
        };
    };

    template <uint32_t x> struct Log2
    {
        enum {   a = x | ( x >> 1 ),
                 b = a | ( a >> 2 ),
                 c = b | ( b >> 4 ),
                 d = c | ( c >> 8 ),
                 e = d | ( d >> 16 ),
                 f = e >> 1,

            result = PopCount<f>::result
        };
    };

    template <int64_t min, int64_t max> struct BitsRequired
    {
        static const uint32_t result = ( min == max ) ? 0 : Log2<uint32_t(max-min)>::result + 1;
    };
    
    inline uint32_t popcount( uint32_t x )
    {
        const uint32_t a = x - ( ( x >> 1 )       & 0x55555555 );
        const uint32_t b =   ( ( ( a >> 2 )       & 0x33333333 ) + ( a & 0x33333333 ) );
        const uint32_t c =   ( ( ( b >> 4 ) + b ) & 0x0f0f0f0f );
        const uint32_t d =   c + ( c >> 8 );
        const uint32_t e =   d + ( d >> 16 );
        const uint32_t result = e & 0x0000003f;
        return result;
    }

#ifdef __GNUC__

    inline int bits_required( uint32_t min, uint32_t max )
    {
        return 32 - __builtin_clz( max - min );
    }

#else

    inline uint32_t log2( uint32_t x )
    {
        const uint32_t a = x | ( x >> 1 );
        const uint32_t b = a | ( a >> 2 );
        const uint32_t c = b | ( b >> 4 );
        const uint32_t d = c | ( c >> 8 );
        const uint32_t e = d | ( d >> 16 );
        const uint32_t f = e >> 1;
        return popcount( f );
    }

    inline int bits_required( uint32_t min, uint32_t max )
    {
        return ( min == max ) ? 0 : log2( max-min ) + 1;
    }

#endif

    inline bool is_power_of_two( uint32_t x )
    {
        return ( x != 0 ) && ( ( x & ( x - 1 ) ) == 0 );
    }

    bool sequence_greater_than( uint16_t s1, uint16_t s2 )
    {
        return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) || 
               ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
    }

    bool sequence_less_than( uint16_t s1, uint16_t s2 )
    {
        return sequence_greater_than( s2, s1 );
    }

    struct TimeBase
    {
        TimeBase() : time(0), deltaTime(0) {}

        double time;                    // frame time. 0.0 is start of process
        double deltaTime;               // delta time this frame in seconds.
    };

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
                m_sequence = entry.sequence + 1;
                m_first_entry = false;
            }
            else if ( sequence_greater_than( entry.sequence + 1, m_sequence ) )
            {
                m_sequence = entry.sequence + 1;
            }
            else if ( sequence_less_than( entry.sequence, m_sequence - m_entries.size() ) )
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
                m_sequence = sequence + 1;
                m_first_entry = false;
            }
            else if ( sequence_greater_than( sequence + 1, m_sequence ) )
            {
                m_sequence = sequence + 1;
            }
            else if ( sequence_less_than( sequence, m_sequence - m_entries.size() ) )
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
        ack = packets.GetSequence() - 1;
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

    inline int random_int( int min, int max )
    {
        assert( max > min );
        int result = min + rand() % ( max - min + 1 );
        assert( result >= min );
        assert( result <= max );
        return result;
    }

    inline float random_float( float min, float max )
    {
        const int res = 10000000;
        double scale = ( rand() % res ) / double( res - 1 );
        return (float) ( min + (double) ( max - min ) * scale );
    }

    uint64_t GenerateGuid()
    {
        // todo: we really want something better than this
        return ( uint64_t(rand()) << 32 ) | time( NULL );
    }
}

#endif
