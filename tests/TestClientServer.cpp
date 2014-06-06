#include "Client.h"
#include "Server.h"
#include "BSDSockets.h"
#include "DNSResolver.h"
#include "ClientServerPackets.h"

using namespace std;
using namespace protocol;

class TestChannel : public ChannelAdapter 
{
public:
    TestChannel() {}
};

class TestChannelStructure : public ChannelStructure
{
public:
    TestChannelStructure()
    {
        AddChannel( "test channel", [] { return make_shared<TestChannel>(); }, [] { return nullptr; } );
        Lock();
    }
};

void test_client_initial_state()
{
    cout << "test_client_initial_state" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<ClientServerPacketFactory>( channelStructure );

    BSDSocketsConfig bsdSocketsConfig;
    bsdSocketsConfig.port = 10000;
    bsdSocketsConfig.maxPacketSize = 1024;
    bsdSocketsConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto networkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

    ClientConfig clientConfig;
    clientConfig.networkInterface = networkInterface;
    clientConfig.channelStructure = channelStructure;

    Client client( clientConfig );

    assert( client.IsDisconnected () );
    assert( !client.IsConnected() );
    assert( !client.IsConnecting() );
    assert( !client.HasError() );
    assert( client.GetError() == CLIENT_ERROR_None );
    assert( client.GetState() == CLIENT_STATE_Disconnected );
    assert( client.GetNetworkInterface() == networkInterface );
    assert( client.GetResolver() == nullptr );
}

void test_client_resolve_hostname_failure()
{
    cout << "test_client_resolve_hostname_failure" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<ClientServerPacketFactory>( channelStructure );

    BSDSocketsConfig bsdSocketsConfig;
    bsdSocketsConfig.port = 10000;
    bsdSocketsConfig.maxPacketSize = 1024;
    bsdSocketsConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto networkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

    ClientConfig clientConfig;
    clientConfig.resolver = make_shared<DNSResolver>();
    clientConfig.networkInterface = networkInterface;
    clientConfig.channelStructure = channelStructure;

    Client client( clientConfig );

    client.Connect( "asduoanuhaoenuthaosuhta" );

    assert( client.IsConnecting() );
    assert( !client.IsDisconnected() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_ResolvingHostname );

    TimeBase timeBase;
    timeBase.deltaTime = 1.0f;

    for ( int i = 0; i < 60; ++i )
    {
        if ( client.HasError() )
            break;

        client.Update( timeBase );

        timeBase.time += timeBase.deltaTime;

        this_thread::sleep_for( chrono::milliseconds( 100 ) );
    }

    assert( client.IsDisconnected() );
    assert( !client.IsConnecting() );
    assert( !client.IsConnected() );
    assert( client.HasError() );
    assert( client.GetState() == CLIENT_STATE_Disconnected );
    assert( client.GetError() == CLIENT_ERROR_ResolveHostnameFailed );
}

void test_client_resolve_hostname_timeout()
{
    cout << "test_client_resolve_hostname_timeout" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<ClientServerPacketFactory>( channelStructure );

    BSDSocketsConfig bsdSocketsConfig;
    bsdSocketsConfig.port = 10000;
    bsdSocketsConfig.maxPacketSize = 1024;
    bsdSocketsConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto networkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

    ClientConfig clientConfig;
    clientConfig.resolver = make_shared<DNSResolver>();
    clientConfig.networkInterface = networkInterface;
    clientConfig.channelStructure = channelStructure;

    Client client( clientConfig );

    client.Connect( "my butt" );

    assert( client.IsConnecting() );
    assert( !client.IsDisconnected() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_ResolvingHostname );

    TimeBase timeBase;
    timeBase.deltaTime = 1.0f;

    for ( int i = 0; i < 60; ++i )
    {
        if ( client.HasError() )
            break;

        client.Update( timeBase );

        timeBase.time += timeBase.deltaTime;
    }

    assert( client.IsDisconnected() );
    assert( !client.IsConnecting() );
    assert( !client.IsConnected() );
    assert( client.HasError() );
    assert( client.GetState() == CLIENT_STATE_Disconnected );
    assert( client.GetError() == CLIENT_ERROR_ResolveHostnameTimedOut );
}

