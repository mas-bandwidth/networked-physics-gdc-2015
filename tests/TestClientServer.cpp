#include "Client.h"
#include "Server.h"
#include "Message.h"
#include "BSDSockets.h"
#include "DNSResolver.h"
#include "ClientServerPackets.h"
#include "ReliableMessageChannel.h"

using namespace std;
using namespace protocol;

enum MessageType
{
    MESSAGE_Block = 0,           // IMPORTANT: 0 is reserved for block messages
    MESSAGE_Test
};

struct TestMessage : public Message
{
    TestMessage() : Message( MESSAGE_Test )
    {
        sequence = 0;
    }

    void Serialize( Stream & stream )
    {        
        serialize_bits( stream, sequence, 16 );
        for ( int i = 0; i < sequence % 8; ++i )
        {
            int value = 0;
            serialize_bits( stream, value, 32 );
        }

        if ( stream.IsWriting() )
            magic = 0xDEADBEEF;

        serialize_bits( stream, magic, 32 );

        assert( magic == 0xDEADBEEF );
    }

    uint16_t sequence;
    uint32_t magic;
};

class MessageFactory : public Factory<Message>
{
public:
    MessageFactory()
    {
        Register( MESSAGE_Block, [] { return make_shared<BlockMessage>(); } );
        Register( MESSAGE_Test, [] { return make_shared<TestMessage>(); } );
    }
};

class TestChannelStructure : public ChannelStructure
{
    ReliableMessageChannelConfig m_config;

public:

    TestChannelStructure()
    {
        m_config.messageFactory = make_shared<MessageFactory>();

        AddChannel( "reliable message channel", 
                    [this] { return CreateReliableMessageChannel(); }, 
                    [this] { return CreateReliableMessageChannelData(); } );

        Lock();
    }

    shared_ptr<ReliableMessageChannel> CreateReliableMessageChannel()
    {
        return make_shared<ReliableMessageChannel>( m_config );
    }

    shared_ptr<ReliableMessageChannelData> CreateReliableMessageChannelData()
    {
        return make_shared<ReliableMessageChannelData>( m_config );
    }

    const ReliableMessageChannelConfig & GetConfig() const
    {
        return m_config;
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
    clientConfig.resolver = make_shared<DNSResolver>( AF_INET6 );
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

    for ( int i = 0; i < 100; ++i )
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
    assert( client.GetError() == CLIENT_ERROR_ConnectionTimedOut );
    assert( client.GetExtendedError() == CLIENT_STATE_ResolvingHostname );
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
    clientConfig.resolver = make_shared<DNSResolver>( AF_INET6 );
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
    assert( client.GetError() == CLIENT_ERROR_ConnectionTimedOut );
    assert( client.GetExtendedError() == CLIENT_STATE_SendingConnectionRequest );
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
        if ( client.GetState() == CLIENT_STATE_SendingChallengeResponse )
            break;

        client.Update( timeBase );

        server.Update( timeBase );

        this_thread::sleep_for( chrono::milliseconds( 1 ) );

        timeBase.time += timeBase.deltaTime;
    }

    assert( server.GetClientState(clientIndex) == SERVER_CLIENT_SendingChallenge );

    assert( !client.IsDisconnected() );
    assert( client.IsConnecting() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_SendingChallengeResponse );
    assert( client.GetError() == CLIENT_ERROR_None );
    assert( client.GetExtendedError() == 0 );
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
        if ( server.GetClientState(clientIndex) == SERVER_CLIENT_RequestingClientData )
            break;

        client.Update( timeBase );

        server.Update( timeBase );

        this_thread::sleep_for( chrono::milliseconds( 1 ) );

        timeBase.time += timeBase.deltaTime;
    }

    assert( server.GetClientState(clientIndex) == SERVER_CLIENT_RequestingClientData );
    assert( !client.IsDisconnected() );
    assert( client.IsConnecting() );
    assert( !client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_SendingChallengeResponse );
    assert( client.GetError() == CLIENT_ERROR_None );
    assert( client.GetExtendedError() == 0 );
}

void test_client_connection_established()
{
    cout << "test_client_connection_established" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<ClientServerPacketFactory>( channelStructure );

    BSDSocketsConfig bsdSocketsConfig;
    bsdSocketsConfig.port = 10000;
    bsdSocketsConfig.family = AF_INET6;
    bsdSocketsConfig.maxPacketSize = 1024;
    bsdSocketsConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto clientNetworkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

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
        if ( client.GetState() == CLIENT_STATE_Connected && server.GetClientState(clientIndex) == SERVER_CLIENT_Connected )
            break;

        client.Update( timeBase );

        server.Update( timeBase );

        this_thread::sleep_for( chrono::milliseconds( 1 ) );

        timeBase.time += timeBase.deltaTime;
    }

    assert( server.GetClientState(clientIndex) == SERVER_CLIENT_Connected );
    assert( !client.IsDisconnected() );
    assert( !client.IsConnecting() );
    assert( client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_Connected );
    assert( client.GetError() == CLIENT_ERROR_None );
    assert( client.GetExtendedError() == 0 );
}

