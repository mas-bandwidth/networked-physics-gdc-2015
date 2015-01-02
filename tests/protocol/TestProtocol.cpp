#include "core/Core.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

extern void test_message_factory();
extern void test_packet_factory();
extern void test_bitpacker();
extern void test_stream();
extern void test_stream_context();
extern void test_bit_array();
extern void test_sliding_window();
extern void test_sequence_buffer();
extern void test_generate_ack_bits();
extern void test_block();

extern void test_connection();
extern void test_acks();

extern void test_reliable_message_channel_messages();
extern void test_reliable_message_channel_small_blocks();
extern void test_reliable_message_channel_large_blocks();
extern void test_reliable_message_channel_mixture();

extern void test_client_initial_state();
extern void test_client_resolve_hostname_failure();
extern void test_client_resolve_hostname_timeout();
extern void test_client_resolve_hostname_success();
extern void test_client_connection_request_timeout();
extern void test_client_connection_request_denied();
extern void test_client_connection_challenge();
extern void test_client_connection_challenge_response();
extern void test_client_connection_established();
extern void test_client_connection_messages();
extern void test_client_connection_disconnect();
extern void test_client_connection_server_full();
extern void test_client_connection_timeout();
extern void test_client_connection_already_connected();
extern void test_client_connection_reconnect();
extern void test_client_side_disconnect();

extern void test_data_block_send_and_receive();
extern void test_data_block_send_and_receive_packet_loss();

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

    test_message_factory();
    test_packet_factory();
    test_bitpacker();
    test_stream();
    test_stream_context();
    test_bit_array();
    test_sliding_window();
    test_sequence_buffer();
    test_generate_ack_bits();
    test_block();

    test_connection();
    test_acks();

    test_reliable_message_channel_messages();
    test_reliable_message_channel_small_blocks();
    test_reliable_message_channel_large_blocks();
    test_reliable_message_channel_mixture();

    test_data_block_send_and_receive();
    test_data_block_send_and_receive_packet_loss();

    return 0;
}
