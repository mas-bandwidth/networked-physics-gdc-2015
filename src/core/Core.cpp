/*
    Networked Physics Demo

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

#include "core/Core.h"
#include <time.h>
#include <stdio.h>

#if CORE_PLATFORM == CORE_PLATFORM_MAC || CORE_PLATFORM == CORE_PLATFORM_UNIX
#include <unistd.h>
#endif
#if CORE_PLATFORM == CORE_PLATFORM_MAC
#include <mach/mach_time.h>
#elif CORE_PLATFORM == CORE_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

char * strncpy_safe( char * destination, const char * source, size_t size )
{
    // IMPORTANT: Make sure the string is always null terminated even if source is longer than dest
    char * result = strncpy( destination, source, size );
    destination[size-1] = '\0';
    return result;
}

namespace core
{
    void DefaultAssertHandler( const char * condition, 
                               const char * function,
                               const char * file,
                               int line )
    {
        printf( "Assert failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
#if CORE_PLATFORM == CORE_PLATFORM_WINDOWS
		__debugbreak();
#else // #if CORE_PLATFORM == CORE_PLATFORM_WINDOWS
        __builtin_trap();
#endif // #if CORE_PLATFORM == CORE_PLATFORM_WINDOWS
    }

    void DefaultCheckHandler( const char * condition, 
                              const char * function,
                              const char * file,
                              int line )
    {
        printf( "Check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
#if CORE_PLATFORM == CORE_PLATFORM_WINDOWS
		__debugbreak();
#else // #if CORE_PLATFORM == CORE_PLATFORM_WINDOWS
        __builtin_trap();
#endif // #if CORE_PLATFORM == CORE_PLATFORM_WINDOWS
    }

    void sleep_milliseconds( uint32_t milliseconds )
    {
        #if CORE_PLATFORM == CORE_PLATFORM_MAC || CORE_PLATFORM == CORE_PLATFORM_UNIX
            usleep( milliseconds * 1000 );
        #elif CORE_PLATFORM == CORE_PLATFORM_WINDOWS
            Sleep( milliseconds );
        #else
            #error need sleep_milliseconds implementation for this platform!
        #endif
    }

    uint16_t generate_id()
    {
        return ( rand() % 65535 ) + 1;
    }

    int random_int( int min, int max )
    {
        CORE_ASSERT( max > min );
        int result = min + rand() % ( max - min + 1 );
        CORE_ASSERT( result >= min );
        CORE_ASSERT( result <= max );
        return result;
    }

    float random_float( float min, float max )
    {
        const int res = 10000000;
        double scale = ( rand() % res ) / double( res - 1 );
        return (float) ( min + (double) ( max - min ) * scale );
    }

    uint32_t hash_data( const uint8_t * data, uint32_t length, uint32_t hash )
    {
        CORE_ASSERT( data );
        for ( uint32_t i = 0; i < length; ++i )
        {
            hash += data[i];
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        return hash;
    } 

    uint32_t hash_string( const char string[], uint32_t hash )
    {
        CORE_ASSERT( string );
        while ( *string != '\0' )
        {
            char c = *string++;
            if ( ( c >= 'a' ) && ( c <= 'z' ) ) 
                c = ( c - 'a' ) + 'A';
            hash += c;
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        return hash;
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
            #if CORE_ENDIAN == CORE_BIG_ENDIAN
                uint64_t k = *data++;
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

    uint64_t nanoseconds() 
    {
        static uint64_t is_init = 0;

        #if CORE_PLATFORM == CORE_PLATFORM_MAC

            static mach_timebase_info_data_t info;
            if ( 0 == is_init )
            {
                mach_timebase_info( &info );
                is_init = 1;
            }
            uint64_t now;
            now = mach_absolute_time();
            now *= info.numer;
            now /= info.denom;
            return now;

        #elif CORE_PLATFORM == CORE_PLATFORM_LINUX

            #ifdef CLOCK_MONOTONIC
            #define CLOCKID CLOCK_MONOTONIC
            #else
            #define CLOCKID CLOCK_REALTIME
            #endif
            static struct timespec linux_rate;
            if ( 0 == is_init )
            {
                clock_getres( CLOCKID, &linux_rate );
                is_init = 1;
            }
            uint64_t now;
            struct timespec spec;
            clock_gettime( CLOCKID, &spec );
            now = spec.tv_sec * 1.0e9 + spec.tv_nsec;
            return now;

        #elif CORE_PLATFORM == CORE_PLATFORM_WINDOWS

            static LARGE_INTEGER win_frequency;
            if ( 0 == is_init ) 
            {
                QueryPerformanceFrequency( &win_frequency );
                is_init = 1;
            }
            LARGE_INTEGER now;
            QueryPerformanceCounter( &now );
            return (uint64_t) ( ( 1e9 * now.QuadPart ) / win_frequency.QuadPart );

        #endif
    }

    double time()
    {
        return nanoseconds() / 1000000000.0;        
    }
}
