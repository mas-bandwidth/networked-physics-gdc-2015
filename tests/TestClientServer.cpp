#include "Client.h"
#include "Server.h"
#include "Message.h"
#include "BSDSocket.h"
#include "DNSResolver.h"
#include "Packets.h"
#include "TestMessages.h"
#include "ReliableMessageChannel.h"

using namespace protocol;

class TestChannelStructure : public ChannelStructure
{
    TestMessageFactory m_messageFactory;
    ReliableMessageChannelConfig m_config;

public:

    TestChannelStructure()
    {
        m_config.messageFactory = &m_messageFactory;

        AddChannel( "reliable message channel", 
                    [this] { return CreateReliableMessageChannel(); }, 
                    [this] { return CreateReliableMessageChannelData(); } );

        Lock();
    }

    ReliableMessageChannel * CreateReliableMessageChannel()
    {
        return new ReliableMessageChannel( m_config );
    }

    ReliableMessageChannelData * CreateReliableMessageChannelData()
    {
        return new ReliableMessageChannelData( m_config );
    }

    const ReliableMessageChannelConfig & GetConfig() const
    {
        return m_config;
    }
};

void test_client_initial_state()
{
    printf( "test_client_initial_state\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto networkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.networkInterface = networkInterface;
        clientConfig.channelStructure = &channelStructure;

        Client client( clientConfig );

        assert( client.IsDisconnected () );
        assert( !client.IsConnected() );
        assert( !client.IsConnecting() );
        assert( !client.HasError() );
        assert( client.GetError() == CLIENT_ERROR_NONE );
        assert( client.GetState() == CLIENT_STATE_DISCONNECTED );
        assert( client.GetNetworkInterface() == networkInterface );
        assert( client.GetResolver() == nullptr );

        delete networkInterface;
    }

    memory::shutdown();
}

void test_client_resolve_hostname_failure()
{
    printf( "test_client_resolve_hostname_failure\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;
        ClientServerPacketFactory packetFactory( &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto networkInterface = new BSDSocket( bsdSocketConfig );

        DNSResolver resolver;

        ClientConfig clientConfig;
        clientConfig.connectingTimeOut = 1000000.0;
        clientConfig.resolver = &resolver;
        clientConfig.networkInterface = networkInterface;
        clientConfig.channelStructure = &channelStructure;

        Client client( clientConfig );

        client.Connect( "my butt" );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

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

        assert( client.IsDisconnected() );
        assert( !client.IsConnecting() );
        assert( !client.IsConnected() );
        assert( client.HasError() );
        assert( client.GetState() == CLIENT_STATE_DISCONNECTED );
        assert( client.GetError() == CLIENT_ERROR_RESOLVE_HOSTNAME_FAILED );

        delete networkInterface;
    }

    memory::shutdown();
}

void test_client_resolve_hostname_timeout()
{
    printf( "test_client_resolve_hostname_timeout\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto networkInterface = new BSDSocket( bsdSocketConfig );

        DNSResolver resolver;

        ClientConfig clientConfig;
        clientConfig.resolver = &resolver;
        clientConfig.networkInterface = networkInterface;
        clientConfig.channelStructure = &channelStructure;

        Client client( clientConfig );

        client.Connect( "my butt" );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

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
        assert( client.GetState() == CLIENT_STATE_DISCONNECTED );
        assert( client.GetError() == CLIENT_ERROR_CONNECTION_TIMED_OUT );
        assert( client.GetExtendedError() == CLIENT_STATE_RESOLVING_HOSTNAME );

        delete networkInterface;
    }
}

void test_client_resolve_hostname_success()
{
    printf( "test_client_resolve_hostname_success\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;
        ClientServerPacketFactory packetFactory( &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto networkInterface = new BSDSocket( bsdSocketConfig );

        DNSResolver resolver;

        ClientConfig clientConfig;
        clientConfig.resolver = &resolver;
        clientConfig.networkInterface = networkInterface;
        clientConfig.channelStructure = &channelStructure;

        Client client( clientConfig );

        client.Connect( "localhost" );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_RESOLVING_HOSTNAME );

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

        assert( !client.IsDisconnected() );
        assert( client.IsConnecting() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );
        assert( client.GetError() == CLIENT_ERROR_NONE );

        delete networkInterface;
    }
}

void test_client_connection_request_timeout()
{
    printf( "test_client_connection_request_timeout\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto networkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.networkInterface = networkInterface;
        clientConfig.channelStructure = &channelStructure;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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
        assert( client.GetState() == CLIENT_STATE_DISCONNECTED );
        assert( client.GetError() == CLIENT_ERROR_CONNECTION_TIMED_OUT );
        assert( client.GetExtendedError() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        delete networkInterface;
    }
}

void test_client_connection_request_denied()
{
    printf( "test_client_connection_request_denied\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.family = AF_INET6;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto clientNetworkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        auto serverNetworkInterface = new BSDSocket( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverNetworkInterface;

        Server server( serverConfig );

        server.Close();     // IMPORTANT: close the server so all connection requests are denied

        assert( !server.IsOpen() );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        assert( client.IsDisconnected() );
        assert( !client.IsConnecting() );
        assert( !client.IsConnected() );
        assert( client.HasError() );
        assert( client.GetState() == CLIENT_STATE_DISCONNECTED );
        assert( client.GetError() == CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        assert( client.GetExtendedError() == CONNECTION_REQUEST_DENIED_SERVER_CLOSED );

        delete clientNetworkInterface;
        delete serverNetworkInterface;
    }
}

void test_client_connection_challenge()
{
    printf( "test_client_connection_challenge\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.family = AF_INET6;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto clientNetworkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        auto serverNetworkInterface = new BSDSocket( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverNetworkInterface;

        Server server( serverConfig );

        assert( server.IsOpen() );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_SENDING_CHALLENGE );

        assert( !client.IsDisconnected() );
        assert( client.IsConnecting() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE );
        assert( client.GetError() == CLIENT_ERROR_NONE );
        assert( client.GetExtendedError() == 0 );

        delete clientNetworkInterface;
        delete serverNetworkInterface;
    }
}

void test_client_connection_challenge_response()
{
    printf( "test_client_connection_challenge_response\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.family = AF_INET6;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto clientNetworkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        auto serverNetworkInterface = new BSDSocket( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverNetworkInterface;

        Server server( serverConfig );

        assert( server.IsOpen() );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_REQUESTING_CLIENT_DATA );
        assert( !client.IsDisconnected() );
        assert( client.IsConnecting() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE );
        assert( client.GetError() == CLIENT_ERROR_NONE );
        assert( client.GetExtendedError() == 0 );

        delete clientNetworkInterface;
        delete serverNetworkInterface;
    }
}

void test_client_connection_established()
{
    printf( "test_client_connection_established\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.family = AF_INET6;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto clientNetworkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        auto serverNetworkInterface = new BSDSocket( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverNetworkInterface;

        Server server( serverConfig );

        assert( server.IsOpen() );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnecting() );
        assert( client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_CONNECTED );
        assert( client.GetError() == CLIENT_ERROR_NONE );
        assert( client.GetExtendedError() == 0 );

        delete clientNetworkInterface;
        delete serverNetworkInterface;
    }

    memory::shutdown();
}

