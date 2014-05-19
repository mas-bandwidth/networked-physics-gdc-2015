#include "Connection.h"
#include "ReliableMessageChannel.h"

using namespace std;
using namespace protocol;

enum PacketType
{
    PACKET_Connection
};

class PacketFactory : public Factory<Packet>
{
public:

    PacketFactory()
    {
        Register( PACKET_Connection, [this] { return make_shared<ConnectionPacket>( PACKET_Connection, m_interface ); } );
    }

    void SetInterface( shared_ptr<ConnectionInterface> interface )
    {
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

        // todo: serialize random bits as a function of sequence #
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

void test_reliable_message_channel()
{
    cout << "test_reliable_message_channel" << endl;

    auto packetFactory = make_shared<PacketFactory>();
    auto messageFactory = make_shared<MessageFactory>();

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = 4 * 1024;
    connectionConfig.packetFactory = packetFactory;

    Connection connection( connectionConfig );

    packetFactory->SetInterface( connection.GetInterface() );

    ReliableMessageChannelConfig channelConfig;
    channelConfig.messageFactory = static_pointer_cast<Factory<Message>>( messageFactory );
    
    auto messageChannel = make_shared<ReliableMessageChannel>( channelConfig );
    
    connection.AddChannel( messageChannel );

    const int NumMessagesSent = 32;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        auto message = make_shared<TestMessage>();
        message->sequence = i;
        messageChannel->SendMessage( message );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    uint64_t numMessagesReceived = 0;

    for ( int i = 0; i < 100; ++i )
    {  
        auto writePacket = connection.WritePacket();

        Stream stream( STREAM_Write );
        writePacket->Serialize( stream );

        stream.SetMode( STREAM_Read );
        auto readPacket = make_shared<ConnectionPacket>( PACKET_Connection, connection.GetInterface() );
        readPacket->Serialize( stream );

        if ( ( rand() % 10 ) == 0 )
            connection.ReadPacket( readPacket );

        assert( connection.GetCounter( Connection::PacketsRead ) <= i+1 );
        assert( connection.GetCounter( Connection::PacketsWritten ) == i+1 );
        assert( connection.GetCounter( Connection::PacketsDiscarded ) == 0 );
        assert( connection.GetCounter( Connection::PacketsAcked ) <= i+1 );
        assert( connection.GetCounter( Connection::ReadPacketFailures ) == 0 );

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceived );
            assert( message->GetType() == MESSAGE_Test );

            auto testMessage = static_pointer_cast<TestMessage>( message );

            assert( testMessage->sequence == numMessagesReceived );

            ++numMessagesReceived;
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;

        connection.Update( timeBase );

        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesSent ) == NumMessagesSent );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == numMessagesReceived );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesDiscardedEarly ) == 0 );

        timeBase.time += timeBase.deltaTime;
    }
}

int main()
{
    try
    {
        test_reliable_message_channel();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