void test_client_resolve_hostname_success()
{
    cout << "test_client_resolve_hostname_success" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<ClientServerPacketFactory>( channelStructure );

    BSDSocketsConfig bsdSocketsConfig;
    bsdSocketsConfig.port = 10000;
    bsdSocketsConfig.maxPacketSize = 1024;
    bsdSocketsConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto networkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

    ClientConfig clientConfig;
    clientConfig.resolver = make_shared<DNSResolver>();
    clientConfig.networkInterface = networkInterface;
    clientConfig.channelStructure = channelStructure;

    Client client( clientConfig );

    client.Connect( "localhost" );

    assert( client.IsConnecting() );
    assert( !client.IsDisconnected() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_ResolvingHostname );

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    for ( int i = 0; i < 60; ++i )
    {
        if ( client.GetState() == CLIENT_STATE_SendingConnectionRequest )
            break;

        client.Update( timeBase );

        timeBase.time += timeBase.deltaTime;

        this_thread::sleep_for( chrono::milliseconds( 100 ) );
    }

    assert( !client.IsDisconnected() );
    assert( client.IsConnecting() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_SendingConnectionRequest );
    assert( client.GetError() == CLIENT_ERROR_None );
}

void test_client_connection_request_timeout()
{
    cout << "test_client_connection_request_timeout" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<ClientServerPacketFactory>( channelStructure );

    BSDSocketsConfig bsdSocketsConfig;
    bsdSocketsConfig.port = 10000;
    bsdSocketsConfig.maxPacketSize = 1024;
    bsdSocketsConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto networkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

    ClientConfig clientConfig;
    clientConfig.resolver = make_shared<DNSResolver>();
    clientConfig.networkInterface = networkInterface;
    clientConfig.channelStructure = channelStructure;

    Client client( clientConfig );

    client.Connect( "[::1]:10001" );

    assert( client.IsConnecting() );
    assert( !client.IsDisconnected() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_SendingConnectionRequest );

    TimeBase timeBase;
    timeBase.deltaTime = 1.0f;

    for ( int i = 0; i < 60; ++i )
    {
        if ( client.HasError() )
            break;

        client.Update( timeBase );

        timeBase.time += timeBase.deltaTime;
    }

    assert( client.IsDisconnected() );
    assert( !client.IsConnecting() );
    assert( !client.IsConnected() );
    assert( client.HasError() );
    assert( client.GetState() == CLIENT_STATE_Disconnected );
    assert( client.GetError() == CLIENT_ERROR_ConnectionRequestTimedOut );
}

void test_client_connection_request_denied()
{
    cout << "test_client_connection_request_denied" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<ClientServerPacketFactory>( channelStructure );

    BSDSocketsConfig bsdSocketsConfig;
    bsdSocketsConfig.port = 10000;
    bsdSocketsConfig.family = AF_INET6;
    bsdSocketsConfig.maxPacketSize = 1024;
    bsdSocketsConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto clientNetworkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

    auto resolver = make_shared<DNSResolver>();

    ClientConfig clientConfig;
    clientConfig.channelStructure = channelStructure;
    clientConfig.networkInterface = clientNetworkInterface;

    Client client( clientConfig );

    client.Connect( "[::1]:10001" );

    bsdSocketsConfig.port = 10001;
    auto serverNetworkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

    ServerConfig serverConfig;
    serverConfig.channelStructure = channelStructure;
    serverConfig.networkInterface = serverNetworkInterface;

    Server server( serverConfig );

    server.Close();     // IMPORTANT: close the server so all connection requests are denied

    assert( client.IsConnecting() );
    assert( !client.IsDisconnected() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_SendingConnectionRequest );

    assert( !server.IsOpen() );

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    for ( int i = 0; i < 256; ++i )
    {
        if ( client.HasError() )
            break;

        client.Update( timeBase );

        server.Update( timeBase );

        this_thread::sleep_for( chrono::milliseconds( 1 ) );

        timeBase.time += timeBase.deltaTime;
    }

    assert( client.IsDisconnected() );
    assert( !client.IsConnecting() );
    assert( !client.IsConnected() );
    assert( client.HasError() );
    assert( client.GetState() == CLIENT_STATE_Disconnected );
    assert( client.GetError() == CLIENT_ERROR_ConnectionRequestDenied );
    assert( client.GetExtendedError() == CONNECTION_REQUEST_DENIED_ServerClosed );
}

