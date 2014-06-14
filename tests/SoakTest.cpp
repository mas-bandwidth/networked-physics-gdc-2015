#include "Connection.h"
#include "BSDSockets.h"
#include "NetworkSimulator.h"
#include "ReliableMessageChannel.h"

using namespace protocol;

enum MessageType
{
    MESSAGE_Block = 0,           // IMPORTANT: 0 is reserved for block messages
    MESSAGE_Test
};

static int messageBitsArray[] = { 1, 320, 120, 4, 256, 45, 11, 13, 101, 100, 84, 95, 203, 2, 3, 8, 512, 5, 3, 7, 50 };

int GetNumBitsForMessage( uint16_t sequence )
{
    const int modulus = sizeof( messageBitsArray ) / sizeof( int );
    const int index = sequence % modulus;
    return messageBitsArray[index];
}

struct TestMessage : public Message
{
    TestMessage() : Message( MESSAGE_Test )
    {
        sequence = 0;
        dummy = 0;
    }

    template <typename Stream> void Serialize( Stream & stream )
    {        
        serialize_bits( stream, sequence, 16 );

        int numBits = GetNumBitsForMessage( sequence );
        int numWords = numBits / 32;
        for ( int i = 0; i < numWords; ++i )
            serialize_bits( stream, dummy, 32 );
        int numRemainderBits = numBits - numWords * 32;
        if ( numRemainderBits > 0 )
            serialize_bits( stream, dummy, numRemainderBits );

        serialize_check( stream, 0xDEADBEEF );
    }

