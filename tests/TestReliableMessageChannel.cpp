#include "Connection.h"
#include "NetworkSimulator.h"
#include "ReliableMessageChannel.h"

using namespace std;
using namespace protocol;

enum PacketType
{
    PACKET_Connection
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

    template <typename Stream> void Serialize( Stream & stream )
    {        
        serialize_bits( stream, sequence, 16 );

        for ( int i = 0; i < sequence % 8; ++i )
        {
            int value = 0;
            serialize_bits( stream, value, 32 );
        }

        stream.Check( 0xDEADBEEF );
    }

    void SerializeRead( ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( WriteStream & stream )
    {
        Serialize( stream );
    }

    uint16_t sequence;
    uint32_t magic;
};

class MessageFactory : public Factory<Message>
{
public:
    MessageFactory()
    {
        Register( MESSAGE_Block, [] { return new BlockMessage(); } );
        Register( MESSAGE_Test,  [] { return new TestMessage();  } );
    }
};

class TestChannelStructure : public ChannelStructure
{
    ReliableMessageChannelConfig m_config;

public:

    TestChannelStructure()
    {
        m_config.messageFactory = new MessageFactory();

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

class PacketFactory : public Factory<Packet>
{
public:

    PacketFactory( ChannelStructure * channelStructure )
    {
        Register( PACKET_Connection, [channelStructure] { return new ConnectionPacket( PACKET_Connection, channelStructure ); } );
    }
};

void test_reliable_message_channel_messages()
{
    printf( "test_reliable_message_channel_messages\n" );

    TestChannelStructure channelStructure;

    PacketFactory packetFactory( &channelStructure );

    const int MaxPacketSize = 256;

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = MaxPacketSize;
    connectionConfig.packetFactory = &packetFactory;
    connectionConfig.channelStructure = &channelStructure;

    Connection connection( connectionConfig );

    auto messageChannel = static_cast<ReliableMessageChannel*>( connection.GetChannel( 0 ) );
    
    const int NumMessagesSent = 32;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        auto message = new TestMessage();
        message->sequence = i;
        messageChannel->SendMessage( message );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.01f;

    uint64_t numMessagesReceived = 0;

    Address address( "::1" );

    NetworkSimulator simulator;
    simulator.AddState( { 1.0f, 1.0f, 90 } );

    int iteration = 0;

    while ( true )
    {  
        auto writePacket = connection.WritePacket();

        uint8_t buffer[MaxPacketSize];

        WriteStream writeStream( buffer, MaxPacketSize );
        writePacket->SerializeWrite( writeStream );
        writeStream.Flush();

        delete writePacket;

        ReadStream readStream( buffer, MaxPacketSize );
        auto readPacket = new ConnectionPacket( PACKET_Connection, &channelStructure );
        readPacket->SerializeRead( readStream );

        simulator.SendPacket( address, readPacket );

        simulator.Update( timeBase );

        auto packet = simulator.ReceivePacket();

        if ( packet )
        {
            connection.ReadPacket( static_cast<ConnectionPacket*>( packet ) );
            delete packet;
        }

        assert( connection.GetCounter( Connection::PacketsRead ) <= iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsWritten ) == iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsAcked ) <= iteration + 1 );

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceived );
            assert( message->GetType() == MESSAGE_Test );

            auto testMessage = static_cast<TestMessage*>( message );

            assert( testMessage->sequence == numMessagesReceived );

            ++numMessagesReceived;

            delete message;
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;

        connection.Update( timeBase );

        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesSent ) == NumMessagesSent );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == numMessagesReceived );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesEarly ) == 0 );

        timeBase.time += timeBase.deltaTime;

        if ( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == NumMessagesSent )
            break;

        iteration++;
    }
}