void test_client_connection_challenge()
{
    cout << "test_client_connection_challenge" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<ClientServerPacketFactory>( channelStructure );

    BSDSocketsConfig bsdSocketsConfig;
    bsdSocketsConfig.port = 10000;
    bsdSocketsConfig.family = AF_INET6;
    bsdSocketsConfig.maxPacketSize = 1024;
    bsdSocketsConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto clientNetworkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

    auto resolver = make_shared<DNSResolver>();

    ClientConfig clientConfig;
    clientConfig.channelStructure = channelStructure;
    clientConfig.networkInterface = clientNetworkInterface;

    Client client( clientConfig );

    client.Connect( "[::1]:10001" );

    bsdSocketsConfig.port = 10001;
    auto serverNetworkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

    ServerConfig serverConfig;
    serverConfig.channelStructure = channelStructure;
    serverConfig.networkInterface = serverNetworkInterface;

    Server server( serverConfig );

    assert( server.IsOpen() );

    assert( client.IsConnecting() );
    assert( !client.IsDisconnected() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_SendingConnectionRequest );

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    for ( int i = 0; i < 256; ++i )
    {
        if ( client.GetState() == CLIENT_STATE_SendingChallengeResponse )
            break;

        client.Update( timeBase );

        server.Update( timeBase );

        this_thread::sleep_for( chrono::milliseconds( 1 ) );

        timeBase.time += timeBase.deltaTime;
    }

    assert( !client.IsDisconnected() );
    assert( client.IsConnecting() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_SendingChallengeResponse );
    assert( client.GetError() == CLIENT_ERROR_None );
    assert( client.GetExtendedError() == 0 );

    // todo: find client slot and assert one slot connected, and is in correct state
}

void test_client_connection_challenge_response()
{
    cout << "test_client_connection_challenge_response" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<ClientServerPacketFactory>( channelStructure );

    BSDSocketsConfig bsdSocketsConfig;
    bsdSocketsConfig.port = 10000;
    bsdSocketsConfig.family = AF_INET6;
    bsdSocketsConfig.maxPacketSize = 1024;
    bsdSocketsConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto clientNetworkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

    auto resolver = make_shared<DNSResolver>();

    ClientConfig clientConfig;
    clientConfig.channelStructure = channelStructure;
    clientConfig.networkInterface = clientNetworkInterface;

    Client client( clientConfig );

    client.Connect( "[::1]:10001" );

    bsdSocketsConfig.port = 10001;
    auto serverNetworkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

    ServerConfig serverConfig;
    serverConfig.channelStructure = channelStructure;
    serverConfig.networkInterface = serverNetworkInterface;

    Server server( serverConfig );

    assert( server.IsOpen() );

    assert( client.IsConnecting() );
    assert( !client.IsDisconnected() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_SendingConnectionRequest );

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    const int clientIndex = 0;

    for ( int i = 0; i < 256; ++i )
    {
        if ( server.GetClientState(clientIndex) == SERVER_CLIENT_SendingDataBlock )
            break;

        client.Update( timeBase );

        server.Update( timeBase );

        this_thread::sleep_for( chrono::milliseconds( 1 ) );

        timeBase.time += timeBase.deltaTime;
    }

    assert( server.GetClientState(clientIndex) == SERVER_CLIENT_SendingDataBlock );
    assert( !client.IsDisconnected() );
    assert( client.IsConnecting() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_SendingChallengeResponse );
    assert( client.GetError() == CLIENT_ERROR_None );
    assert( client.GetExtendedError() == 0 );
}

/*

void test_client_connection_request_timeout()
{
    cout << "test_client_connection_request_timeout" << endl;

    // ...
}

void test_client_connection_challenge_timeout()
{
    cout << "test_client_connection_challenge_timeout" << endl;

    // ...
}

void test_client_receive_data_timeout()
{
    cout << "test_client_receive_data_timeout" << endl;

    // ...
}

void test_client_send_data_timeout()
{
    cout << "test_client_send_data_timeout" << endl;

    // ...
}

void test_client_connect_by_name()
{
    cout << "test_client_connect_by_name" << endl;

    // ...
}

void test_client_connect_by_address()
{
    cout << "test_client_connect_by_address" << endl;

    // ...
}

void test_client_connect_by_address_string()
{
    cout << "test_client_connect_by_address_string" << endl;

    // ...
}

void test_client_connection()
{
    cout << "test_client_connection" << endl;
}

void test_client_disconnected_from_server()
{
    cout << "test_client_disconnected_from_server" << endl;

    // ...
}

void test_client_connection_timed_out()
{
    cout << "test_client_connection_timed_out" << endl;

    // ...
}
*/

int main()
{
    srand( time( NULL ) );

    try
    {
        test_client_initial_state();
        test_client_resolve_hostname_failure();
        test_client_resolve_hostname_timeout();
        test_client_resolve_hostname_success();
        test_client_connection_request_timeout();
        test_client_connection_request_denied();
        test_client_connection_challenge();
        test_client_connection_challenge_response();

        //test_client_connection_request_succeeded();

        /*
        test_client_connection_challenge_timeout();
        test_client_receive_data_timeout();
        test_client_send_data_timeout();
        test_client_connect_by_name();
        test_client_connect_by_address();
        test_client_connect_by_address_string();
        test_client_connection();
        test_client_disconnected_from_server();
        test_client_connection_timed_out();
        */
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