    void SerializeRead( ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( MeasureStream & stream )
    {
        Serialize( stream );
    }

    uint16_t sequence;
    uint32_t dummy;
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

enum 
{ 
    PACKET_Connection = 0,
    PACKET_Dummy
};

class TestChannelStructure : public ChannelStructure
{
    MessageFactory m_messageFactory;
    ReliableMessageChannelConfig m_config;

public:

    TestChannelStructure()
    {
        m_config.maxMessagesPerPacket = 256;
        m_config.sendQueueSize = 2048;
        m_config.receiveQueueSize = 512;
        m_config.packetBudget = 4000;
        m_config.maxMessageSize = 1024;
        m_config.blockFragmentSize = 3900;
        m_config.maxLargeBlockSize = 32 * 1024 * 1024;
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

class PacketFactory : public Factory<Packet>
{
public:

    PacketFactory( ChannelStructure * channelStructure )
    {
        Register( PACKET_Connection, [channelStructure] { return new ConnectionPacket( PACKET_Connection, channelStructure ); } );
        Register( PACKET_Dummy, [] { return nullptr; } );
    }
};

void soak_test()
{
#if !PROFILE
    printf( "[soak test]\n" );
#endif

    TestChannelStructure channelStructure;

    PacketFactory packetFactory( &channelStructure );

    const int MaxPacketSize = 4096;

    NetworkSimulator simulator;
    simulator.AddState( { 0.0f, 0.0f, 0.0f } );
    simulator.AddState( { 0.0f, 0.0f, 0.0f } );
    simulator.AddState( { 0.1f, 0.01f, 1.0f } );
    simulator.AddState( { 0.1f, 0.01f, 1.0f } );
    simulator.AddState( { 0.2f, 0.10f, 5.0f } );
    simulator.AddState( { 0.3f, 0.20f, 10.0f } );
    simulator.AddState( { 0.5f, 0.25f, 25.0f } );
    simulator.AddState( { 1.0f, 0.50f, 50.0f } );
    simulator.AddState( { 1.0f, 1.00f, 100.0f } );

    Address address( "::1" );

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = MaxPacketSize;
    connectionConfig.packetFactory = &packetFactory;
    connectionConfig.slidingWindowSize = 1024;
    connectionConfig.channelStructure = &channelStructure;

    Connection connection( connectionConfig );

    auto messageChannel = static_cast<ReliableMessageChannel*>( connection.GetChannel( 0 ) );

    auto messageChannelConfig = channelStructure.GetConfig(); 
    
    uint16_t sendMessageId = 0;
    uint64_t numMessagesSent = 0;
    uint64_t numMessagesReceived = 0;

    TimeBase timeBase;
    timeBase.time = 0.0;
    timeBase.deltaTime = 0.01;

    while ( true )
    //for ( int i = 0; i < 1000; ++i )
    {
        const int maxMessagesToSend = 1 + rand() % 32;

        for ( int i = 0; i < maxMessagesToSend; ++i )
        {
            if ( !messageChannel->CanSendMessage() )
                break;

            int value = rand() % 10000;

            if ( value < 5000 )
            {
                // bitpacked message
                auto message = new TestMessage();
                message->sequence = sendMessageId;
                messageChannel->SendMessage( message );
            }
            else if ( value < 9999 )
            {
                // small block
                int index = sendMessageId % 32;
                auto block = new Block( index + 1 );
                for ( int i = 0; i < block->size(); ++i )
                    (*block)[i] = ( index + i ) % 256;
                messageChannel->SendBlock( block );
            }
            else
            {
                // large block
                int index = sendMessageId % 4;
                auto block = new Block( (index+1) * 1024 * 1000 + index );
                for ( int i = 0; i < block->size(); ++i )
                    (*block)[i] = ( index + i ) % 256;
                messageChannel->SendBlock( block );
            }
            
            sendMessageId++;
            numMessagesSent++;
        }

        ConnectionPacket * writePacket = connection.WritePacket();

        uint8_t buffer[MaxPacketSize];

        WriteStream writeStream( buffer, MaxPacketSize );
        writePacket->SerializeWrite( writeStream );
        writeStream.Flush();
        assert( !writeStream.IsOverflow() );

        delete writePacket;

        ReadStream readStream( buffer, MaxPacketSize );
        auto readPacket = new ConnectionPacket( PACKET_Connection, &channelStructure );
        readPacket->SerializeRead( readStream );
        assert( !readStream.IsOverflow() );

        connection.ReadPacket( static_cast<ConnectionPacket*>( readPacket ) );

        delete readPacket;

        connection.Update( timeBase );

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceived % 65536 );
            assert( message->GetType() == MESSAGE_Block || message->GetType() == MESSAGE_Test );

            if ( message->GetType() == MESSAGE_Test )
            {
#if !PROFILE
                printf( "%09.2f - received message %d - test message\n", timeBase.time, message->GetId() );
#endif
            }
            else
            {
                auto blockMessage = static_cast<BlockMessage*>( message );
                auto block = blockMessage->GetBlock();
                assert( block );
                
                if ( block->size() <= messageChannelConfig.maxSmallBlockSize )
                {
                    const int index = numMessagesReceived % 32;
                    assert( block->size() == index + 1 );
                    for ( int i = 0; i < block->size(); ++i )
                        assert( (*block)[i] == ( index + i ) % 256 );
#if !PROFILE
                    printf( "%09.2f - received message %d - small block\n", timeBase.time, message->GetId() );
#endif
                }
                else
                {
                    const int index = numMessagesReceived % 4;
                    assert( block->size() == (index + 1 ) * 1024 * 1000 + index );
                    for ( int i = 0; i < block->size(); ++i )
                        assert( (*block)[i] == ( index + i ) % 256 );
#if !PROFILE
                    printf( "%09.2f - received message %d - large block\n", timeBase.time, message->GetId() );
#endif
                }
            }

            numMessagesReceived++;

            delete message;
        }

#if !PROFILE
        auto status = messageChannel->GetReceiveLargeBlockStatus();
        if ( status.receiving )
            printf( "%09.2f - receiving large block %d - %d/%d fragments\n",
                    timeBase.time, 
                    status.blockId, 
                    status.numReceivedFragments, 
                    status.numFragments );
#endif

        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesSent ) == numMessagesSent );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == numMessagesReceived );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesEarly ) == 0 );

        timeBase.time += timeBase.deltaTime;
    }
}

int main()
{
    srand( time( NULL ) );

    if ( !InitializeSockets() )
    {
        printf( "failed to initialize sockets\n" );
        return 1;
    }

    memory::initialize();

    soak_test();

    memory::shutdown();

    ShutdownSockets();

    return 0;
}