void test_reliable_message_channel_small_blocks()
{
    printf( "test_reliable_message_channel_small_blocks\n" );

    TestChannelStructure channelStructure;

    PacketFactory packetFactory( &channelStructure );
    
    MessageFactory messageFactory;

    const int MaxPacketSize = 256;

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = MaxPacketSize;
    connectionConfig.packetFactory = &packetFactory;
    connectionConfig.channelStructure = &channelStructure;

    Connection connection( connectionConfig );

    auto messageChannel = static_cast<ReliableMessageChannel*>( connection.GetChannel( 0 ) );

    auto channelConfig = channelStructure.GetConfig(); 

    const int NumMessagesSent = channelConfig.maxSmallBlockSize;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        auto block = new Block( i + 1 );
        for ( int j = 0; j < block->size(); ++j )
            (*block)[j] = ( i + j ) % 256;
        messageChannel->SendBlock( block );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.01f;

    uint64_t numMessagesReceived = 0;

    int iteration = 0;

    Address address( "::1" );

    NetworkSimulator simulator;
    simulator.AddState( { 1.0f, 1.0f, 90 } );

    while ( true )
    {  
        auto writePacket = connection.WritePacket();

        uint8_t buffer[MaxPacketSize];

        WriteStream writeStream( buffer, MaxPacketSize );
        writePacket->SerializeWrite( writeStream );
        writeStream.Flush();

        ReadStream readStream( buffer, MaxPacketSize );
        auto readPacket = new ConnectionPacket( PACKET_Connection, &channelStructure );
        readPacket->SerializeRead( readStream );

        simulator.SendPacket( address, readPacket );

        simulator.Update( timeBase );

        auto packet = simulator.ReceivePacket();

        if ( packet )
        {
            connection.ReadPacket( static_cast<ConnectionPacket*>( packet ) );
            delete packet;
        }

        assert( connection.GetCounter( Connection::PacketsRead ) <= iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsWritten ) == iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsAcked ) <= iteration + 1 );

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceived );
            assert( message->GetType() == MESSAGE_Block );

            auto blockMessage = static_cast<BlockMessage*>( message );

            auto block = blockMessage->GetBlock();

            assert( block->size() == numMessagesReceived + 1 );
            for ( int i = 0; i < block->size(); ++i )
                assert( (*block)[i] == ( numMessagesReceived + i ) % 256 );

            ++numMessagesReceived;

            delete message;
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;

        connection.Update( timeBase );

        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesSent ) == NumMessagesSent );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == numMessagesReceived );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesEarly ) == 0 );

        timeBase.time += timeBase.deltaTime;

        if ( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == NumMessagesSent )
            break;

        iteration++;
    }
}

void test_reliable_message_channel_large_blocks()
{
    printf( "test_reliable_message_channel_large_blocks\n" );

    TestChannelStructure channelStructure;

    PacketFactory packetFactory( &channelStructure );

    const int MaxPacketSize = 256;

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = MaxPacketSize;
    connectionConfig.packetFactory = &packetFactory;
    connectionConfig.channelStructure = &channelStructure;

    Connection connection( connectionConfig );

    auto messageChannel = static_cast<ReliableMessageChannel*>( connection.GetChannel( 0 ) );

    const int NumMessagesSent = 16;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        auto block = new Block( ( i + 1 ) * 1024 + i );
        for ( int j = 0; j < block->size(); ++j )
            (*block)[j] = ( i + j ) % 256;
        messageChannel->SendBlock( block );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.01f;

    uint64_t numMessagesReceived = 0;

    int iteration = 0;

    Address address( "::1" );

    NetworkSimulator simulator;
    simulator.AddState( { 1.0f, 1.0f, 90 } );

    while ( true )
    {  
        auto writePacket = connection.WritePacket();

        uint8_t buffer[MaxPacketSize];

        WriteStream writeStream( buffer, MaxPacketSize );
        writePacket->SerializeWrite( writeStream );
        writeStream.Flush();

        ReadStream readStream( buffer, MaxPacketSize );
        auto readPacket = new ConnectionPacket( PACKET_Connection, &channelStructure );
        readPacket->SerializeRead( readStream );

        simulator.SendPacket( address, readPacket );

        simulator.Update( timeBase );

        auto packet = simulator.ReceivePacket();

        if ( packet )
        {
            connection.ReadPacket( static_cast<ConnectionPacket*>( packet ) );

            delete packet;
        }
    
        assert( connection.GetCounter( Connection::PacketsRead ) <= iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsWritten ) == iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsAcked ) <= iteration + 1 );

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceived );
            assert( message->GetType() == MESSAGE_Block );

            auto blockMessage = static_cast<BlockMessage*>( message );

            auto block = blockMessage->GetBlock();

