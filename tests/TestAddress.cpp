#include "Address.h"

using namespace std;
using namespace protocol;

void test_address4()
{
    printf( "test_address4\n" );

    {
        Address address( 127, 0, 0, 1 );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 0 );
        assert( address.GetAddress4() == 0x100007f );
        // todo: fix once we have a std::string replacement
//        assert( address.ToString() == "127.0.0.1" );
    }

    {
        Address address( 127, 0, 0, 1, 1000 );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 1000 );
        assert( address.GetAddress4() == 0x100007f );
        // todo: fix once we have a std::string replacement
//        assert( address.ToString() == "127.0.0.1:1000" );
    }

    {
        Address address( "127.0.0.1" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 0 );
        assert( address.GetAddress4() == 0x100007f );
        // todo: fix once we have a std::string replacement
        //assert( address.ToString() == "127.0.0.1" );
    }

    {
        Address address( "127.0.0.1:65535" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 65535 );
        assert( address.GetAddress4() == 0x100007f );
        // todo: fix once we have a std::string replacement
        //assert( address.ToString() == "127.0.0.1:65535" );
    }

    {
        Address address( "10.24.168.192:3000" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 3000 );
        assert( address.GetAddress4() == 0xc0a8180a );
        // todo: fix once we have a std::string replacement
//        assert( address.ToString() == "10.24.168.192:3000" );
    }

    {
        Address address( "255.255.255.255:65535" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 65535 );
        assert( address.GetAddress4() == 0xffffffff );
        // todo: fix once we have a std::string replacement
//        assert( address.ToString() == "255.255.255.255:65535" );
    }
}

void test_address6()
{
    printf( "test_address6\n" );

    // without port numbers

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6[0], address6[1], address6[2], address6[2],
                         address6[4], address6[5], address6[6], address6[7] );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        // todo: fix once we have a std::string replacement

        //assert( address.ToString() == "fe80::202:b3ff:fe1e:8329" );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6 );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        // todo: fix once we have a std::string replacement

        //assert( address.ToString() == "fe80::202:b3ff:fe1e:8329" );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6 );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        // todo: fix once we have a std::string replacement

//        assert( address.ToString() == "::1" );
    }

    // same addresses but with port numbers

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6[0], address6[1], address6[2], address6[2],
                         address6[4], address6[5], address6[6], address6[7], 65535 );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        // todo: fix once we have a std::string replacement

//        assert( address.ToString() == "[fe80::202:b3ff:fe1e:8329]:65535" );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6, 65535 );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        // todo: fix once we have a std::string replacement
//        assert( address.ToString() == "[fe80::202:b3ff:fe1e:8329]:65535" );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6, 65535 );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        // todo: fix once we have a std::string replacement

//        assert( address.ToString() == "[::1]:65535" );
    }

    // parse addresses from strings (no ports)

    {
        Address address( "fe80::202:b3ff:fe1e:8329" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 0 );
        // todo: fix once we have a std::string replacement
//        assert( address.ToString() == "fe80::202:b3ff:fe1e:8329" );
    }

    {
        Address address( "::1" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 0 );
        // todo: fix once we have a std::string replacement
//        assert( address.ToString() == "::1" );
    }

    // parse addresses from strings (with ports)

    {
        Address address( "[fe80::202:b3ff:fe1e:8329]:65535" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 65535 );
        // todo: fix once we have a std::string replacement
//        assert( address.ToString() == "[fe80::202:b3ff:fe1e:8329]:65535" );
    }

    {
        Address address( "[::1]:65535" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 65535 );
        // todo: fix once we have a std::string replacement
//        assert( address.ToString() == "[::1]:65535" );
    }
}

int main()
{
    srand( time( NULL ) );

    test_address4();
    test_address6();

    return 0;
}