void test_client_connection_messages()
{
    cout << "test_client_connection_messages" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<ClientServerPacketFactory>( channelStructure );

    BSDSocketsConfig bsdSocketsConfig;
    bsdSocketsConfig.port = 10000;
    bsdSocketsConfig.family = AF_INET6;
    bsdSocketsConfig.maxPacketSize = 1024;
    bsdSocketsConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto clientNetworkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

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
        if ( client.GetState() == CLIENT_STATE_Connected && server.GetClientState(clientIndex) == SERVER_CLIENT_Connected )
            break;

        client.Update( timeBase );

        server.Update( timeBase );

        this_thread::sleep_for( chrono::milliseconds( 1 ) );

        timeBase.time += timeBase.deltaTime;
    }

    assert( server.GetClientState(clientIndex) == SERVER_CLIENT_Connected );
    assert( !client.IsDisconnected() );
    assert( !client.IsConnecting() );
    assert( client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_Connected );
    assert( client.GetError() == CLIENT_ERROR_None );
    assert( client.GetExtendedError() == 0 );

    auto clientMessageChannel = static_pointer_cast<ReliableMessageChannel>( client.GetConnection()->GetChannel( 0 ) );
    auto serverMessageChannel = static_pointer_cast<ReliableMessageChannel>( server.GetClientConnection( clientIndex )->GetChannel( 0 ) );

    const int NumMessagesSent = 32;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        auto message = make_shared<TestMessage>();
        message->sequence = i;
        clientMessageChannel->SendMessage( message );
    }

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        auto message = make_shared<TestMessage>();
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
            assert( message->GetType() == MESSAGE_Test );

            auto testMessage = static_pointer_cast<TestMessage>( message );

            assert( testMessage->sequence == numMessagesReceivedOnClient );

            ++numMessagesReceivedOnClient;
        }

        while ( true )
        {
            auto message = serverMessageChannel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceivedOnServer );
            assert( message->GetType() == MESSAGE_Test );

            auto testMessage = static_pointer_cast<TestMessage>( message );

            assert( testMessage->sequence == numMessagesReceivedOnServer );

            ++numMessagesReceivedOnServer;
        }

        if ( numMessagesReceivedOnClient == NumMessagesSent && numMessagesReceivedOnServer == NumMessagesSent )
            break;

        this_thread::sleep_for( chrono::milliseconds( 1 ) );

        timeBase.time += timeBase.deltaTime;

    }
}

void test_client_connection_disconnect()
{
    cout << "test_client_connection_disconnect" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<ClientServerPacketFactory>( channelStructure );

    BSDSocketsConfig bsdSocketsConfig;
    bsdSocketsConfig.port = 10000;
    bsdSocketsConfig.family = AF_INET6;
    bsdSocketsConfig.maxPacketSize = 1024;
    bsdSocketsConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto clientNetworkInterface = make_shared<BSDSockets>( bsdSocketsConfig );

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
        if ( client.GetState() == CLIENT_STATE_Connected && server.GetClientState(clientIndex) == SERVER_CLIENT_Connected )
            break;

        client.Update( timeBase );

        server.Update( timeBase );

        this_thread::sleep_for( chrono::milliseconds( 1 ) );

        timeBase.time += timeBase.deltaTime;
    }

    assert( server.GetClientState(clientIndex) == SERVER_CLIENT_Connected );
    assert( !client.IsDisconnected() );
    assert( !client.IsConnecting() );
    assert( client.IsConnected() );
    assert( !client.HasError() );
    assert( client.GetState() == CLIENT_STATE_Connected );
    assert( client.GetError() == CLIENT_ERROR_None );
    assert( client.GetExtendedError() == 0 );

    server.DisconnectClient( clientIndex );

    assert( server.GetClientState( clientIndex ) == SERVER_CLIENT_Disconnected );

    for ( int i = 0; i < 256; ++i )
    {
        if ( client.GetState() == CLIENT_STATE_Disconnected )
            break;

        client.Update( timeBase );

        server.Update( timeBase );

        this_thread::sleep_for( chrono::milliseconds( 1 ) );

        timeBase.time += timeBase.deltaTime;
    }

    assert( server.GetClientState(clientIndex) == SERVER_CLIENT_Disconnected );
    assert( client.IsDisconnected() );
    assert( !client.IsConnecting() );
    assert( !client.IsConnected() );
    assert( client.HasError() );
    assert( client.GetState() == CLIENT_STATE_Disconnected );
    assert( client.GetError() == CLIENT_ERROR_DisconnectedFromServer );
    assert( client.GetExtendedError() == 0 );
}

/*
    auto serverBlock = make_shared<Block>( 64 * 1024 );
    for ( int i = 0; i < serverBlock->size(); ++i )
        (*serverBlock)[i] = (uint8_t) i;
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
        test_client_connection_established();
        test_client_connection_messages();
        test_client_connection_disconnect();

        // todo: client and server side timeouts need to be tested

        //test_client_connection_timeout();
        //test_client_connection_reconnect();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