//            printf( "received block %d (%d bytes)\n", blockMessage->GetId(), (int) block->size() );

            assert( block->size() == ( numMessagesReceived + 1 ) * 1024 + numMessagesReceived );
            for ( int i = 0; i < block->size(); ++i )
                assert( (*block)[i] == ( numMessagesReceived + i ) % 256 );

            ++numMessagesReceived;

            delete message;
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;

        connection.Update( timeBase );

        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesSent ) == NumMessagesSent );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == numMessagesReceived );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesEarly ) == 0 );

        timeBase.time += timeBase.deltaTime;

        iteration++;
    }

    assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == NumMessagesSent );
}

void test_reliable_message_channel_mixture()
{
    printf( "test_reliable_message_channel_mixture\n" );

    TestChannelStructure channelStructure;

    PacketFactory packetFactory( &channelStructure );

    const int MaxPacketSize = 256;

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = MaxPacketSize;
    connectionConfig.packetFactory = &packetFactory;
    connectionConfig.channelStructure = &channelStructure;

    Connection connection( connectionConfig );

    auto messageChannel = static_cast<ReliableMessageChannel*>( connection.GetChannel( 0 ) );

    const int NumMessagesSent = 256;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        if ( rand() % 10 )
        {
            auto message = new TestMessage();
            message->sequence = i;
            messageChannel->SendMessage( message );
        }
        else
        {
            auto block = new Block( ( i + 1 ) * 8 + i );
            for ( int j = 0; j < block->size(); ++j )
                (*block)[j] = ( i + j ) % 256;
            messageChannel->SendBlock( block );
        }
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.01f;

    uint64_t numMessagesReceived = 0;

    int iteration = 0;

    Address address( "::1" );

    NetworkSimulator simulator;
    simulator.AddState( { 1.0f, 1.0f, 90 } );

    while ( true )
    {  
        auto writePacket = connection.WritePacket();

        uint8_t buffer[MaxPacketSize];

        WriteStream writeStream( buffer, MaxPacketSize );
        writePacket->SerializeWrite( writeStream );
        writeStream.Flush();

        ReadStream readStream( buffer, MaxPacketSize );
        auto readPacket = new ConnectionPacket( PACKET_Connection, &channelStructure );
        readPacket->SerializeRead( readStream );

        simulator.SendPacket( address, readPacket );

        simulator.Update( timeBase );

        auto packet = simulator.ReceivePacket();

        if ( packet )
        {
            connection.ReadPacket( static_cast<ConnectionPacket*>( packet ) );
            delete packet;
        }
        
        assert( connection.GetCounter( Connection::PacketsRead ) <= iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsWritten ) == iteration + 1 );
        assert( connection.GetCounter( Connection::PacketsAcked ) <= iteration + 1 );

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceived );

            if ( message->GetType() == MESSAGE_Block )
            {
                assert( message->GetType() == MESSAGE_Block );

                auto blockMessage = static_cast<BlockMessage*>( message );

                auto block = blockMessage->GetBlock();

//                printf( "received block %d (%d bytes)\n", blockMessage->GetId(), (int) block->size() );

                assert( block->size() == ( numMessagesReceived + 1 ) * 8 + numMessagesReceived );
                for ( int i = 0; i < block->size(); ++i )
                    assert( (*block)[i] == ( numMessagesReceived + i ) % 256 );
            }
            else
            {
                assert( message->GetType() == MESSAGE_Test );

//                printf( "received message %d\n", message->GetId() );

                auto testMessage = static_cast<TestMessage*>( message );

                assert( testMessage->sequence == numMessagesReceived );
            }

            ++numMessagesReceived;

            delete message;
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;

        connection.Update( timeBase );

        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesSent ) == NumMessagesSent );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == numMessagesReceived );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesEarly ) == 0 );

        timeBase.time += timeBase.deltaTime;

        iteration++;
    }

    assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == NumMessagesSent );
}

int main()
{
    srand( time( NULL ) );

    test_reliable_message_channel_messages();
    test_reliable_message_channel_small_blocks();
    test_reliable_message_channel_large_blocks();
    test_reliable_message_channel_mixture();

    return 0;
}