void test_client_connection_messages()
{
    printf( "test_client_connection_messages\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.family = AF_INET6;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto clientNetworkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        auto serverNetworkInterface = new BSDSocket( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverNetworkInterface;

        Server server( serverConfig );

        assert( server.IsOpen() );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnecting() );
        assert( client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_CONNECTED );
        assert( client.GetError() == CLIENT_ERROR_NONE );
        assert( client.GetExtendedError() == 0 );

        auto clientMessageChannel = static_cast<ReliableMessageChannel*>( client.GetConnection()->GetChannel( 0 ) );
        auto serverMessageChannel = static_cast<ReliableMessageChannel*>( server.GetClientConnection( clientIndex )->GetChannel( 0 ) );

        const int NumMessagesSent = 32;

        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            auto message = new TestMessage();
            message->sequence = i;
            clientMessageChannel->SendMessage( message );
        }

        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            auto message = new TestMessage();
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

                assert( message->GetId() == numMessagesReceivedOnClient );
                assert( message->GetType() == MESSAGE_TEST );

                auto testMessage = static_cast<TestMessage*>( message );

                assert( testMessage->sequence == numMessagesReceivedOnClient );

                ++numMessagesReceivedOnClient;

                delete message;
            }

            while ( true )
            {
                auto message = serverMessageChannel->ReceiveMessage();

                if ( !message )
                    break;

                assert( message->GetId() == numMessagesReceivedOnServer );
                assert( message->GetType() == MESSAGE_TEST );

                auto testMessage = static_cast<TestMessage*>( message );

                assert( testMessage->sequence == numMessagesReceivedOnServer );

                ++numMessagesReceivedOnServer;

                delete message;
            }

            if ( numMessagesReceivedOnClient == NumMessagesSent && numMessagesReceivedOnServer == NumMessagesSent )
                break;

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        delete clientNetworkInterface;
        delete serverNetworkInterface;
    }
}

