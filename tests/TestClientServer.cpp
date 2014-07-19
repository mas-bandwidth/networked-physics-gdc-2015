#include "Client.h"
#include "Server.h"
#include "Message.h"
#include "BSDSocket.h"
#include "DNSResolver.h"
#include "Packets.h"
#include "TestPackets.h"
#include "TestMessages.h"
#include "TestChannelStructure.h"
#include "ReliableMessageChannel.h"

#include <thread>
#include <chrono>

using namespace protocol;

void test_client_initial_state()
{
    printf( "test_client_initial_state\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket networkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.networkInterface = &networkInterface;
        clientConfig.channelStructure = &channelStructure;

        Client client( clientConfig );

        PROTOCOL_CHECK( client.IsDisconnected () );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( client.GetNetworkInterface() == &networkInterface );
#if PROTOCOL_USE_RESOLVER
        PROTOCOL_CHECK( client.GetResolver() == nullptr );
#endif
    }

    memory::shutdown();
}

#if PROTOCOL_USE_RESOLVER

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

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

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

        PROTOCOL_CHECK( client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_RESOLVE_HOSTNAME_FAILED );
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

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

        TimeBase timeBase;
        timeBase.deltaTime = 1.0f;

        for ( int i = 0; i < 60; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        PROTOCOL_CHECK( client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_CONNECTION_TIMED_OUT );
        PROTOCOL_CHECK( client.GetExtendedError() == CLIENT_STATE_RESOLVING_HOSTNAME );
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

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

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

        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
    }
}

#endif

void test_client_connection_request_timeout()
{
    printf( "test_client_connection_request_timeout\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        TimeBase timeBase;
        timeBase.deltaTime = 1.0f;

        for ( int i = 0; i < 60; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        PROTOCOL_CHECK( client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_CONNECTION_TIMED_OUT );
        PROTOCOL_CHECK( client.GetExtendedError() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );
    }
}

void test_client_connection_request_denied()
{
    printf( "test_client_connection_request_denied\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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

        PROTOCOL_CHECK( !server.IsOpen() );

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        PROTOCOL_CHECK( client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        PROTOCOL_CHECK( client.GetExtendedError() == CONNECTION_REQUEST_DENIED_SERVER_CLOSED );
    }
}

void test_client_connection_challenge()
{
    printf( "test_client_connection_challenge\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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

        PROTOCOL_CHECK( server.IsOpen() );

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_SENDING_CHALLENGE );

        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );
    }
}

void test_client_connection_challenge_response()
{
    printf( "test_client_connection_challenge_response\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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

        PROTOCOL_CHECK( server.IsOpen() );

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_REQUESTING_CLIENT_DATA );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );
    }
}

void test_client_connection_established()
{
    printf( "test_client_connection_established\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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

        PROTOCOL_CHECK( server.IsOpen() );

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );
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

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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

        PROTOCOL_CHECK( server.IsOpen() );

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );

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

                PROTOCOL_CHECK( message->GetId() == numMessagesReceivedOnClient );
                PROTOCOL_CHECK( message->GetType() == MESSAGE_TEST );

                auto testMessage = static_cast<TestMessage*>( message );

                PROTOCOL_CHECK( testMessage->sequence == numMessagesReceivedOnClient );

                ++numMessagesReceivedOnClient;

                messageFactory.Release( message );
            }

            while ( true )
            {
                auto message = serverMessageChannel->ReceiveMessage();

                if ( !message )
                    break;

                PROTOCOL_CHECK( message->GetId() == numMessagesReceivedOnServer );
                PROTOCOL_CHECK( message->GetType() == MESSAGE_TEST );

                auto testMessage = static_cast<TestMessage*>( message );

                PROTOCOL_CHECK( testMessage->sequence == numMessagesReceivedOnServer );

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

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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

        PROTOCOL_CHECK( server.IsOpen() );

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );

        server.DisconnectClient( clientIndex );

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED );

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_DISCONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_DISCONNECTED_FROM_SERVER );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );
    }
}

