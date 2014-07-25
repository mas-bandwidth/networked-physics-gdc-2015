/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#include "Common.h"
#include <time.h>

#if PROTOCOL_PLATFORM == PROTOCOL_PLATFORM_MAC || PROTOCOL_PLATFORM == PROTOCOL_PLATFORM_UNIX
#include <unistd.h>
#endif

namespace protocol
{
    void DefaultAssertHandler( const char * condition, 
                               const char * function,
                               const char * file,
                               int line )
    {
        printf( "Assert failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
        __builtin_trap();
    }

    void DefaultCheckHandler( const char * condition, 
                              const char * function,
                              const char * file,
                              int line )
    {
        printf( "Check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
        __builtin_trap();
    }

    void sleep_milliseconds( uint32_t milliseconds )
    {
        #if PROTOCOL_PLATFORM == PROTOCOL_PLATFORM_MAC || PROTOCOL_PLATFORM == PROTOCOL_PLATFORM_UNIX
            usleep( milliseconds * 1000);
        #elif PROTOCOL_PLATFORM == PROTOCOL_PLATFORM_WINDOWS
            Sleep( milliseconds );
        #else
            #error need sleep_milliseconds implementation for this platform!
        #endif
    }

    uint64_t generate_guid()
    {
        return ( uint64_t( rand() ) << 32 ) | time( nullptr );
    }

    int random_int( int min, int max )
    {
        PROTOCOL_ASSERT( max > min );
        int result = min + rand() % ( max - min + 1 );
        PROTOCOL_ASSERT( result >= min );
        PROTOCOL_ASSERT( result <= max );
        return result;
    }

    float random_float( float min, float max )
    {
        const int res = 10000000;
        double scale = ( rand() % res ) / double( res - 1 );
        return (float) ( min + (double) ( max - min ) * scale );
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
            #if PROTOCOL_ENDIAN == PROTOCOL_BIG_ENDIAN
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
