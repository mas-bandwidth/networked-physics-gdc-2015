#include "network/Config.h"

#if NETWORK_USE_RESOLVER

#include "network/DNSResolver.h"

void test_dns_resolve()
{
    printf( "test_dns_resolve\n" );

    network::DNSResolver resolver;

    std::string google_hostname( "google.com" );

    resolver.Resolve( google_hostname );

    auto google_entry = resolver.GetEntry( google_hostname );
    CORE_CHECK( google_entry );
    CORE_CHECK( google_entry->status == network::RESOLVE_IN_PROGRESS );

    double t = 0.0;
    double dt = 0.1f;
    int ms = (int) ( dt * 1000 );

    for ( int i = 0; i < 50; ++i )
    {
        resolver.Update( TimeBase() );

        auto google_entry = resolver.GetEntry( google_hostname );
        if ( google_entry && google_entry->status != network::RESOLVE_IN_PROGRESS )
            break;

        sleep_milliseconds( ms );

        t += dt;
    }

    google_entry = resolver.GetEntry( google_hostname );
    CORE_CHECK( google_entry );
    CORE_CHECK( google_entry->status == network::RESOLVE_SUCCEEDED );
    CORE_CHECK( google_entry->result.numAddresses );
}

void test_dns_resolve_with_port()
{
    printf( "test_dns_resolve_with_port\n" );

    network::DNSResolver resolver;

    std::string google_hostname( "google.com:5000" );

    resolver.Resolve( google_hostname );

    auto google_entry = resolver.GetEntry( google_hostname );
    CORE_CHECK( google_entry );
    CORE_CHECK( google_entry->status == network::RESOLVE_IN_PROGRESS );

    double t = 0.0;
    double dt = 0.1f;
    int ms = (int) ( dt * 1000 );

    for ( int i = 0; i < 50; ++i )
    {
        resolver.Update( TimeBase() );

        auto google_entry = resolver.GetEntry( google_hostname );
        if ( google_entry && google_entry->status != network::RESOLVE_IN_PROGRESS )
            break;

        sleep_milliseconds( ms );

        t += dt;
    }

    google_entry = resolver.GetEntry( google_hostname );
    CORE_CHECK( google_entry );
    CORE_CHECK( google_entry->status == network::RESOLVE_SUCCEEDED );
    CORE_CHECK( google_entry->result.numAddresses );
    for ( int i = 0; i < google_entry->result.numAddresses; ++i )
        CORE_CHECK( google_entry->result.address[i].GetPort() == 5000 );
}

void test_dns_resolve_failure()
{
    printf( "test_dns_resolve_failure\n" );

    network::DNSResolver resolver;

    std::string garbage_hostname( "aoeusoanthuoaenuhansuhtasthas" );

//    printf( "resolving garbage hostname: %s\n", garbage_hostname.c_str() );

    resolver.Resolve( garbage_hostname );

    auto entry = resolver.GetEntry( garbage_hostname );
    CORE_CHECK( entry );
    CORE_CHECK( entry->status == network::RESOLVE_IN_PROGRESS );

    double t = 0.0;
    double dt = 0.1f;
    int ms = (int) ( dt * 1000 );

    while ( true )
    {
        resolver.Update( TimeBase() );

        auto entry = resolver.GetEntry( garbage_hostname );
        if ( entry && entry->status != network::RESOLVE_IN_PROGRESS )
            break;

        sleep_milliseconds( ms );

        t += dt;
    }

    entry = resolver.GetEntry( garbage_hostname );
    CORE_CHECK( entry );

    CORE_CHECK( entry->status == network::RESOLVE_FAILED );
}

#else

enum { Dummy };

#endif