void test_client_connection_server_full()
{
    printf( "test_client_connection_server_full\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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

        PROTOCOL_CHECK( server.IsOpen() );

        // connect the maximum number of clients to the server
        // and wait until they are all fully connected.

        Client * clients[NumClients];
        NetworkInterface * clientInterface[NumClients];

        bsdSocketConfig.port = 0;

        for ( int i = 0; i < NumClients; ++i )
        {
            auto clientNetworkInterface = PROTOCOL_NEW( memory::default_allocator(), BSDSocket, bsdSocketConfig );

            PROTOCOL_CHECK( clientNetworkInterface );

            ClientConfig clientConfig;
            clientConfig.channelStructure = &channelStructure;
            clientConfig.networkInterface = clientNetworkInterface;

            auto client = PROTOCOL_NEW( memory::default_allocator(), Client, clientConfig );

            PROTOCOL_CHECK( client );

            client->Connect( "[::1]:10000" );

            PROTOCOL_CHECK( client->IsConnecting() );
            PROTOCOL_CHECK( !client->IsDisconnected() );
            PROTOCOL_CHECK( !client->IsConnected() );
            PROTOCOL_CHECK( !client->HasError() );
            PROTOCOL_CHECK( client->GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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
            PROTOCOL_CHECK( server.GetClientState(i) == SERVER_CLIENT_STATE_CONNECTED );

        for ( auto client : clients )
        {
            PROTOCOL_CHECK( !client->IsDisconnected() );
            PROTOCOL_CHECK( !client->IsConnecting() );
            PROTOCOL_CHECK( client->IsConnected() );
            PROTOCOL_CHECK( !client->HasError() );
            PROTOCOL_CHECK( client->GetState() == CLIENT_STATE_CONNECTED );
            PROTOCOL_CHECK( client->GetError() == CLIENT_ERROR_NONE );
            PROTOCOL_CHECK( client->GetExtendedError() == 0 );
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

        PROTOCOL_CHECK( extraClient.IsConnecting() );
        PROTOCOL_CHECK( !extraClient.IsDisconnected() );
        PROTOCOL_CHECK( !extraClient.IsConnected() );
        PROTOCOL_CHECK( !extraClient.HasError() );
        PROTOCOL_CHECK( extraClient.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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
            PROTOCOL_CHECK( server.GetClientState(i) == SERVER_CLIENT_STATE_CONNECTED );

        for ( auto client : clients )
        {
            PROTOCOL_CHECK( !client->IsDisconnected() );
            PROTOCOL_CHECK( !client->IsConnecting() );
            PROTOCOL_CHECK( client->IsConnected() );
            PROTOCOL_CHECK( !client->HasError() );
            PROTOCOL_CHECK( client->GetState() == CLIENT_STATE_CONNECTED );
            PROTOCOL_CHECK( client->GetError() == CLIENT_ERROR_NONE );
            PROTOCOL_CHECK( client->GetExtendedError() == 0 );
        }

        PROTOCOL_CHECK( extraClient.HasError() );
        PROTOCOL_CHECK( extraClient.IsDisconnected() );
        PROTOCOL_CHECK( !extraClient.IsConnecting() );
        PROTOCOL_CHECK( !extraClient.IsConnected() );
        PROTOCOL_CHECK( extraClient.GetState() == CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( extraClient.GetError() == CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        PROTOCOL_CHECK( extraClient.GetExtendedError() == CONNECTION_REQUEST_DENIED_SERVER_FULL );

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

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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

        PROTOCOL_CHECK( server.IsOpen() );

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );

        // now stop updating the server and verify that the client times out

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        PROTOCOL_CHECK( client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_CONNECTION_TIMED_OUT );    
        PROTOCOL_CHECK( client.GetExtendedError() == CLIENT_STATE_CONNECTED );

        // now update only the server and verify that the client slot times out

        for ( int i = 0; i < 256; ++i )
        {
            if ( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED )
                break;

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED );
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

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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

        PROTOCOL_CHECK( server.IsOpen() );

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );

        // now connect a new client while already connected
        // verify the client connect is *denied* with reason already connected.

        Client newClient( clientConfig );

        newClient.Connect( "[::1]:10001" );

        for ( int i = 0; i < 256; ++i )
        {
            if ( newClient.HasError() )
                break;

            newClient.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        PROTOCOL_CHECK( newClient.IsDisconnected() );
        PROTOCOL_CHECK( !newClient.IsConnecting() );
        PROTOCOL_CHECK( !newClient.IsConnected() );
        PROTOCOL_CHECK( newClient.HasError() );
        PROTOCOL_CHECK( newClient.GetState() == CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( newClient.GetError() == CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        PROTOCOL_CHECK( newClient.GetExtendedError() == CONNECTION_REQUEST_DENIED_ALREADY_CONNECTED );
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

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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

        PROTOCOL_CHECK( server.IsOpen() );

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );

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

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );
    }

    memory::shutdown();
}

void test_client_side_disconnect()
{
    printf( "test_client_side_disconnect\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

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
        serverConfig.connectedTimeOut = 10000;  // IMPORTANT: effectively disables server side timeout
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        PROTOCOL_CHECK( server.IsOpen() );

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );

        // now disconnect the client on on the client side
        // verify the server sees the client disconnected packet
        // and cleans up the slot, rather than slow timeout (disabled)

        client.Disconnect();

        for ( int i = 0; i < 256; ++i )
        {
            if ( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_DISCONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );
    }

    memory::shutdown();
}

void test_client_server_data()
{
    printf( "test_client_server_data\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( memory::default_allocator(), &channelStructure );

        // create a server and set it up with some server data

        const int ServerDataSize = 10 * 1024 + 11;

        Block serverData( memory::default_allocator(), ServerDataSize );
        {
            uint8_t * data = serverData.GetData();
            for ( int i = 0; i < ServerDataSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket serverNetworkInterface( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.serverData = &serverData;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        Server server( serverConfig );

        PROTOCOL_CHECK( server.IsOpen() );

        // connect a client to the server and wait the connect to complete

        bsdSocketConfig.port = 10001;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        BSDSocket clientNetworkInterface( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10000" );

        PROTOCOL_CHECK( client.IsConnecting() );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        PROTOCOL_CHECK( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( !client.IsDisconnected() );
        PROTOCOL_CHECK( !client.IsConnecting() );
        PROTOCOL_CHECK( client.IsConnected() );
        PROTOCOL_CHECK( !client.HasError() );
        PROTOCOL_CHECK( client.GetState() == CLIENT_STATE_CONNECTED );
        PROTOCOL_CHECK( client.GetError() == CLIENT_ERROR_NONE );
        PROTOCOL_CHECK( client.GetExtendedError() == 0 );

        // verify the client has received the server block

        const Block * clientServerData = client.GetServerData();

        PROTOCOL_CHECK( clientServerData );
        PROTOCOL_CHECK( clientServerData->IsValid() );
        PROTOCOL_CHECK( clientServerData->GetData() );
        PROTOCOL_CHECK( clientServerData->GetSize() == ServerDataSize );
        {
            const uint8_t * data = clientServerData->GetData();
            for ( int i = 0; i < ServerDataSize; ++i )
                PROTOCOL_CHECK( data[i] == ( 10 + i ) % 256 );
        }
    }

    memory::shutdown();
}
