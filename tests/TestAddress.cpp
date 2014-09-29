#include "network/Address.h"
#include "core/Memory.h"
#include <string.h>
#include <arpa/inet.h>

using namespace protocol;

void test_address4()
{
    printf( "test_address4\n" );

    memory::initialize();

    char buffer[256];

    {
        Address address( 127, 0, 0, 1 );
        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV4 );
        PROTOCOL_CHECK( address.GetPort() == 0 );
        PROTOCOL_CHECK( address.GetAddress4() == 0x100007f );
        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "127.0.0.1" ) == 0 );
    }

    {
        Address address( 127, 0, 0, 1, 1000 );
        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV4 );
        PROTOCOL_CHECK( address.GetPort() == 1000 );
        PROTOCOL_CHECK( address.GetAddress4() == 0x100007f );
        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "127.0.0.1:1000" ) == 0 );
    }

    {
        Address address( "127.0.0.1" );
        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV4 );
        PROTOCOL_CHECK( address.GetPort() == 0 );
        PROTOCOL_CHECK( address.GetAddress4() == 0x100007f );
        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "127.0.0.1" ) == 0 );
    }

    {
        Address address( "127.0.0.1:65535" );
        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV4 );
        PROTOCOL_CHECK( address.GetPort() == 65535 );
        PROTOCOL_CHECK( address.GetAddress4() == 0x100007f );
        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "127.0.0.1:65535" ) == 0 );
    }

    {
        Address address( "10.24.168.192:3000" );
        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV4 );
        PROTOCOL_CHECK( address.GetPort() == 3000 );
        PROTOCOL_CHECK( address.GetAddress4() == 0xc0a8180a );
        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "10.24.168.192:3000" ) == 0 );
    }

    {
        Address address( "255.255.255.255:65535" );
        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV4 );
        PROTOCOL_CHECK( address.GetPort() == 65535 );
        PROTOCOL_CHECK( address.GetAddress4() == 0xffffffff );
        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "255.255.255.255:65535" ) == 0 );
    }

    memory::shutdown();
}

void test_address6()
{
    printf( "test_address6\n" );

    memory::initialize();

    char buffer[256];

    // without port numbers

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6[0], address6[1], address6[2], address6[2],
                         address6[4], address6[5], address6[6], address6[7] );

        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV6 );
        PROTOCOL_CHECK( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            PROTOCOL_CHECK( htons( address6[i] ) == address.GetAddress6()[i] );

        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6 );

        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV6 );
        PROTOCOL_CHECK( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            PROTOCOL_CHECK( htons( address6[i] ) == address.GetAddress6()[i] );

        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6 );

        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV6 );
        PROTOCOL_CHECK( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            PROTOCOL_CHECK( htons( address6[i] ) == address.GetAddress6()[i] );

        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "::1" ) == 0 );
    }

    // same addresses but with port numbers

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6[0], address6[1], address6[2], address6[2],
                         address6[4], address6[5], address6[6], address6[7], 65535 );

        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV6 );
        PROTOCOL_CHECK( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            PROTOCOL_CHECK( htons( address6[i] ) == address.GetAddress6()[i] );

        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6, 65535 );

        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV6 );
        PROTOCOL_CHECK( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            PROTOCOL_CHECK( htons( address6[i] ) == address.GetAddress6()[i] );

        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6, 65535 );

        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV6 );
        PROTOCOL_CHECK( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            PROTOCOL_CHECK( htons( address6[i] ) == address.GetAddress6()[i] );

        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "[::1]:65535" ) == 0 );
    }

    // parse addresses from strings (no ports)

    {
        Address address( "fe80::202:b3ff:fe1e:8329" );
        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV6 );
        PROTOCOL_CHECK( address.GetPort() == 0 );
        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        Address address( "::1" );
        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV6 );
        PROTOCOL_CHECK( address.GetPort() == 0 );
        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "::1" ) == 0 );
    }

    // parse addresses from strings (with ports)

    {
        Address address( "[fe80::202:b3ff:fe1e:8329]:65535" );
        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV6 );
        PROTOCOL_CHECK( address.GetPort() == 65535 );
        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        Address address( "[::1]:65535" );
        PROTOCOL_CHECK( address.IsValid() );
        PROTOCOL_CHECK( address.GetType() == ADDRESS_IPV6 );
        PROTOCOL_CHECK( address.GetPort() == 65535 );
        PROTOCOL_CHECK( strcmp( address.ToString( buffer, 256 ), "[::1]:65535" ) == 0 );
    }

    memory::shutdown();
}
