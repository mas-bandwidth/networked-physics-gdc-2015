/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_COMMON_H
#define PROTOCOL_COMMON_H

#include "Constants.h"

#include <cassert>

#if NOSTL == 0
#include <string>
#include <vector>
#include <limits>
#include <memory>
#include <thread>
#include <queue>
#include <chrono>
#include <map>
#endif

#include <future>
#include <unistd.h>
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

        virtual void SerializeRead( class ReadStream & stream ) = 0;

        virtual void SerializeWrite( class WriteStream & stream ) = 0;

        virtual void SerializeMeasure( class MeasureStream & stream ) = 0;
    };

#if !NOSTL

    template <typename T> class Factory
    {
    public:

        typedef std::function<T*()> create_function;
        
        Factory()
        {
            max_type = 0;
        }

        void Register( int type, create_function const & function )
        {
            create_map[type] = function;
            max_type = std::max( type, max_type );
        }

        bool TypeExists( int type )
        {
            auto itor = create_map.find( type );
            return itor != create_map.end();
        }

        T * Create( int type )
        {
            auto itor = create_map.find( type );
            assert( itor != create_map.end() );
            if ( itor != create_map.end() )
                return itor->second();
            else
                return nullptr;
        }

        int GetMaxType() const
        {
            return max_type;
        }

    private:

        int max_type;
        
        std::map<int,create_function> create_map;
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
            auto entry = &m_entries[index];
            entry->valid = 1;
            entry->sequence = sequence;
            return entry;
        }

        bool HasSlotAvailable( uint16_t sequence ) const
        {
            const int index = sequence % m_entries.size();
            return !m_entries[index].valid;
        }

        int GetIndex( uint16_t sequence ) const
        {
            const int index = sequence % m_entries.size();
            return index;
        }

        const T * Find( uint16_t sequence ) const
        {
            const int index = sequence % m_entries.size();
            if ( m_entries[index].valid && m_entries[index].sequence == sequence )
                return &m_entries[index];
            else
                return nullptr;
        }

        T * Find( uint16_t sequence )
        {
            const int index = sequence % m_entries.size();
            if ( m_entries[index].valid && m_entries[index].sequence == sequence )
                return &m_entries[index];
            else
                return nullptr;
        }

        T * GetAtIndex( int index )
        {
            assert( index >= 0 );
            assert( index < m_entries.size() );
            return m_entries[index].valid ? &m_entries[index] : nullptr;
        }

        uint16_t GetSequence() const 
        {
            return m_sequence;
        }

        int GetSize() const
        {
            return m_entries.size();
        }

    private:

        bool m_first_entry;
        uint16_t m_sequence;
        std::vector<T> m_entries;
    };

    typedef std::vector<uint8_t> Block;

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

#endif

    inline uint64_t GenerateGuid()
    {
        return ( uint64_t(rand()) << 32 ) | time( nullptr );
    }

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

    inline void * align_forward( void * p, uint32_t align )
    {
        uintptr_t pi = uintptr_t( p );
        const uint32_t mod = pi % align;
        if ( mod )
            pi += align - mod;
        return (void*) pi;
    }

    inline void * pointer_add( void * p, uint32_t bytes )
    {
        return (void*) ( (uint8_t*)p + bytes );
    }

    inline const void * pointer_add( const void * p, uint32_t bytes )
    {
        return (const void*) ( (const uint8_t*)p + bytes );
    }

    inline void * pointer_sub( void * p, uint32_t bytes )
    {
        return (void*)( (char*)p - bytes );
    }

    inline const void * pointer_sub( const void * p, uint32_t bytes )
    {
        return (const void*) ( (const char*)p - bytes );
    }

    uint64_t murmur_hash_64( const void * key, uint32_t len, uint64_t seed )
    {
        const uint64_t m = 0xc6a4a7935bd1e995ULL;
        const uint32_t r = 47;

        uint64_t h = seed ^ ( len * m );

        const uint64_t * data = ( const uint64_t*) key;
        const uint64_t * end = data + len / 8;

        while ( data != end )
        {
            #ifdef PROTOCOL_BIG_ENDIAN
                uint64 k = *data++;
                uint8_t * p = (uint8_t*) &k;
                uint8_t c;
                c = p[0]; p[0] = p[7]; p[7] = c;
                c = p[1]; p[1] = p[6]; p[6] = c;
                c = p[2]; p[2] = p[5]; p[5] = c;
                c = p[3]; p[3] = p[4]; p[4] = c;
            #else
                uint64_t k = *data++;
            #endif

            k *= m;
            k ^= k >> r;
            k *= m;
            
            h ^= k;
            h *= m;
        }

        const uint8_t * data2 = (const uint8_t*) data;

        switch ( len & 7 )
        {
            case 7: h ^= uint64_t( data2[6] ) << 48;
            case 6: h ^= uint64_t( data2[5] ) << 40;
            case 5: h ^= uint64_t( data2[4] ) << 32;
            case 4: h ^= uint64_t( data2[3] ) << 24;
            case 3: h ^= uint64_t( data2[2] ) << 16;
            case 2: h ^= uint64_t( data2[1] ) << 8;
            case 1: h ^= uint64_t( data2[0] );
                    h *= m;
        };
        
        h ^= h >> r;
        h *= m;
        h ^= h >> r;

        return h;
    }
}

#endif
