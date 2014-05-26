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
        serialize_bits( stream, sequence, 16 );
        for ( int i = 0; i < sequence % 8; ++i )
        {
            int value = 0;
            serialize_bits( stream, value, 32 );
        }
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

void test_reliable_message_channel_messages()
{
    cout << "test_reliable_message_channel_messages" << endl;

    auto packetFactory = make_shared<PacketFactory>();
    auto messageFactory = make_shared<MessageFactory>();

    const int MaxPacketSize = 256;

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = MaxPacketSize;
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
    timeBase.deltaTime = 0.01f;

    uint64_t numMessagesReceived = 0;

    int iteration = 0;

    while ( true )
    {  
        auto writePacket = connection.WritePacket();

        uint8_t buffer[MaxPacketSize];

        Stream writeStream( STREAM_Write, buffer, MaxPacketSize );
        writePacket->Serialize( writeStream );
        writeStream.Flush();

        Stream readStream( STREAM_Read, buffer, MaxPacketSize );
        auto readPacket = make_shared<ConnectionPacket>( PACKET_Connection, connection.GetInterface() );
        readPacket->Serialize( readStream );

        if ( ( rand() % 10 ) == 0 )
            connection.ReadPacket( readPacket );

        assert( connection.GetCounter( Connection::PacketsRead ) <= iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsWritten ) == iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsDiscarded ) == 0 );
        assert( connection.GetCounter( Connection::PacketsAcked ) <= iteration + 1 );
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

        if ( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == NumMessagesSent )
            break;

        iteration++;
    }
}

void test_reliable_message_channel_small_blocks()
{
    cout << "test_reliable_message_channel_small_blocks" << endl;

    auto packetFactory = make_shared<PacketFactory>();
    auto messageFactory = make_shared<MessageFactory>();

    const int MaxPacketSize = 256;

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = MaxPacketSize;
    connectionConfig.packetFactory = packetFactory;

    Connection connection( connectionConfig );

    packetFactory->SetInterface( connection.GetInterface() );

    ReliableMessageChannelConfig channelConfig;
    channelConfig.messageFactory = static_pointer_cast<Factory<Message>>( messageFactory );
    
    auto messageChannel = make_shared<ReliableMessageChannel>( channelConfig );
    
    connection.AddChannel( messageChannel );

    const int NumMessagesSent = channelConfig.maxSmallBlockSize;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        auto block = make_shared<Block>( i + 1, i );
        messageChannel->SendBlock( block );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.01f;

    uint64_t numMessagesReceived = 0;

    int iteration = 0;

    while ( true )
    {  
        auto writePacket = connection.WritePacket();

        uint8_t buffer[MaxPacketSize];

        Stream writeStream( STREAM_Write, buffer, MaxPacketSize );
        writePacket->Serialize( writeStream );
        writeStream.Flush();

        Stream readStream( STREAM_Read, buffer, MaxPacketSize );
        auto readPacket = make_shared<ConnectionPacket>( PACKET_Connection, connection.GetInterface() );
        readPacket->Serialize( readStream );

        if ( ( rand() % 10 ) == 0 )
            connection.ReadPacket( readPacket );

        assert( connection.GetCounter( Connection::PacketsRead ) <= iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsWritten ) == iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsDiscarded ) == 0 );
        assert( connection.GetCounter( Connection::PacketsAcked ) <= iteration + 1 );
        assert( connection.GetCounter( Connection::ReadPacketFailures ) == 0 );

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceived );
            assert( message->GetType() == MESSAGE_Block );

            auto blockMessage = static_pointer_cast<BlockMessage>( message );

            auto block = blockMessage->GetBlock();

            assert( block->size() == numMessagesReceived + 1 );
            for ( auto c : *block )
                assert( c == numMessagesReceived );

            // todo: must verify actual contents of block, eg. make it a modulus 
            // of sequence # and current byte index in the block or something

            ++numMessagesReceived;
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;

        connection.Update( timeBase );

        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesSent ) == NumMessagesSent );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == numMessagesReceived );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesDiscardedEarly ) == 0 );

        timeBase.time += timeBase.deltaTime;

        if ( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == NumMessagesSent )
            break;

        iteration++;
    }
}

void test_reliable_message_channel_large_blocks()
{
    cout << "test_reliable_message_channel_large_blocks" << endl;

    auto packetFactory = make_shared<PacketFactory>();
    auto messageFactory = make_shared<MessageFactory>();

    const int MaxPacketSize = 256;

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = MaxPacketSize;
    connectionConfig.packetFactory = packetFactory;

    Connection connection( connectionConfig );

    packetFactory->SetInterface( connection.GetInterface() );

    ReliableMessageChannelConfig channelConfig;
    channelConfig.messageFactory = static_pointer_cast<Factory<Message>>( messageFactory );
    
    auto messageChannel = make_shared<ReliableMessageChannel>( channelConfig );
    
    connection.AddChannel( messageChannel );

    const int NumMessagesSent = 64;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        auto block = make_shared<Block>( ( i + 1 ) * 1024 + i, i + 1 );
        messageChannel->SendBlock( block );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.01f;

    uint64_t numMessagesReceived = 0;

    int iteration = 0;

    while ( true )
    {  
        auto writePacket = connection.WritePacket();

        uint8_t buffer[MaxPacketSize];

        Stream writeStream( STREAM_Write, buffer, MaxPacketSize );
        writePacket->Serialize( writeStream );
        writeStream.Flush();

        Stream readStream( STREAM_Read, buffer, MaxPacketSize );
        auto readPacket = make_shared<ConnectionPacket>( PACKET_Connection, connection.GetInterface() );
        readPacket->Serialize( readStream );

        if ( ( rand() % 10 ) == 0 )
            connection.ReadPacket( readPacket );

        assert( connection.GetCounter( Connection::PacketsRead ) <= iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsWritten ) == iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsDiscarded ) == 0 );
        assert( connection.GetCounter( Connection::PacketsAcked ) <= iteration + 1 );

        // todo: i really dislike this counter name. it sounds like a bad #
        // but in fact it is going to naturally be non-zero under high latency.

        //assert( connection.GetCounter( Connection::ReadPacketFailures ) == 0 );

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceived );
            assert( message->GetType() == MESSAGE_Block );

            auto blockMessage = static_pointer_cast<BlockMessage>( message );

            auto block = blockMessage->GetBlock();

            cout << "received block " << blockMessage->GetId() << " (" << block->size() << " bytes)" << endl;

            assert( block->size() == ( numMessagesReceived + 1 ) * 1024 + numMessagesReceived );
            for ( auto c : *block )
                assert( c == numMessagesReceived + 1 );

            // todo: must verify actual contents of block, eg. in such a way that
            // the byte of each block is unique, not always the same value. too easy
            // to have a broken implementation slip through otherwise!

            ++numMessagesReceived;
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;

        connection.Update( timeBase );

        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesSent ) == NumMessagesSent );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == numMessagesReceived );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesDiscardedEarly ) == 0 );

        timeBase.time += timeBase.deltaTime;

        iteration++;
    }

    assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == NumMessagesSent );
}

int main()
{
    try
    {
        test_reliable_message_channel_messages();
        test_reliable_message_channel_small_blocks();
        test_reliable_message_channel_large_blocks();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
