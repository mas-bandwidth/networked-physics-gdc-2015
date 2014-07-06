#include "Client.h"
#include "Server.h"
#include "Message.h"
#include "BSDSocket.h"
#include "DNSResolver.h"
#include "Packets.h"
#include "TestMessages.h"
#include "TestCHannelStructure.h"
#include "ReliableMessageChannel.h"

using namespace protocol;

void test_client_initial_state()
{
    printf( "test_client_initial_state\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket networkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.networkInterface = &networkInterface;
        clientConfig.channelStructure = &channelStructure;

        Client client( clientConfig );

        check( client.IsDisconnected () );
        check( !client.IsConnected() );
        check( !client.IsConnecting() );
        check( !client.HasError() );
        check( client.GetError() == CLIENT_ERROR_NONE );
        check( client.GetState() == CLIENT_STATE_DISCONNECTED );
        check( client.GetNetworkInterface() == &networkInterface );
        check( client.GetResolver() == nullptr );
    }

    memory::shutdown();
}

void test_client_resolve_hostname_failure()
{
    printf( "test_client_resolve_hostname_failure\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket networkInterface( bsdSocketConfig );

        DNSResolver resolver;

        ClientConfig clientConfig;
        clientConfig.connectingTimeOut = 1000000.0;
        clientConfig.resolver = &resolver;
        clientConfig.networkInterface = &networkInterface;
        clientConfig.channelStructure = &channelStructure;

        Client client( clientConfig );

        client.Connect( "my butt" );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

        TimeBase timeBase;
        timeBase.deltaTime = 1.0f;

        for ( int i = 0; i < 100; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }

        check( client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( !client.IsConnected() );
        check( client.HasError() );
        check( client.GetState() == CLIENT_STATE_DISCONNECTED );
        check( client.GetError() == CLIENT_ERROR_RESOLVE_HOSTNAME_FAILED );
    }

    memory::shutdown();
}

void test_client_resolve_hostname_timeout()
{
    printf( "test_client_resolve_hostname_timeout\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket networkInterface( bsdSocketConfig );

        DNSResolver resolver;

        ClientConfig clientConfig;
        clientConfig.resolver = &resolver;
        clientConfig.networkInterface = &networkInterface;
        clientConfig.channelStructure = &channelStructure;

        Client client( clientConfig );

        client.Connect( "my butt" );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

        TimeBase timeBase;
        timeBase.deltaTime = 1.0f;

        for ( int i = 0; i < 60; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        check( client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( !client.IsConnected() );
        check( client.HasError() );
        check( client.GetState() == CLIENT_STATE_DISCONNECTED );
        check( client.GetError() == CLIENT_ERROR_CONNECTION_TIMED_OUT );
        check( client.GetExtendedError() == CLIENT_STATE_RESOLVING_HOSTNAME );
    }
}

void test_client_resolve_hostname_success()
{
    printf( "test_client_resolve_hostname_success\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket networkInterface( bsdSocketConfig );

        DNSResolver resolver;

        ClientConfig clientConfig;
        clientConfig.resolver = &resolver;
        clientConfig.networkInterface = &networkInterface;
        clientConfig.channelStructure = &channelStructure;

        Client client( clientConfig );

        client.Connect( "localhost" );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        for ( int i = 0; i < 60; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }

        check( !client.IsDisconnected() );
        check( client.IsConnecting() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );
        check( client.GetError() == CLIENT_ERROR_NONE );
    }
}

void test_client_connection_request_timeout()
{
    printf( "test_client_connection_request_timeout\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket networkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.networkInterface = &networkInterface;
        clientConfig.channelStructure = &channelStructure;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        TimeBase timeBase;
        timeBase.deltaTime = 1.0f;

        for ( int i = 0; i < 60; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        check( client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( !client.IsConnected() );
        check( client.HasError() );
        check( client.GetState() == CLIENT_STATE_DISCONNECTED );
        check( client.GetError() == CLIENT_ERROR_CONNECTION_TIMED_OUT );
        check( client.GetExtendedError() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );
    }
}

void test_client_connection_request_denied()
{
    printf( "test_client_connection_request_denied\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket clientNetworkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        BSDSocket serverNetworkInterface( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        server.Close();     // IMPORTANT: close the server so all connection requests are denied

        check( !server.IsOpen() );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

    //    printf( "client error: %d\n", client.GetError() );

        check( client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( !client.IsConnected() );
        check( client.HasError() );
        check( client.GetState() == CLIENT_STATE_DISCONNECTED );
        check( client.GetError() == CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        check( client.GetExtendedError() == CONNECTION_REQUEST_DENIED_SERVER_CLOSED );
    }
}

void test_client_connection_challenge()
{
    printf( "test_client_connection_challenge\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket clientNetworkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        BSDSocket serverNetworkInterface( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        check( server.IsOpen() );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        const int clientIndex = 0;

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_SENDING_CHALLENGE );

        check( !client.IsDisconnected() );
        check( client.IsConnecting() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE );
        check( client.GetError() == CLIENT_ERROR_NONE );
        check( client.GetExtendedError() == 0 );
    }
}

void test_client_connection_challenge_response()
{
    printf( "test_client_connection_challenge_response\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket clientNetworkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        BSDSocket serverNetworkInterface( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        check( server.IsOpen() );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        const int clientIndex = 0;

        for ( int i = 0; i < 256; ++i )
        {
            if ( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_REQUESTING_CLIENT_DATA )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_REQUESTING_CLIENT_DATA );
        check( !client.IsDisconnected() );
        check( client.IsConnecting() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE );
        check( client.GetError() == CLIENT_ERROR_NONE );
        check( client.GetExtendedError() == 0 );
    }
}

void test_client_connection_established()
{
    printf( "test_client_connection_established\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket clientNetworkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        BSDSocket serverNetworkInterface( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        check( server.IsOpen() );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        const int clientIndex = 0;

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        check( !client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_CONNECTED );
        check( client.GetError() == CLIENT_ERROR_NONE );
        check( client.GetExtendedError() == 0 );
    }

    memory::shutdown();
}

void test_client_connection_messages()
{
    printf( "test_client_connection_messages\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket clientNetworkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        BSDSocket serverNetworkInterface( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        check( server.IsOpen() );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        const int clientIndex = 0;

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        check( !client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_CONNECTED );
        check( client.GetError() == CLIENT_ERROR_NONE );
        check( client.GetExtendedError() == 0 );

        auto clientMessageChannel = static_cast<ReliableMessageChannel*>( client.GetConnection()->GetChannel( 0 ) );
        auto serverMessageChannel = static_cast<ReliableMessageChannel*>( server.GetClientConnection( clientIndex )->GetChannel( 0 ) );

        const int NumMessagesSent = 32;

        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            auto message = (TestMessage*) messageFactory.Create( MESSAGE_TEST );
            message->sequence = i;
            clientMessageChannel->SendMessage( message );
        }

        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            auto message = (TestMessage*) messageFactory.Create( MESSAGE_TEST );
            message->sequence = i;
            serverMessageChannel->SendMessage( message );
        }

        int numMessagesReceivedOnClient = 0;
        int numMessagesReceivedOnServer = 0;

        for ( int i = 0; i < 256; ++i )
        {
            client.Update( timeBase );

            server.Update( timeBase );

            while ( true )
            {
                auto message = clientMessageChannel->ReceiveMessage();

                if ( !message )
                    break;

                check( message->GetId() == numMessagesReceivedOnClient );
                check( message->GetType() == MESSAGE_TEST );

                auto testMessage = static_cast<TestMessage*>( message );

                check( testMessage->sequence == numMessagesReceivedOnClient );

                ++numMessagesReceivedOnClient;

                messageFactory.Release( message );
            }

            while ( true )
            {
                auto message = serverMessageChannel->ReceiveMessage();

                if ( !message )
                    break;

                check( message->GetId() == numMessagesReceivedOnServer );
                check( message->GetType() == MESSAGE_TEST );

                auto testMessage = static_cast<TestMessage*>( message );

                check( testMessage->sequence == numMessagesReceivedOnServer );

                ++numMessagesReceivedOnServer;

                messageFactory.Release( message );
            }

            if ( numMessagesReceivedOnClient == NumMessagesSent && numMessagesReceivedOnServer == NumMessagesSent )
                break;

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }
    }
}

void test_client_connection_disconnect()
{
    printf( "test_client_connection_disconnect\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket clientNetworkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        BSDSocket serverNetworkInterface( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        check( server.IsOpen() );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        const int clientIndex = 0;

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        check( !client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_CONNECTED );
        check( client.GetError() == CLIENT_ERROR_NONE );
        check( client.GetExtendedError() == 0 );

        server.DisconnectClient( clientIndex );

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED );

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_DISCONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED );
        check( client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( !client.IsConnected() );
        check( client.HasError() );
        check( client.GetState() == CLIENT_STATE_DISCONNECTED );
        check( client.GetError() == CLIENT_ERROR_DISCONNECTED_FROM_SERVER );
        check( client.GetExtendedError() == 0 );
    }
}

void test_client_connection_server_full()
{
    printf( "test_client_connection_server_full\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        // create a server on port 10000

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket serverNetworkInterface( bsdSocketConfig );

        const int NumClients = 32;

        ServerConfig serverConfig;
        serverConfig.maxClients = NumClients;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        check( server.IsOpen() );

        // connect the maximum number of clients to the server
        // and wait until they are all fully connected.

        Client * clients[NumClients];
        NetworkInterface * clientInterface[NumClients];

        bsdSocketConfig.port = 0;

        for ( int i = 0; i < NumClients; ++i )
        {
            auto clientNetworkInterface = PROTOCOL_NEW( memory::default_allocator(), BSDSocket, bsdSocketConfig );

            check( clientNetworkInterface );

            ClientConfig clientConfig;
            clientConfig.channelStructure = &channelStructure;
            clientConfig.networkInterface = clientNetworkInterface;

            auto client = PROTOCOL_NEW( memory::default_allocator(), Client, clientConfig );

            check( client );

            client->Connect( "[::1]:10000" );

            check( client->IsConnecting() );
            check( !client->IsDisconnected() );
            check( !client->IsConnected() );
            check( !client->HasError() );
            check( client->GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

            clients[i] = client;
            clientInterface[i] = clientNetworkInterface;
        }

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        for ( int i = 0; i < 256; ++i )
        {
            int numConnectedClients = 0;
            for ( auto client : clients )
            {
                if ( client->GetState() == CLIENT_STATE_CONNECTED )
                    numConnectedClients++;

                client->Update( timeBase );
            }

            if ( numConnectedClients == serverConfig.maxClients )
                break;

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        for ( int i = 0; i < serverConfig.maxClients; ++i )
            check( server.GetClientState(i) == SERVER_CLIENT_STATE_CONNECTED );

        for ( auto client : clients )
        {
            check( !client->IsDisconnected() );
            check( !client->IsConnecting() );
            check( client->IsConnected() );
            check( !client->HasError() );
            check( client->GetState() == CLIENT_STATE_CONNECTED );
            check( client->GetError() == CLIENT_ERROR_NONE );
            check( client->GetExtendedError() == 0 );
        }

        // now try to connect another client, and verify this client fails to connect
        // with the "server full" connection denied response and the other clients
        // remain connected throughout the test.

        BSDSocket clientNetworkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        Client extraClient( clientConfig );

        extraClient.Connect( "[::1]:10000" );

        check( extraClient.IsConnecting() );
        check( !extraClient.IsDisconnected() );
        check( !extraClient.IsConnected() );
        check( !extraClient.HasError() );
        check( extraClient.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        for ( int i = 0; i < 256; ++i )
        {
            for ( auto client : clients )
                client->Update( timeBase );

            extraClient.Update( timeBase );

            if ( extraClient.HasError() )
                break;

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        for ( int i = 0; i < serverConfig.maxClients; ++i )
            check( server.GetClientState(i) == SERVER_CLIENT_STATE_CONNECTED );

        for ( auto client : clients )
        {
            check( !client->IsDisconnected() );
            check( !client->IsConnecting() );
            check( client->IsConnected() );
            check( !client->HasError() );
            check( client->GetState() == CLIENT_STATE_CONNECTED );
            check( client->GetError() == CLIENT_ERROR_NONE );
            check( client->GetExtendedError() == 0 );
        }

        check( extraClient.HasError() );
        check( extraClient.IsDisconnected() );
        check( !extraClient.IsConnecting() );
        check( !extraClient.IsConnected() );
        check( extraClient.GetState() == CLIENT_STATE_DISCONNECTED );
        check( extraClient.GetError() == CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        check( extraClient.GetExtendedError() == CONNECTION_REQUEST_DENIED_SERVER_FULL );

        for ( int i = 0; i < NumClients; ++i )
            PROTOCOL_DELETE( memory::default_allocator(), Client, clients[i] );

        for ( int i = 0; i < NumClients; ++i )
            PROTOCOL_DELETE( memory::default_allocator(), NetworkInterface, clientInterface[i] );
    }

    memory::shutdown(); 
}

void test_client_connection_timeout()
{
    printf( "test_client_connection_timeout\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        // start server and connect one client and wait the client is fully connected

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket clientNetworkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        BSDSocket serverNetworkInterface( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        check( server.IsOpen() );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        const int clientIndex = 0;

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        check( !client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_CONNECTED );
        check( client.GetError() == CLIENT_ERROR_NONE );
        check( client.GetExtendedError() == 0 );

        // now stop updating the server and verify that the client times out

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        check( client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( !client.IsConnected() );
        check( client.HasError() );
        check( client.GetState() == CLIENT_STATE_DISCONNECTED );
        check( client.GetError() == CLIENT_ERROR_CONNECTION_TIMED_OUT );    
        check( client.GetExtendedError() == CLIENT_STATE_CONNECTED );

        // now update only the server and verify that the client slot times out

        for ( int i = 0; i < 256; ++i )
        {
            if ( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED )
                break;

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED );
    }

    memory::shutdown();
}

void test_client_connection_already_connected()
{
    printf( "test_client_connection_already_connected\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        // start a server and connect a client. wait until the client is fully connected

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket clientNetworkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        BSDSocket serverNetworkInterface( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        check( server.IsOpen() );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        const int clientIndex = 0;

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        check( !client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_CONNECTED );
        check( client.GetError() == CLIENT_ERROR_NONE );
        check( client.GetExtendedError() == 0 );

        // now connect the client while already connected
        // verify the client connect is *denied* with reason already connected.

        client.Connect( "[::1]:10001" );

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        check( client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( !client.IsConnected() );
        check( client.HasError() );
        check( client.GetState() == CLIENT_STATE_DISCONNECTED );
        check( client.GetError() == CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        check( client.GetExtendedError() == CONNECTION_REQUEST_DENIED_ALREADY_CONNECTED );
    }

    memory::shutdown();
}

void test_client_connection_reconnect()
{
    printf( "test_client_connection_reconnect\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        ClientServerPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        // start a server and connect a client. wait until the client is fully connected

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket clientNetworkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        BSDSocket serverNetworkInterface( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        check( server.IsOpen() );

        check( client.IsConnecting() );
        check( !client.IsDisconnected() );
        check( !client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        const int clientIndex = 0;

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        check( !client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_CONNECTED );
        check( client.GetError() == CLIENT_ERROR_NONE );
        check( client.GetExtendedError() == 0 );

        // now disconnect the client on the server and call connect again
        // verify the client can create a new connection to the server.

        server.DisconnectClient( clientIndex );

        client.Connect( "[::1]:10001" );

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        check( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        check( !client.IsDisconnected() );
        check( !client.IsConnecting() );
        check( client.IsConnected() );
        check( !client.HasError() );
        check( client.GetState() == CLIENT_STATE_CONNECTED );
        check( client.GetError() == CLIENT_ERROR_NONE );
        check( client.GetExtendedError() == 0 );
    }

    memory::shutdown();
}
