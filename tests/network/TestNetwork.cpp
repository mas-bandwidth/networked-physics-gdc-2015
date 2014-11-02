#include "core/Core.h"
#include "network/Network.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

extern void test_address4();
extern void test_address6();

extern void test_bsd_socket_send_and_receive_ipv4();
extern void test_bsd_socket_send_and_receive_ipv6();
extern void test_bsd_socket_send_and_receive_multiple_ipv4();
extern void test_bsd_socket_send_and_receive_multiple_ipv6();

#if PROTOCOL_USE_RESOLVER
extern void test_dns_resolve();
extern void test_dns_resolve_with_port();
extern void test_dns_resolve_failure();
#endif

int main()
{
    srand( time( nullptr ) );

    if ( !network::InitializeNetwork() )
    {
        printf( "failed to initialize network\n" );
        return 1;
    }

    CORE_ASSERT( network::IsNetworkInitialized() );

    test_address4();
    test_address6();

    test_bsd_socket_send_and_receive_ipv4();
    test_bsd_socket_send_and_receive_ipv6();
    test_bsd_socket_send_and_receive_multiple_ipv4();
    test_bsd_socket_send_and_receive_multiple_ipv6();

#if PROTOCOL_USE_RESOLVER
    test_dns_resolve();
    test_dns_resolve_with_port();
    test_dns_resolve_failure();
#endif

    network::ShutdownNetwork();

    return 0;
}
