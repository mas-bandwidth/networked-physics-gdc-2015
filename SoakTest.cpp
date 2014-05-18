#include "Connection.h"
#include "MessageChannel.h"
#include "BSDSocketsInterface.h"

using namespace std;
using namespace protocol;

enum { PACKET_Connection = 0 };

class PacketFactory : public Factory<Packet>
{
public:
    PacketFactory()
    {
        Register( PACKET_Connection, [this] { return make_shared<ConnectionPacket>( PACKET_Connection, m_interface ); } );
    }

    void SetInterface( shared_ptr<ConnectionInterface> interface )
    {
        assert( interface );
        m_interface = interface;
    }

private:

    shared_ptr<ConnectionInterface> m_interface;
};

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
        serialize_int( stream, sequence, 0, 65535 );
    }

    uint16_t sequence;
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

void soak_test()
{
    cout << "[soak test]" << endl;

    auto packetFactory = make_shared<PacketFactory>();
    auto messageFactory = make_shared<MessageFactory>();

    BSDSocketsInterfaceConfig interfaceConfig;
    interfaceConfig.port = 10000;
    interfaceConfig.family = AF_INET6;
    interfaceConfig.maxPacketSize = 4096;
    interfaceConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );
    
    BSDSocketsInterface interface( interfaceConfig );

    Address address( "::1" );
    address.SetPort( interfaceConfig.port );

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = 4 * 1024;
    connectionConfig.packetFactory = packetFactory;

    Connection connection( connectionConfig );

    packetFactory->SetInterface( connection.GetInterface() );

    MessageChannelConfig messageChannelConfig;
    messageChannelConfig.sendQueueSize = 256;
    messageChannelConfig.receiveQueueSize = 64;
    messageChannelConfig.messageFactory = static_pointer_cast<Factory<Message>>( messageFactory );
    
    auto messageChannel = make_shared<MessageChannel>( messageChannelConfig );
    
    connection.AddChannel( messageChannel );

    double dt = 1.0 / 100;
    chrono::milliseconds ms( 1 );

    uint16_t sendMessageId = 0;

    TimeBase timeBase;
    timeBase.time = 0.0;
    timeBase.deltaTime = dt;

    while ( true )
    {
        const int maxMessagesToSend = 1 + rand() % 32;

        for ( int i = 0; i < maxMessagesToSend; ++i )
        {
            if ( !messageChannel->CanSendMessage() )
                break;
            auto message = make_shared<TestMessage>();
            message->sequence = sendMessageId;
            messageChannel->SendMessage( message );
//            cout << format_string( "%09.2f - sent message %d", timeBase.time, message->GetId() ) << endl;
            sendMessageId++;
        }

        auto packet = connection.WritePacket();

//        cout << format_string( "%09.2f - sent packet %d", timeBase.time, packet->sequence ) << endl;

        interface.Update();

        interface.SendPacket( address, packet );

        connection.Update( timeBase );

        this_thread::sleep_for( ms );

        while ( true )
        {
            auto packet = interface.ReceivePacket();
            if ( !packet )
                break;

            if ( rand() % 2 )
                continue;

            assert( packet->GetAddress() == address );
            assert( packet->GetType() == PACKET_Connection );

            auto connectionPacket = static_pointer_cast<ConnectionPacket>( packet );

//            cout << format_string( "%09.2f - received packet %d", timeBase.time, connectionPacket->sequence ) << endl;

            connection.ReadPacket( connectionPacket );
        }

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            cout << format_string( "%09.2f - received message %d", timeBase.time, message->GetId() ) << endl;
        }

        timeBase.time += dt;
    }
}

int main()
{
    if ( !InitializeSockets() )
    {
        cerr << "failed to initialize sockets" << endl;
        return 1;
    }

    try
    {
        soak_test();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    ShutdownSockets();

    return 0;
}
