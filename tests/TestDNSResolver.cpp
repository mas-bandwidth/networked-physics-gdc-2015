#include "DNSResolver.h"

using namespace protocol;

void test_dns_resolve()
{
    printf( "test_dns_resolve\n" );

    DNSResolver resolver;

    int num_google_success_callbacks = 0;
    int num_google_failure_callbacks = 0;

    std::string google_hostname( "google.com" );

//    printf( "resolving %s\n", google_hostname.c_str() );

    const int num_google_iterations = 10;

    for ( int i = 0; i < num_google_iterations; ++i )
    {
        resolver.Resolve( google_hostname, [&google_hostname, &num_google_success_callbacks, &num_google_failure_callbacks] ( const std::string & name, ResolveResult * result ) 
        { 
            assert( name == google_hostname );
            assert( result );
            if ( result )
                ++num_google_success_callbacks;
            else
                ++num_google_failure_callbacks;
        } );
    }

    auto google_entry = resolver.GetEntry( google_hostname );
    assert( google_entry );
    assert( google_entry->status == ResolveStatus::InProgress );

    double t = 0.0;
    double dt = 0.1f;
    std::chrono::milliseconds ms( (int) ( dt * 1000 ) );

    for ( int i = 0; i < 50; ++i )
    {
        resolver.Update( TimeBase() );

        if ( num_google_success_callbacks == num_google_iterations )
            break;

        std::this_thread::sleep_for( ms );

        t += dt;
    }

    assert( num_google_success_callbacks == num_google_iterations );
    assert( num_google_failure_callbacks == 0 );

    google_entry = resolver.GetEntry( google_hostname );
    assert( google_entry );
    assert( google_entry->status == ResolveStatus::Succeeded );
    assert( google_entry->result->addresses.size() );

    /*
    printf( "%s:\n", google_hostname.c_str() );
    for ( auto & address : google_entry->result->addresses )
        printf( " + %s\n", address.ToString().c_str() );
        */
}

void test_dns_resolve_with_port()
{
    printf( "test_dns_resolve_with_port\n" );

    DNSResolver resolver;

    int num_google_success_callbacks = 0;
    int num_google_failure_callbacks = 0;

    std::string google_hostname( "google.com:5000" );

//    printf( "resolving %s\n", google_hostname.c_str() );

    const int num_google_iterations = 10;

    for ( int i = 0; i < num_google_iterations; ++i )
    {
        resolver.Resolve( google_hostname, [&google_hostname, &num_google_success_callbacks, &num_google_failure_callbacks] ( const std::string & name, ResolveResult * result ) 
        { 
            assert( name == google_hostname );
            assert( result );
            if ( result )
                ++num_google_success_callbacks;
            else
                ++num_google_failure_callbacks;
        } );
    }

    auto google_entry = resolver.GetEntry( google_hostname );
    assert( google_entry );
    assert( google_entry->status == ResolveStatus::InProgress );

    double t = 0.0;
    double dt = 0.1f;
    std::chrono::milliseconds ms( (int) ( dt * 1000 ) );

    for ( int i = 0; i < 50; ++i )
    {
        resolver.Update( TimeBase() );

        if ( num_google_success_callbacks == num_google_iterations )
            break;

        std::this_thread::sleep_for( ms );

        t += dt;
    }

    assert( num_google_success_callbacks == num_google_iterations );
    assert( num_google_failure_callbacks == 0 );

    google_entry = resolver.GetEntry( google_hostname );
    assert( google_entry );
    assert( google_entry->status == ResolveStatus::Succeeded );
    assert( google_entry->result->addresses.size() );

    /*
    printf( "%s:\n", google_hostname.c_str() );
    for ( auto & address : google_entry->result->addresses )
        printf( " + %s\n", address.ToString().c_str() );
        */
}

void test_dns_resolve_failure()
{
    printf( "test_dns_resolve_failure\n" );

    DNSResolver resolver;

    bool resolved = false;

    std::string garbage_hostname( "aoeusoanthuoaenuhansuhtasthas" );

//    printf( "resolving garbage hostname: %s\n", garbage_hostname.c_str() );

    resolver.Resolve( garbage_hostname, [&resolved, &garbage_hostname] ( const std::string & name, ResolveResult * result ) 
    { 
        assert( name == garbage_hostname );
        assert( result == nullptr );
        resolved = true;
    } );

    auto entry = resolver.GetEntry( garbage_hostname );
    assert( entry );
    assert( entry->status == ResolveStatus::InProgress );

    double t = 0.0;
    double dt = 0.1f;
    std::chrono::milliseconds ms( (int) ( dt * 1000 ) );

    for ( int i = 0; i < 50; ++i )
    {
        resolver.Update( TimeBase() );

        if ( resolved )
            break;

        std::this_thread::sleep_for( ms );

        t += dt;
    }

    entry = resolver.GetEntry( garbage_hostname );
    assert( entry );
    assert( entry->status == ResolveStatus::Failed );
    assert( entry->result == nullptr );
}

int main()
{
    srand( time( nullptr ) );

    test_dns_resolve();
    test_dns_resolve_with_port();
    test_dns_resolve_failure();

    return 0;
}