void test_client_connection_disconnect()
{
    printf( "test_client_connection_disconnect\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.family = AF_INET6;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto clientNetworkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        auto serverNetworkInterface = new BSDSocket( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverNetworkInterface;

        Server server( serverConfig );

        assert( server.IsOpen() );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnecting() );
        assert( client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_CONNECTED );
        assert( client.GetError() == CLIENT_ERROR_NONE );
        assert( client.GetExtendedError() == 0 );

        server.DisconnectClient( clientIndex );

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED );

        for ( int i = 0; i < 256; ++i )
        {
            if ( client.GetState() == CLIENT_STATE_DISCONNECTED )
                break;

            client.Update( timeBase );

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED );
        assert( client.IsDisconnected() );
        assert( !client.IsConnecting() );
        assert( !client.IsConnected() );
        assert( client.HasError() );
        assert( client.GetState() == CLIENT_STATE_DISCONNECTED );
        assert( client.GetError() == CLIENT_ERROR_DISCONNECTED_FROM_SERVER );
        assert( client.GetExtendedError() == 0 );

        delete clientNetworkInterface;
        delete serverNetworkInterface;
    }
}

void test_client_connection_server_full()
{
    printf( "test_client_connection_server_full\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        // create a server on port 10000

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.family = AF_INET6;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto serverNetworkInterface = new BSDSocket( bsdSocketConfig );

        const int NumClients = 32;

        ServerConfig serverConfig;
        serverConfig.maxClients = NumClients;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverNetworkInterface;

        Server server( serverConfig );

        assert( server.IsOpen() );

        // connect the maximum number of clients to the server
        // and wait until they are all fully connected.

        // todo: remove std::vector usage. I know how many there are
        Client * clients[NumClients];
        NetworkInterface * clientInterface[NumClients];

        bsdSocketConfig.port = 0;

        for ( int i = 0; i < NumClients; ++i )
        {
            auto clientNetworkInterface = new BSDSocket( bsdSocketConfig );

            assert( clientNetworkInterface );

            ClientConfig clientConfig;
            clientConfig.channelStructure = &channelStructure;
            clientConfig.networkInterface = clientNetworkInterface;

            auto client = new Client( clientConfig );

            assert( client );

            client->Connect( "[::1]:10000" );

            assert( client->IsConnecting() );
            assert( !client->IsDisconnected() );
            assert( !client->IsConnected() );
            assert( !client->HasError() );
            assert( client->GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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
            assert( server.GetClientState(i) == SERVER_CLIENT_STATE_CONNECTED );

        for ( auto client : clients )
        {
            assert( !client->IsDisconnected() );
            assert( !client->IsConnecting() );
            assert( client->IsConnected() );
            assert( !client->HasError() );
            assert( client->GetState() == CLIENT_STATE_CONNECTED );
            assert( client->GetError() == CLIENT_ERROR_NONE );
            assert( client->GetExtendedError() == 0 );
        }

        // now try to connect another client, and verify this client fails to connect
        // with the "server full" connection denied response and the other clients
        // remain connected throughout the test.

        auto clientNetworkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = clientNetworkInterface;

        auto extraClient = new Client( clientConfig );

        extraClient->Connect( "[::1]:10000" );

        assert( extraClient->IsConnecting() );
        assert( !extraClient->IsDisconnected() );
        assert( !extraClient->IsConnected() );
        assert( !extraClient->HasError() );
        assert( extraClient->GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        for ( int i = 0; i < 256; ++i )
        {
            for ( auto client : clients )
                client->Update( timeBase );

            extraClient->Update( timeBase );

            if ( extraClient->HasError() )
                break;

            server.Update( timeBase );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

            timeBase.time += timeBase.deltaTime;
        }

        for ( int i = 0; i < serverConfig.maxClients; ++i )
            assert( server.GetClientState(i) == SERVER_CLIENT_STATE_CONNECTED );

        for ( auto client : clients )
        {
            assert( !client->IsDisconnected() );
            assert( !client->IsConnecting() );
            assert( client->IsConnected() );
            assert( !client->HasError() );
            assert( client->GetState() == CLIENT_STATE_CONNECTED );
            assert( client->GetError() == CLIENT_ERROR_NONE );
            assert( client->GetExtendedError() == 0 );
        }

        assert( extraClient->HasError() );
        assert( extraClient->IsDisconnected() );
        assert( !extraClient->IsConnecting() );
        assert( !extraClient->IsConnected() );
        assert( extraClient->GetState() == CLIENT_STATE_DISCONNECTED );
        assert( extraClient->GetError() == CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        assert( extraClient->GetExtendedError() == CONNECTION_REQUEST_DENIED_SERVER_FULL );

        delete clientNetworkInterface;
        delete serverNetworkInterface;
        delete extraClient;
        for ( int i = 0; i < NumClients; ++i )
            delete clients[i];
        for ( int i = 0; i < NumClients; ++i )
            delete clientInterface[i];
    }

    memory::shutdown(); 
}

void test_client_connection_timeout()
{
    printf( "test_client_connection_timeout\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        // start server and connect one client and wait the client is fully connected

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.family = AF_INET6;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto clientNetworkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        auto serverNetworkInterface = new BSDSocket( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverNetworkInterface;

        Server server( serverConfig );

        assert( server.IsOpen() );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnecting() );
        assert( client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_CONNECTED );
        assert( client.GetError() == CLIENT_ERROR_NONE );
        assert( client.GetExtendedError() == 0 );

        // now stop updating the server and verify that the client times out

        for ( int i = 0; i < 256; ++i )
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
        assert( client.GetState() == CLIENT_STATE_DISCONNECTED );
        assert( client.GetError() == CLIENT_ERROR_CONNECTION_TIMED_OUT );    
        assert( client.GetExtendedError() == CLIENT_STATE_CONNECTED );

        // now update only the server and verify that the client slot times out

        for ( int i = 0; i < 256; ++i )
        {
            if ( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED )
                break;

            server.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_DISCONNECTED );

        delete clientNetworkInterface;
        delete serverNetworkInterface;
    }

    memory::shutdown();
}

void test_client_connection_already_connected()
{
    printf( "test_client_connection_already_connected\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        // start a server and connect a client. wait until the client is fully connected

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.family = AF_INET6;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto clientNetworkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        auto serverNetworkInterface = new BSDSocket( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverNetworkInterface;

        Server server( serverConfig );

        assert( server.IsOpen() );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnecting() );
        assert( client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_CONNECTED );
        assert( client.GetError() == CLIENT_ERROR_NONE );
        assert( client.GetExtendedError() == 0 );

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

        assert( client.IsDisconnected() );
        assert( !client.IsConnecting() );
        assert( !client.IsConnected() );
        assert( client.HasError() );
        assert( client.GetState() == CLIENT_STATE_DISCONNECTED );
        assert( client.GetError() == CLIENT_ERROR_CONNECTION_REQUEST_DENIED );
        assert( client.GetExtendedError() == CONNECTION_REQUEST_DENIED_ALREADY_CONNECTED );

        delete clientNetworkInterface;
        delete serverNetworkInterface;
    }

    memory::shutdown();
}

void test_client_connection_reconnect()
{
    printf( "test_client_connection_reconnect\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        ClientServerPacketFactory packetFactory( &channelStructure );

        // start a server and connect a client. wait until the client is fully connected

        BSDSocketConfig bsdSocketConfig;
        bsdSocketConfig.port = 10000;
        bsdSocketConfig.family = AF_INET6;
        bsdSocketConfig.maxPacketSize = 1024;
        bsdSocketConfig.packetFactory = &packetFactory;

        auto clientNetworkInterface = new BSDSocket( bsdSocketConfig );

        ClientConfig clientConfig;
        clientConfig.channelStructure = &channelStructure;
        clientConfig.networkInterface = clientNetworkInterface;

        Client client( clientConfig );

        client.Connect( "[::1]:10001" );

        bsdSocketConfig.port = 10001;
        auto serverNetworkInterface = new BSDSocket( bsdSocketConfig );

        ServerConfig serverConfig;
        serverConfig.channelStructure = &channelStructure;
        serverConfig.networkInterface = serverNetworkInterface;

        Server server( serverConfig );

        assert( server.IsOpen() );

        assert( client.IsConnecting() );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_SENDING_CONNECTION_REQUEST );

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

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnecting() );
        assert( client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_CONNECTED );
        assert( client.GetError() == CLIENT_ERROR_NONE );
        assert( client.GetExtendedError() == 0 );

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

        assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_STATE_CONNECTED );
        assert( !client.IsDisconnected() );
        assert( !client.IsConnecting() );
        assert( client.IsConnected() );
        assert( !client.HasError() );
        assert( client.GetState() == CLIENT_STATE_CONNECTED );
        assert( client.GetError() == CLIENT_ERROR_NONE );
        assert( client.GetExtendedError() == 0 );

        delete clientNetworkInterface;
        delete serverNetworkInterface;
    }

    memory::shutdown();
}
