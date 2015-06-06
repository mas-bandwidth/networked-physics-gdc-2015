// Core Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef CORE_H
#define CORE_H

#include "core/Config.h"
#include "core/Types.h"
#include <stdint.h>
#include <stddef.h>

extern "C"
{
    extern void * memset( void * str, int c, size_t n );
    extern void * memcpy( void * dst, const void * src, size_t n );
    extern int memcmp ( const void * ptr1, const void * ptr2, size_t num );
    extern void * memmove( void * dest, const void * src, size_t count );
    extern void * malloc( size_t size );
    extern void free( void * ptr );
    extern int rand();
    extern void srand( unsigned int seed );
    extern int atoi( const char * string );
    extern size_t strlen( const char * string );
    extern int strcmp( const char * str1, const char * str2 );
    extern char * strcpy( char * destination, const char * source );
    extern char * strncpy( char * destination, const char * source, size_t size );
    extern char * strncpy_safe( char * destination, const char * source, size_t size );
    extern int printf( const char * format, ... );
    extern int sprintf( char * str, const char * format, ... );
    extern int snprintf( char * str, size_t size, const char * format, ... );
    extern void exit( int result );
}

namespace core
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
#define CORE_ASSERT( condition )                                                            \
do                                                                                          \
{                                                                                           \
    if ( !(condition) )                                                                     \
    {                                                                                       \
        core::DefaultAssertHandler( #condition, __FUNCTION__, __FILE__, __LINE__ );         \
    }                                                                                       \
} while(0)
#define CORE_ASSERT_CLOSE( a, b, tolerance )                                                \
do                                                                                          \
{                                                                                           \
    if ( fabs(a-b) > tolerance )                                                            \
    {                                                                                       \
        core::DefaultAssertHandler( #a " vs. " #b " is outside tolerance " #tolerance, __FUNCTION__, __FILE__, __LINE__ );         \
    }                                                                                       \
} while(0)
#else
#define CORE_ASSERT( condition ) do {} while(0)
#define CORE_ASSERT_CLOSE( a, b, tolerance ) do {} while(0)
#endif

#define CORE_CHECK( condition )                                                             \
do                                                                                          \
{                                                                                           \
    if ( !(condition) )                                                                     \
    {                                                                                       \
        core::DefaultCheckHandler( #condition, __FUNCTION__, __FILE__, __LINE__ );          \
    }                                                                                       \
} while(0)

#include "core/Log.h"

namespace core
{
    void sleep_milliseconds( uint32_t milliseconds );

    inline uint32_t host_to_network( uint32_t value )
    {
#if CORE_ENDIAN == CORE_BIG_ENDIAN
        return __builtin_bswap32( value );
#else
        return value;
#endif
    }

    inline uint32_t network_to_host( uint32_t value )
    {
#if CORE_ENDIAN == CORE_BIG_ENDIAN
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
    
    template <typename T> void swap( T & a, T & b )
    {
        T tmp = a;
        a = b;
        b = tmp;
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

    template <typename T> const T & min( const T & a, const T & b )
    {
        return ( a < b ) ? a : b;
    }

    template <typename T> const T & max( const T & a, const T & b )
    {
        return ( a > b ) ? a : b;
    }

    template <typename T> T clamp( const T & value, const T & min, const T & max )
    {
        if ( value < min )
            return min;
        else if ( value > max )
            return max;
        else
            return value;
    }

    template <typename T> T abs( const T & value )
    {
        return ( value < 0 ) ? -value : value;
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

    inline int sequence_difference( uint16_t _s1, uint16_t _s2 )
    {
        int s1 = _s1;
        int s2 = _s2;
        if ( abs( s1 - s2 ) >= 32786 )
        {
            if ( s1 > s2 )
                s2 += 65536;
            else
                s1 += 65536;
        }
        return s1 - s2;
    }

    uint16_t generate_id();

    int random_int( int min, int max );

    float random_float( float min, float max );

    uint32_t hash_data( const uint8_t * data, uint32_t length, uint32_t hash = 0 );
    uint32_t hash_string( const char string[], uint32_t hash = 0 );
    uint64_t murmur_hash_64( const void * key, uint32_t len, uint64_t seed );

    struct TimeBase
    {
        double time = 0.0;                // frame time. 0.0 is start of process
        double deltaTime = 0.0;           // delta time this frame in seconds.
    };

    uint64_t nanoseconds();

    double time();
}

#endif
