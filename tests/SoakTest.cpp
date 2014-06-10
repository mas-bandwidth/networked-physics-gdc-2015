#include "Connection.h"
#include "BSDSockets.h"
#include "NetworkSimulator.h"
#include "ReliableMessageChannel.h"

using namespace std;
using namespace protocol;

enum MessageType
{
    MESSAGE_Block = 0,           // IMPORTANT: 0 is reserved for block messages
    MESSAGE_Test
};

static int messageBitsArray[] = { 1, 320, 120, 4, 256, 45, 11, 13, 101, 100, 84, 95, 203, 2, 3, 8, 512, 1000, 5000, 5, 3, 7, 800 };

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
    }

    void Serialize( Stream & stream )
    {        
        serialize_bits( stream, sequence, 16 );

        int numBits = GetNumBitsForMessage( sequence );
        int numWords = numBits / 32;
        for ( int i = 0; i < numWords; ++i )
        {
            int dummy = 0;
            serialize_bits( stream, dummy, 32 );
        }
        int numRemainderBits = numBits - numWords * 32;
        if ( numRemainderBits > 0 )
        {
            int dummy = 0;
            serialize_bits( stream, dummy, numRemainderBits );
        }

        if ( stream.IsWriting() )
            magic = 0xDEADBEEF;

        serialize_uint32( stream, magic );

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

enum 
{ 
    PACKET_Connection = 0,
    PACKET_Dummy
};

class TestChannelStructure : public ChannelStructure
{
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

class PacketFactory : public Factory<Packet>
{
public:

    PacketFactory( shared_ptr<ChannelStructure> channelStructure )
    {
        Register( PACKET_Connection, [channelStructure] { return make_shared<ConnectionPacket>( PACKET_Connection, channelStructure ); } );
        Register( PACKET_Dummy, [] { return nullptr; } );
    }
};

void soak_test()
{
//    printf( "[soak test]\n" );

    auto channelStructure = make_shared<TestChannelStructure>();
    auto packetFactory = make_shared<PacketFactory>( channelStructure );

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

    BSDSocketsConfig interfaceConfig;
    interfaceConfig.port = 10000;
    interfaceConfig.family = AF_INET6;
    interfaceConfig.maxPacketSize = MaxPacketSize;
    interfaceConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );
    
    BSDSockets interface( interfaceConfig );

    Address address( "::1" );
    address.SetPort( interfaceConfig.port );

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = MaxPacketSize;
    connectionConfig.packetFactory = packetFactory;
    connectionConfig.slidingWindowSize = 1024;
    connectionConfig.channelStructure = channelStructure;

    Connection connection( connectionConfig );

    auto messageChannel = static_pointer_cast<ReliableMessageChannel>( connection.GetChannel( 0 ) );

    auto messageChannelConfig = channelStructure->GetConfig(); 
    
    double dt = 0.01f;
    chrono::milliseconds ms( 1 );

    uint16_t sendMessageId = 0;
    uint64_t numMessagesSent = 0;
    uint64_t numMessagesReceived = 0;

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

            int value = rand() % 10000;

            if ( value < 5000 )
            {
                // bitpacked message
                auto message = make_shared<TestMessage>();
                message->sequence = sendMessageId;
                messageChannel->SendMessage( message );
            }
            else if ( value < 9999 )
            {
                // small block
                int index = sendMessageId % 32;
                auto block = make_shared<Block>( index + 1, 0 );
                for ( int i = 0; i < block->size(); ++i )
                    (*block)[i] = ( index + i ) % 256;
                messageChannel->SendBlock( block );
            }
            else
            {
                // large block
                int index = sendMessageId % 4;
                auto block = make_shared<Block>( (index+1) * 1024 * 1000 + index, 0 );
                for ( int i = 0; i < block->size(); ++i )
                    (*block)[i] = ( index + i ) % 256;
                messageChannel->SendBlock( block );
            }
            
            sendMessageId++;
            numMessagesSent++;
        }

        shared_ptr<Packet> packet = connection.WritePacket();

        simulator.SendPacket( address, packet );
        
        simulator.Update( timeBase );

        packet = simulator.ReceivePacket();

        if ( packet )
            interface.SendPacket( address, packet );

        connection.Update( timeBase );

        this_thread::sleep_for( ms );

        interface.Update( timeBase );

        while ( true )
        {
            auto packet = interface.ReceivePacket();
            if ( !packet )
                break;

            assert( packet->GetAddress() == address );
            assert( packet->GetType() == PACKET_Connection );

            auto connectionPacket = static_pointer_cast<ConnectionPacket>( packet );

            connection.ReadPacket( connectionPacket );
        }

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceived % 65536 );
            assert( message->GetType() == MESSAGE_Block || message->GetType() == MESSAGE_Test );

            if ( message->GetType() == MESSAGE_Test )
            {
//                printf( "%09.2f - received message %d - test message\n", timeBase.time, message->GetId() );
            }
            else
            {
                auto blockMessage = static_pointer_cast<BlockMessage>( message );
                auto block = blockMessage->GetBlock();
                assert( block );
                
                if ( block->size() <= messageChannelConfig.maxSmallBlockSize )
                {
                    const int index = numMessagesReceived % 32;
                    assert( block->size() == index + 1 );
                    for ( int i = 0; i < block->size(); ++i )
                        assert( (*block)[i] == ( index + i ) % 256 );
//                    printf( "%09.2f - received message %d - small block\n", timeBase.time, message->GetId() );
                }
                else
                {
                    const int index = numMessagesReceived % 4;
                    assert( block->size() == (index + 1 ) * 1024 * 1000 + index );
                    for ( int i = 0; i < block->size(); ++i )
                        assert( (*block)[i] == ( index + i ) % 256 );
//                    printf( "%09.2f - received message %d - large block\n", timeBase.time, message->GetId() );
                }
            }

            numMessagesReceived++;
        }

        /*
        auto status = messageChannel->GetReceiveLargeBlockStatus();
        if ( status.receiving )
            printf( "%09.2f - receiving large block %d - %d/%d fragments\n",
                    timeBase.time, 
                    status.blockId, 
                    status.numReceivedFragments, 
                    status.numFragments );
                    */

        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesSent ) == numMessagesSent );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == numMessagesReceived );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesEarly ) == 0 );

        timeBase.time += dt;
    }
}

int main()
{
    srand( time( NULL ) );

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
