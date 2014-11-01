#include "core/Core.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

extern void test_server_data();
extern void test_client_data();
extern void test_client_and_server_data();
extern void test_client_and_server_data_reconnect();
extern void test_client_and_server_data_multiple_clients();
extern void test_server_data_too_large();
extern void test_client_server_user_context();

int main()
{
    srand( time( nullptr ) );

    test_server_data();
    test_client_data();
    test_client_and_server_data();
    test_client_and_server_data_reconnect();
    test_client_and_server_data_multiple_clients();
    test_server_data_too_large();
    test_client_server_user_context();

    return 0;
}
