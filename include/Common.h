/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_COMMON_H
#define PROTOCOL_COMMON_H

#include "Config.h"
#include "Types.h"
#include "Constants.h"
#include <stdint.h>
#include <stddef.h>

extern "C"
{
    extern void * memset( void * str, int c, size_t n );
    extern void * memcpy( void * dst, const void * src, size_t n );
    extern void * malloc( size_t size );
    extern void free( void * ptr );
    extern int rand();
    extern void srand( unsigned int seed );
    extern int atoi( const char * string );
    extern int printf( const char * format, ... );
    extern int snprintf( char * str, size_t size, const char * format, ... );
    extern void exit( int result );
}

namespace protocol
{
    extern void DefaultAssertHandler( const char * condition, 
                                      const char * function,
                                      const char * file,
                                      int line );

    extern void DefaultCheckHandler( const char * condition, 
                                     const char * function,
                                     const char * file,
                                     int line );
}

#ifndef NDEBUG
#define PROTOCOL_ASSERT( condition )                                                        \
do                                                                                          \
{                                                                                           \
    if ( !(condition) )                                                                     \
    {                                                                                       \
        protocol::DefaultAssertHandler( #condition, __FUNCTION__, __FILE__, __LINE__ );     \
    }                                                                                       \
} while(0)
#else
#define PROTOCOL_ASSERT( condition ) do {} while(0)
#endif

#define PROTOCOL_CHECK( condition )                                                         \
do                                                                                          \
{                                                                                           \
    if ( !(condition) )                                                                     \
    {                                                                                       \
        protocol::DefaultCheckHandler( #condition, __FUNCTION__, __FILE__, __LINE__ );      \
    }                                                                                       \
} while(0)

#include "Enums.h"
#include "Log.h"

namespace protocol
{
    inline uint32_t host_to_network( uint32_t value )
    {
#if PROTOCOL_ENDIAN == PROTOCOL_BIG_ENDIAN
        return __builtin_bswap32( value );
#else
        return value;
#endif
    }

    inline uint32_t network_to_host( uint32_t value )
    {
#if PROTOCOL_ENDIAN == PROTOCOL_BIG_ENDIAN
        return __builtin_bswap32( value );
#else
        return value;
#endif
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

    template <typename T> const T & max( const T & a, const T & b )
    {
        return ( a > b ) ? a : b;
    }

    inline bool sequence_greater_than( uint16_t s1, uint16_t s2 )
    {
        return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) || 
               ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
    }

    inline bool sequence_less_than( uint16_t s1, uint16_t s2 )
    {
        return sequence_greater_than( s2, s1 );
    }

    uint64_t generate_guid();

    int random_int( int min, int max );

    float random_float( float min, float max );

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

    uint64_t murmur_hash_64( const void * key, uint32_t len, uint64_t seed );

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
}

#endif
