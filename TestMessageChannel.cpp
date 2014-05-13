#include "Connection.h"
#include "MessageChannel.h"

using namespace std;
using namespace protocol;

enum PacketType
{
    PACKET_Connection,           // for connection class
    PACKET_Connect,
    PACKET_Update,
    PACKET_Disconnect
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

void test_message_channel()
{
    cout << "test_message_channel" << endl;

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = 4 * 1024;

    Connection connection( connectionConfig );

    auto messageFactory = make_shared<MessageFactory>();

    MessageChannelConfig channelConfig;
    channelConfig.factory = static_pointer_cast<Factory<Message>>( messageFactory );
    
    auto channel = make_shared<MessageChannel>( channelConfig );
    
    connection.AddChannel( channel );

    const int NumMessagesSent = 32;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        auto message = make_shared<TestMessage>();
        message->sequence = i;
        channel->SendMessage( message );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    int numMessagesReceived = 0;

    for ( int i = 0; i < 100; ++i )
    {  
        auto writePacket = connection.WritePacket();

        Stream stream( STREAM_Write );
        writePacket->Serialize( stream );

        stream.SetMode( STREAM_Read );
        auto readPacket = make_shared<ConnectionPacket>( connection.GetInterface(), PACKET_Connection );
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
            auto message = channel->ReceiveMessage();

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

        // todo: test counters in message channel

        timeBase.time += timeBase.deltaTime;
    }
}

int main()
{
    try
    {
        test_message_channel();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
