#include "DNSResolver.h"

using namespace std;
using namespace protocol;

void test_dns_resolve()
{
    cout << "test_dns_resolve" << endl;

    DNSResolver resolver;

    int num_google_success_callbacks = 0;
    int num_google_failure_callbacks = 0;

    string google_hostname( "google.com" );

    cout << "resolving " << google_hostname << endl;

    const int num_google_iterations = 10;

    for ( int i = 0; i < num_google_iterations; ++i )
    {
        resolver.Resolve( google_hostname, [&google_hostname, &num_google_success_callbacks, &num_google_failure_callbacks] ( const string & name, shared_ptr<ResolveResult> result ) 
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

    auto start = chrono::steady_clock::now();

    double t = 0.0;
    double dt = 0.1f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    for ( int i = 0; i < 50; ++i )
    {
        resolver.Update( TimeBase() );

        if ( num_google_success_callbacks == num_google_iterations )
            break;

        this_thread::sleep_for( ms );

        t += dt;
    }

    assert( num_google_success_callbacks == num_google_iterations );
    assert( num_google_failure_callbacks == 0 );

    auto finish = chrono::steady_clock::now();

    auto delta = finish - start;

    cout << chrono::duration<double,milli>( delta ).count() << " ms" << endl;

    google_entry = resolver.GetEntry( google_hostname );
    assert( google_entry );
    assert( google_entry->status == ResolveStatus::Succeeded );

    cout << google_hostname << ":" << endl;
    for ( auto & address : google_entry->result->addresses )
        cout << " + " << address.ToString() << endl;
}

void test_dns_resolve_failure()
{
    cout << "test_dns_resolve_failure" << endl;

    DNSResolver resolver;

    bool resolved = false;

    string garbage_hostname( "aoeusoanthuoaenuhansuhtasthas" );

    cout << "resolving garbage hostname: " << garbage_hostname << endl;

    resolver.Resolve( garbage_hostname, [&resolved, &garbage_hostname] ( const string & name, shared_ptr<ResolveResult> result ) 
    { 
        assert( name == garbage_hostname );
        assert( result == nullptr );
        resolved = true;
    } );

    auto entry = resolver.GetEntry( garbage_hostname );
    assert( entry );
    assert( entry->status == ResolveStatus::InProgress );

    auto start = chrono::steady_clock::now();

    double t = 0.0;
    double dt = 0.1f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    for ( int i = 0; i < 50; ++i )
    {
        resolver.Update( TimeBase() );

        if ( resolved )
            break;

        this_thread::sleep_for( ms );

        t += dt;
    }

    auto finish = chrono::steady_clock::now();

    auto delta = finish - start;

    cout << chrono::duration<double,milli>( delta ).count() << " ms" << endl;

    entry = resolver.GetEntry( garbage_hostname );
    assert( entry );
    assert( entry->status == ResolveStatus::Failed );
    assert( entry->result == nullptr );
}

// todo: add tests for resolving with port, eg. "localhost:10000" should return
// an address with port 10000, eg. "[::1]:10000"

int main()
{
    srand( time( NULL ) );

    try
    {
        test_dns_resolve();
        test_dns_resolve_failure();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
