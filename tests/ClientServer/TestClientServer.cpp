#include "core/Core.h"
#include "clientServer/Client.h"
#include "clientServer/Server.h"
#include "clientServer/ClientServerPackets.h"
#include "protocol/Message.h"
#include "protocol/ReliableMessageChannel.h"
#include "network/Network.h"
#include "network/Interface.h"
#include "network/BSDSocket.h"
#include "network/DNSResolver.h"
#include "TestCommon.h"
#include "TestPackets.h"
#include "TestMessages.h"
#include "TestClientServer.h"
#include "TestChannelStructure.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

void test_client_initial_state()
{
    printf( "test_client_initial_state\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket networkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.networkInterface = &networkInterface;
        clientConfig.channelStructure = &channelStructure;

        clientServer::Client client( clientConfig );

        CORE_CHECK( client.IsDisconnected () );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( client.GetNetworkInterface() == &networkInterface );
#if PROTOCOL_USE_RESOLVER
        CORE_CHECK( client.GetResolver() == nullptr );
#endif
    }

    core::memory::shutdown();
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

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

        TimeBase timeBase;
        timeBase.deltaTime = 1.0f;

        for ( int i = 0; i < 100; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        CORE_CHECK( client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( client.HasError() );
        CORE_CHECK( client.GetState() == CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( client.GetError() == CLIENT_ERROR_RESOLVE_HOSTNAME_FAILED );
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

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

        TimeBase timeBase;
        timeBase.deltaTime = 1.0f;

        for ( int i = 0; i < 60; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        CORE_CHECK( client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( client.HasError() );
        CORE_CHECK( client.GetState() == CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( client.GetError() == CLIENT_ERROR_CONNECTION_TIMED_OUT );
        CORE_CHECK( client.GetExtendedError() == CLIENT_STATE_RESOLVING_HOSTNAME );
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

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

        TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        for ( int i = 0; i < 60; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );
        CORE_CHECK( client.GetError() == CLIENT_ERROR_NONE );
    }
}

#endif

void test_client_connection_request_timeout()
{
    printf( "test_client_connection_request_timeout\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket networkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.networkInterface = &networkInterface;
        clientConfig.channelStructure = &channelStructure;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 1.0f;

        for ( int i = 0; i < 60; ++i )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        CORE_CHECK( client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_CONNECTION_TIMED_OUT );
        CORE_CHECK( client.GetExtendedError() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );
    }
}

void test_client_connection_request_denied()
{
    printf( "test_client_connection_request_denied\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        server.Close();     // IMPORTANT: close the server so all connection requests are denied

        CORE_CHECK( !server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        int iteration = 0;

        while ( true )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

    //    printf( "client error: %d\n", client.GetError() );

        CORE_CHECK( client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        CORE_CHECK( client.GetExtendedError() == clientServer::CONNECTION_REQUEST_DENIED_SERVER_CLOSED );
    }
}

void test_client_connection_challenge()
{
    printf( "test_client_connection_challenge\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_SENDING_CHALLENGE_RESPONSE )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_SENDING_CHALLENGE );

        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CHALLENGE_RESPONSE );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );
    }
}

void test_client_connection_challenge_response()
{
    printf( "test_client_connection_challenge_response\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_READY_FOR_CONNECTION )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_READY_FOR_CONNECTION );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CHALLENGE_RESPONSE );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );
    }
}

void test_client_connection_established()
{
    printf( "test_client_connection_established\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );
    }

    core::memory::shutdown();
}

void test_client_connection_messages()
{
    printf( "test_client_connection_messages\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );

        auto clientMessageChannel = static_cast<protocol::ReliableMessageChannel*>( client.GetConnection()->GetChannel( 0 ) );
        auto serverMessageChannel = static_cast<protocol::ReliableMessageChannel*>( server.GetClientConnection( clientIndex )->GetChannel( 0 ) );

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

        while ( true )
        {
            client.Update( timeBase );

            server.Update( timeBase );

            while ( true )
            {
                auto message = clientMessageChannel->ReceiveMessage();

                if ( !message )
                    break;

                CORE_CHECK( message->GetId() == numMessagesReceivedOnClient );
                CORE_CHECK( message->GetType() == MESSAGE_TEST );

                auto testMessage = static_cast<TestMessage*>( message );

                CORE_CHECK( testMessage->sequence == numMessagesReceivedOnClient );

                ++numMessagesReceivedOnClient;

                messageFactory.Release( message );
            }

            while ( true )
            {
                auto message = serverMessageChannel->ReceiveMessage();

                if ( !message )
                    break;

                CORE_CHECK( message->GetId() == numMessagesReceivedOnServer );
                CORE_CHECK( message->GetType() == MESSAGE_TEST );

                auto testMessage = static_cast<TestMessage*>( message );

                CORE_CHECK( testMessage->sequence == numMessagesReceivedOnServer );

                ++numMessagesReceivedOnServer;

                messageFactory.Release( message );
            }

            if ( numMessagesReceivedOnClient == NumMessagesSent && numMessagesReceivedOnServer == NumMessagesSent )
                break;

            timeBase.time += timeBase.deltaTime;
        }
    }
}

void test_client_connection_disconnect()
{
    printf( "test_client_connection_disconnect\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );

        server.DisconnectClient( clientIndex );

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_DISCONNECTED );

        iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_DISCONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_DISCONNECTED_FROM_SERVER );
        CORE_CHECK( client.GetExtendedError() == 0 );
    }
}

void test_client_connection_server_full()
{
    printf( "test_client_connection_server_full\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        // create a server on port 10000

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        const int NumClients = 4;

        clientServer::ServerConfig serverConfig;
        serverConfig.maxClients = NumClients;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        // connect the maximum number of clients to the server
        // and wait until they are all fully connected.

        clientServer::Client * clients[NumClients];
        network::Interface * clientInterface[NumClients];

        bsdSocketConfig.port = 0;

        for ( int i = 0; i < NumClients; ++i )
        {
            auto clientNetworkInterface = CORE_NEW( core::memory::default_allocator(), network::BSDSocket, bsdSocketConfig );

            CORE_CHECK( clientNetworkInterface );

            clientServer::ClientConfig clientConfig;
            clientConfig.channelStructure = &channelStructure;
            clientConfig.networkInterface = clientNetworkInterface;

            auto client = CORE_NEW( core::memory::default_allocator(), clientServer::Client, clientConfig );

            CORE_CHECK( client );

            client->Connect( "[::1]:10000" );

            CORE_CHECK( client->IsConnecting() );
            CORE_CHECK( !client->IsDisconnected() );
            CORE_CHECK( !client->IsConnected() );
            CORE_CHECK( !client->HasError() );
            CORE_CHECK( client->GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

            clients[i] = client;
            clientInterface[i] = clientNetworkInterface;
        }

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        int iteration = 0;

        while ( true )
        {
            int numConnectedClients = 0;
            for ( auto client : clients )
            {
                if ( client->GetState() == clientServer::CLIENT_STATE_CONNECTED )
                    numConnectedClients++;

                client->Update( timeBase );
            }

            if ( numConnectedClients == serverConfig.maxClients )
                break;

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        for ( int i = 0; i < serverConfig.maxClients; ++i )
            CORE_CHECK( server.GetClientState(i) == clientServer::SERVER_CLIENT_STATE_CONNECTED );

        for ( auto client : clients )
        {
            CORE_CHECK( !client->IsDisconnected() );
            CORE_CHECK( !client->IsConnecting() );
            CORE_CHECK( client->IsConnected() );
            CORE_CHECK( !client->HasError() );
            CORE_CHECK( client->GetState() == clientServer::CLIENT_STATE_CONNECTED );
            CORE_CHECK( client->GetError() == clientServer::CLIENT_ERROR_NONE );
            CORE_CHECK( client->GetExtendedError() == 0 );
        }

        // now try to connect another client, and verify this client fails to connect
        // with the "server full" connection denied response and the other clients
        // remain connected throughout the test.

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client extraClient( clientConfig );

        extraClient.Connect( "[::1]:10000" );

        CORE_CHECK( extraClient.IsConnecting() );
        CORE_CHECK( !extraClient.IsDisconnected() );
        CORE_CHECK( !extraClient.IsConnected() );
        CORE_CHECK( !extraClient.HasError() );
        CORE_CHECK( extraClient.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        iteration = 0;

        while( true )
        {
            for ( auto client : clients )
                client->Update( timeBase );

            extraClient.Update( timeBase );

            if ( extraClient.HasError() )
                break;

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        for ( int i = 0; i < serverConfig.maxClients; ++i )
            CORE_CHECK( server.GetClientState(i) == clientServer::SERVER_CLIENT_STATE_CONNECTED );

        for ( auto client : clients )
        {
            CORE_CHECK( !client->IsDisconnected() );
            CORE_CHECK( !client->IsConnecting() );
            CORE_CHECK( client->IsConnected() );
            CORE_CHECK( !client->HasError() );
            CORE_CHECK( client->GetState() == clientServer::CLIENT_STATE_CONNECTED );
            CORE_CHECK( client->GetError() == clientServer::CLIENT_ERROR_NONE );
            CORE_CHECK( client->GetExtendedError() == 0 );
        }

        CORE_CHECK( extraClient.HasError() );
        CORE_CHECK( extraClient.IsDisconnected() );
        CORE_CHECK( !extraClient.IsConnecting() );
        CORE_CHECK( !extraClient.IsConnected() );
        CORE_CHECK( extraClient.GetState() == clientServer::CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( extraClient.GetError() == clientServer::CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        CORE_CHECK( extraClient.GetExtendedError() == clientServer::CONNECTION_REQUEST_DENIED_SERVER_FULL );

        for ( int i = 0; i < NumClients; ++i )
            CORE_DELETE( core::memory::default_allocator(), Client, clients[i] );

        for ( int i = 0; i < NumClients; ++i )
        {
            typedef network::Interface NetworkInterface;
            CORE_DELETE( core::memory::default_allocator(), NetworkInterface, clientInterface[i] );
        }
    }

    core::memory::shutdown(); 
}

void test_client_connection_timeout()
{
    printf( "test_client_connection_timeout\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        // start server and connect one client and wait the client is fully connected

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );

        // now stop updating the server and verify that the client times out

        while ( true )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        CORE_CHECK( client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_CONNECTION_TIMED_OUT );    
        CORE_CHECK( client.GetExtendedError() == clientServer::CLIENT_STATE_CONNECTED );

        // now update only the server and verify that the client slot times out

        iteration = 0;

        while ( true )
        {
            if ( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_DISCONNECTED )
                break;

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_DISCONNECTED );
    }

    core::memory::shutdown();
}

void test_client_connection_already_connected()
{
    printf( "test_client_connection_already_connected\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        // start a server and connect a client. wait until the client is fully connected

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );

        // now connect a new client while already connected
        // verify the client connect is *denied* with reason already connected.

        clientServer::Client newClient( clientConfig );

        newClient.Connect( "[::1]:10001" );

        iteration = 0;

        for ( int i = 0; i < 256; ++i )
        {
            if ( newClient.HasError() )
                break;

            newClient.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( newClient.IsDisconnected() );
        CORE_CHECK( !newClient.IsConnecting() );
        CORE_CHECK( !newClient.IsConnected() );
        CORE_CHECK( newClient.HasError() );
        CORE_CHECK( newClient.GetState() == clientServer::CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( newClient.GetError() == clientServer::CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        CORE_CHECK( newClient.GetExtendedError() == clientServer::CONNECTION_REQUEST_DENIED_ALREADY_CONNECTED );
    }

    core::memory::shutdown();
}

void test_client_connection_reconnect()
{
    printf( "test_client_connection_reconnect\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        // start a server and connect a client. wait until the client is fully connected

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );

        // now disconnect the client on the server and call connect again
        // verify the client can create a new connection to the server.

        server.DisconnectClient( clientIndex );

        client.Connect( "[::1]:10001" );

        iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );
    }

    core::memory::shutdown();
}

void test_client_connection_disconnect_client_side()
{
    printf( "test_client_connection_disconnect_client_side\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        // start a server and connect a client. wait until the client is fully connected

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.connectedTimeOut = 10000;  // IMPORTANT: effectively disables server side timeout
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );

        // now disconnect the client on on the client side
        // verify the server sees the client disconnected packet
        // and cleans up the slot, rather than slow timeout (disabled)

        client.Disconnect();

        iteration = 0;

        while ( true )
        {
            if ( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_DISCONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );
    }

    core::memory::shutdown();
}

void test_server_data()
{
    printf( "test_server_data\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        // create a server and set it up with some server data

        const int ServerDataSize = 10 * 1024 + 11;

        protocol::Block serverData( core::memory::default_allocator(), ServerDataSize );
        {
            uint8_t * data = serverData.GetData();
            for ( int i = 0; i < ServerDataSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.serverData = &serverData;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        // connect a client to the server and wait the connect to complete

        bsdSocketConfig.port = 10001;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10000" );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );

        // verify there is no client data on the server

        CORE_CHECK( server.GetClientData( clientIndex ) == nullptr );

        // verify the client has received the server block

        const protocol::Block * clientServerData = client.GetServerData();

        CORE_CHECK( clientServerData );
        CORE_CHECK( clientServerData->IsValid() );
        CORE_CHECK( clientServerData->GetData() );
        CORE_CHECK( clientServerData->GetSize() == ServerDataSize );
        {
            const uint8_t * data = clientServerData->GetData();
            for ( int i = 0; i < ServerDataSize; ++i )
                CORE_CHECK( data[i] == ( 10 + i ) % 256 );
        }
    }

    core::memory::shutdown();
}

void test_client_data()
{
    printf( "test_client_data\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        // create a server

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        // connect a client with client data to the server and wait the connect to complete

        const int ClientDataSize = 10 * 1024 + 11;

        protocol::Block clientData( core::memory::default_allocator(), ClientDataSize );
        {
            uint8_t * data = clientData.GetData();
            for ( int i = 0; i < ClientDataSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        bsdSocketConfig.port = 10001;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.clientData = &clientData;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10000" );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );

        // verify there is no server data on the client

        CORE_CHECK( client.GetServerData() == nullptr );

        // verify the server has received the client block

        const protocol::Block * serverClientData = server.GetClientData( clientIndex );

        CORE_CHECK( serverClientData );
        CORE_CHECK( serverClientData->IsValid() );
        CORE_CHECK( serverClientData->GetData() );
        CORE_CHECK( serverClientData->GetSize() == ClientDataSize );
        {
            const uint8_t * data = serverClientData->GetData();
            for ( int i = 0; i < ClientDataSize; ++i )
                CORE_CHECK( data[i] == ( 10 + i ) % 256 );
        }
    }

    core::memory::shutdown();
}

void test_client_and_server_data()
{
    printf( "test_client_and_server_data\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        // create a server and set it up with some server data

        const int ServerDataSize = 10 * 1024 + 11;

        protocol::Block serverData( core::memory::default_allocator(), ServerDataSize );
        {
            uint8_t * data = serverData.GetData();
            for ( int i = 0; i < ServerDataSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.serverData = &serverData;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        // connect a client with some client data to the server and wait the connect to complete

        const int ClientDataSize = 10 * 1024 + 11;

        protocol::Block clientData( core::memory::default_allocator(), ClientDataSize );
        {
            uint8_t * data = clientData.GetData();
            for ( int i = 0; i < ClientDataSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        bsdSocketConfig.port = 10001;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.clientData = &clientData;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10000" );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );

        // verify the client has received the server block

        const protocol::Block * clientServerData = client.GetServerData();

        CORE_CHECK( clientServerData );
        CORE_CHECK( clientServerData->IsValid() );
        CORE_CHECK( clientServerData->GetData() );
        CORE_CHECK( clientServerData->GetSize() == ServerDataSize );
        {
            const uint8_t * data = clientServerData->GetData();
            for ( int i = 0; i < ServerDataSize; ++i )
                CORE_CHECK( data[i] == ( 10 + i ) % 256 );
        }

        // verify the server has received the client block

        const protocol::Block * serverClientData = server.GetClientData( clientIndex );

        CORE_CHECK( serverClientData );
        CORE_CHECK( serverClientData->IsValid() );
        CORE_CHECK( serverClientData->GetData() );
        CORE_CHECK( serverClientData->GetSize() == ClientDataSize );
        {
            const uint8_t * data = serverClientData->GetData();
            for ( int i = 0; i < ClientDataSize; ++i )
                CORE_CHECK( data[i] == ( 10 + i ) % 256 );
        }
    }

    core::memory::shutdown();
}

void test_client_and_server_data_reconnect()
{
    printf( "test_client_and_server_data_reconnect\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        // start a server and connect a client. wait until the client is fully connected

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        const int ClientDataSize = 10 * 1024 + 11;

        protocol::Block clientData( core::memory::default_allocator(), ClientDataSize );
        {
            uint8_t * data = clientData.GetData();
            for ( int i = 0; i < ClientDataSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        clientServer::ClientConfig clientConfig;
        clientConfig.clientData = &clientData;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        const int ServerDataSize = 10 * 1024 + 11;

        protocol::Block serverData( core::memory::default_allocator(), ServerDataSize );
        {
            uint8_t * data = serverData.GetData();
            for ( int i = 0; i < ServerDataSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        clientServer::ServerConfig serverConfig;
        serverConfig.serverData = &serverData;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );

        // verify the client has received the server block

        const protocol::Block * clientServerData = client.GetServerData();

        CORE_CHECK( clientServerData );
        CORE_CHECK( clientServerData->IsValid() );
        CORE_CHECK( clientServerData->GetData() );
        CORE_CHECK( clientServerData->GetSize() == ServerDataSize );
        {
            const uint8_t * data = clientServerData->GetData();
            for ( int i = 0; i < ServerDataSize; ++i )
                CORE_CHECK( data[i] == ( 10 + i ) % 256 );
        }

        // verify the server has received the client block

        const protocol::Block * serverClientData = server.GetClientData( clientIndex );

        CORE_CHECK( serverClientData );
        CORE_CHECK( serverClientData->IsValid() );
        CORE_CHECK( serverClientData->GetData() );
        CORE_CHECK( serverClientData->GetSize() == ClientDataSize );
        {
            const uint8_t * data = serverClientData->GetData();
            for ( int i = 0; i < ClientDataSize; ++i )
                CORE_CHECK( data[i] == ( 10 + i ) % 256 );
        }

        // now disconnect the client on the server and call connect again
        // with a new client that has a different client data block.

        client.Disconnect();
        server.DisconnectClient( clientIndex );

        const int NewClientDataSize = 5 * 1024 + 23;

        protocol::Block newClientData( core::memory::default_allocator(), NewClientDataSize );
        {
            uint8_t * data = newClientData.GetData();
            for ( int i = 0; i < NewClientDataSize; ++i )
                data[i] = ( 26 + i ) % 256;
        }

        clientConfig.clientData = &newClientData;

        clientServer::Client newClient( clientConfig );

        newClient.Connect( "[::1]:10001" );

        iteration = 0;

        while ( true )
        {
            if ( newClient.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            newClient.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !newClient.IsDisconnected() );
        CORE_CHECK( !newClient.IsConnecting() );
        CORE_CHECK( newClient.IsConnected() );
        CORE_CHECK( !newClient.HasError() );
        CORE_CHECK( newClient.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( newClient.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( newClient.GetExtendedError() == 0 );

        // verify the new client has received the server block

        const protocol::Block * newClientServerData = newClient.GetServerData();

        CORE_CHECK( newClientServerData );
        CORE_CHECK( newClientServerData->IsValid() );
        CORE_CHECK( newClientServerData->GetData() );
        CORE_CHECK( newClientServerData->GetSize() == ServerDataSize );
        {
            const uint8_t * data = newClientServerData->GetData();
            for ( int i = 0; i < ServerDataSize; ++i )
                CORE_CHECK( data[i] == ( 10 + i ) % 256 );
        }

        // verify the server has received the client block

        const protocol::Block * serverNewClientData = server.GetClientData( clientIndex );

        CORE_CHECK( serverNewClientData );
        CORE_CHECK( serverNewClientData->IsValid() );
        CORE_CHECK( serverNewClientData->GetData() );
        CORE_CHECK( serverNewClientData->GetSize() == NewClientDataSize );
        {
            const uint8_t * data = serverClientData->GetData();
            for ( int i = 0; i < NewClientDataSize; ++i )
                CORE_CHECK( data[i] == ( 26 + i ) % 256 );
        }
    }

    core::memory::shutdown();
}

void test_client_and_server_data_multiple_clients()
{
    printf( "test_client_and_server_data_multiple_clients\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        // create a server on port 10000

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        const int NumClients = 4;

        const int ServerDataSize = 2 * 1024 + 11;

        protocol::Block serverData( core::memory::default_allocator(), ServerDataSize );
        {
            uint8_t * data = serverData.GetData();
            for ( int i = 0; i < ServerDataSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        clientServer::ServerConfig serverConfig;
        serverConfig.serverData = &serverData;
        serverConfig.maxClients = NumClients;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        // connect the maximum number of clients to the server
        // and wait until they are all fully connected.

        clientServer::Client * clients[NumClients];

        network::Interface * clientInterface[NumClients];

        bsdSocketConfig.port = 0;

        protocol::Block * clientData[NumClients];

        for ( int i = 0; i < NumClients; ++i )
        {
            auto clientNetworkInterface = CORE_NEW( core::memory::default_allocator(), network::BSDSocket, bsdSocketConfig );

            CORE_CHECK( clientNetworkInterface );

            const int clientDataSize = 2000 + i * 1024 + i;

            clientData[i] = CORE_NEW( core::memory::default_allocator(), protocol::Block, core::memory::default_allocator(), clientDataSize );
            {
                uint8_t * data = clientData[i]->GetData();
                for ( int j = 0; j < clientDataSize; ++j )
                    data[j] = ( i + j ) % 256;
            }

            clientServer::ClientConfig clientConfig;
            clientConfig.clientData = clientData[i];
            clientConfig.channelStructure = &channelStructure;
            clientConfig.networkInterface = clientNetworkInterface;

            auto client = CORE_NEW( core::memory::default_allocator(), clientServer::Client, clientConfig );

            CORE_CHECK( client );

            client->Connect( "[::1]:10000" );

            CORE_CHECK( client->IsConnecting() );
            CORE_CHECK( !client->IsDisconnected() );
            CORE_CHECK( !client->IsConnected() );
            CORE_CHECK( !client->HasError() );
            CORE_CHECK( client->GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

            clients[i] = client;
            clientInterface[i] = clientNetworkInterface;
        }

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        int iteration = 0;

        while ( true )
        {
            int numConnectedClients = 0;
            for ( auto client : clients )
            {
                if ( client->GetState() == clientServer::CLIENT_STATE_CONNECTED )
                    numConnectedClients++;

                client->Update( timeBase );
            }

            if ( numConnectedClients == serverConfig.maxClients )
                break;

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        for ( int i = 0; i < serverConfig.maxClients; ++i )
        {
            CORE_CHECK( server.GetClientState(i) == clientServer::SERVER_CLIENT_STATE_CONNECTED );

            const protocol::Block * serverClientData = server.GetClientData( i );

            CORE_CHECK( serverClientData );
            CORE_CHECK( serverClientData->IsValid() );
            CORE_CHECK( serverClientData->GetData() );
            CORE_CHECK( serverClientData->GetSize() == clientData[i]->GetSize() );
            {
                const uint8_t * data = serverClientData->GetData();
                const int size = serverClientData->GetSize();
                for ( int j = 0; j < size; ++j )
                    CORE_CHECK( data[j] == ( i + j ) % 256 );
            }
        }

        for ( auto client : clients )
        {
            CORE_CHECK( !client->IsDisconnected() );
            CORE_CHECK( !client->IsConnecting() );
            CORE_CHECK( client->IsConnected() );
            CORE_CHECK( !client->HasError() );
            CORE_CHECK( client->GetState() == clientServer::CLIENT_STATE_CONNECTED );
            CORE_CHECK( client->GetError() == clientServer::CLIENT_ERROR_NONE );
            CORE_CHECK( client->GetExtendedError() == 0 );

            const protocol::Block * clientServerData = client->GetServerData();

            CORE_CHECK( clientServerData );
            CORE_CHECK( clientServerData->IsValid() );
            CORE_CHECK( clientServerData->GetData() );
            CORE_CHECK( clientServerData->GetSize() == ServerDataSize );
            {
                const uint8_t * data = clientServerData->GetData();
                for ( int i = 0; i < ServerDataSize; ++i )
                    CORE_CHECK( data[i] == ( 10 + i ) % 256 );
            }
        }

        for ( int i = 0; i < NumClients; ++i )
        {
            CORE_DELETE( core::memory::default_allocator(), Client, clients[i] );
            CORE_DELETE( core::memory::default_allocator(), Block, clientData[i] );
        }

        for ( int i = 0; i < NumClients; ++i )
        {
            typedef network::Interface NetworkInterface;
            CORE_DELETE( core::memory::default_allocator(), NetworkInterface, clientInterface[i] );
        }
    }

    core::memory::shutdown(); 
}

void test_server_data_too_large()
{
    printf( "test_server_data_too_large\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        // create a server and set it up with some server data

        const int ServerDataSize = 10 * 1024 + 11;

        protocol::Block serverData( core::memory::default_allocator(), ServerDataSize );
        {
            uint8_t * data = serverData.GetData();
            for ( int i = 0; i < ServerDataSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        clientServer::ServerConfig serverConfig;
        serverConfig.serverData = &serverData;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        clientServer::Server server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        // connect a client with a config that doesn't have enough room for the server data

        bsdSocketConfig.port = 10001;
        bsdSocketConfig.maxPacketSize = 1200;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.maxServerDataSize = 22;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        clientServer::Client client( clientConfig );

        client.Connect( "[::1]:10000" );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        int iteration = 0;

        while ( true )
        {
            if ( client.HasError() )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        // verify the client is in error state with server block too large error

        CORE_CHECK( client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_DISCONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_DATA_BLOCK_ERROR );
        CORE_CHECK( client.GetExtendedError() == protocol::DATA_BLOCK_RECEIVER_ERROR_BLOCK_TOO_LARGE );
    }

    core::memory::shutdown();
}

void test_client_server_user_context()
{
    printf( "test_client_server_user_context\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        network::BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        network::BSDSocket clientNetworkInterface( bsdSocketConfig );

        clientServer::ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = &clientNetworkInterface;

        TestClient client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        network::BSDSocket serverNetworkInterface( bsdSocketConfig );

        protocol::Block serverData( core::memory::default_allocator(), sizeof( TestContext ) );
        
        auto testContext = (TestContext*) serverData.GetData();
        testContext->value_min = -1 - ( rand() % 100000 );
        testContext->value_max = rand() % 100000;

        clientServer::ServerConfig serverConfig;
        serverConfig.serverData = &serverData;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = &serverNetworkInterface;

        TestServer server( serverConfig );

        CORE_CHECK( server.IsOpen() );

        CORE_CHECK( client.IsConnecting() );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        const int clientIndex = 0;

        int iteration = 0;

        while ( true )
        {
            if ( client.GetState() == clientServer::CLIENT_STATE_CONNECTED && server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;

            sleep_after_too_many_iterations( iteration );
        }

        CORE_CHECK( server.GetClientState( clientIndex ) == clientServer::SERVER_CLIENT_STATE_CONNECTED );
        CORE_CHECK( !client.IsDisconnected() );
        CORE_CHECK( !client.IsConnecting() );
        CORE_CHECK( client.IsConnected() );
        CORE_CHECK( !client.HasError() );
        CORE_CHECK( client.GetState() == clientServer::CLIENT_STATE_CONNECTED );
        CORE_CHECK( client.GetError() == clientServer::CLIENT_ERROR_NONE );
        CORE_CHECK( client.GetExtendedError() == 0 );

        auto clientMessageChannel = static_cast<protocol::ReliableMessageChannel*>( client.GetConnection()->GetChannel( 0 ) );
        auto serverMessageChannel = static_cast<protocol::ReliableMessageChannel*>( server.GetClientConnection( clientIndex )->GetChannel( 0 ) );

        const int NumMessagesSent = 32;

        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            auto message = (TestContextMessage*) messageFactory.Create( MESSAGE_TEST_CONTEXT );
            message->sequence = i;
            message->value = core::random_int( testContext->value_min, testContext->value_max );
            clientMessageChannel->SendMessage( message );
        }

        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            auto message = (TestContextMessage*) messageFactory.Create( MESSAGE_TEST_CONTEXT );
            message->sequence = i;
            message->value = core::random_int( testContext->value_min, testContext->value_max );
            serverMessageChannel->SendMessage( message );
        }

        int numMessagesReceivedOnClient = 0;
        int numMessagesReceivedOnServer = 0;

        while ( true )
        {
            client.Update( timeBase );

            server.Update( timeBase );

            while ( true )
            {
                auto message = clientMessageChannel->ReceiveMessage();

                if ( !message )
                    break;

                CORE_CHECK( message->GetId() == numMessagesReceivedOnClient );
                CORE_CHECK( message->GetType() == MESSAGE_TEST_CONTEXT );

                auto testContextMessage = static_cast<TestContextMessage*>( message );

                CORE_CHECK( testContextMessage->sequence == numMessagesReceivedOnClient );

                ++numMessagesReceivedOnClient;

                messageFactory.Release( message );
            }

            while ( true )
            {
                auto message = serverMessageChannel->ReceiveMessage();

                if ( !message )
                    break;

                CORE_CHECK( message->GetId() == numMessagesReceivedOnServer );
                CORE_CHECK( message->GetType() == MESSAGE_TEST_CONTEXT );

                auto testContextMessage = static_cast<TestContextMessage*>( message );

                CORE_CHECK( testContextMessage->sequence == numMessagesReceivedOnServer );

                ++numMessagesReceivedOnServer;

                messageFactory.Release( message );
            }

            if ( numMessagesReceivedOnClient == NumMessagesSent && numMessagesReceivedOnServer == NumMessagesSent )
                break;

            timeBase.time += timeBase.deltaTime;
        }
    }
}

int main()
{
    srand( time( nullptr ) );

    if ( !network::InitializeNetwork() )
    {
        printf( "failed to initialize network\n" );
        return 1;
    }

    CORE_ASSERT( network::IsNetworkInitialized() );

    test_client_initial_state();

#if PROTOCOL_USE_RESOLVER
    test_client_resolve_hostname_failure();
    test_client_resolve_hostname_timeout();
    test_client_resolve_hostname_success();
#endif

    test_client_connection_request_timeout();
    test_client_connection_request_denied();
    test_client_connection_challenge();
    test_client_connection_challenge_response();
    test_client_connection_established();
    test_client_connection_messages();
    test_client_connection_disconnect();
    test_client_connection_server_full();
    test_client_connection_timeout();
    test_client_connection_already_connected();
    test_client_connection_reconnect();
    test_client_connection_disconnect_client_side();

    test_server_data();
    test_client_data();
    test_client_and_server_data();
    test_client_and_server_data_reconnect();
    test_client_and_server_data_multiple_clients();
    test_server_data_too_large();

    test_client_server_user_context();

    network::ShutdownNetwork();

    return 0;
}
