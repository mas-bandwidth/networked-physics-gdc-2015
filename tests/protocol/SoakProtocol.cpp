#include "protocol/Connection.h"
#include "protocol/ReliableMessageChannel.h"
#include "network/Simulator.h"
#include "TestMessages.h"
#include "TestPackets.h"
#include <time.h>

class TestChannelStructure : public protocol::ChannelStructure
{
    protocol::ReliableMessageChannelConfig m_config;

public:

    TestChannelStructure( protocol::MessageFactory & messageFactory )
        : ChannelStructure( core::memory::default_allocator(), core::memory::scratch_allocator(), 1 )
    {
        m_config.maxMessagesPerPacket = 256;
        m_config.sendQueueSize = 2048;
        m_config.receiveQueueSize = 512;
        m_config.packetBudget = 4000;
        m_config.maxMessageSize = 1024;
        m_config.blockFragmentSize = 3900;
        m_config.maxLargeBlockSize = 32 * 1024 * 1024;
        m_config.messageFactory = &messageFactory;
        m_config.messageAllocator = &core::memory::default_allocator();
        m_config.smallBlockAllocator = &core::memory::default_allocator();
        m_config.largeBlockAllocator = &core::memory::default_allocator();
    }

    const protocol::ReliableMessageChannelConfig & GetConfig() const
    {
        return m_config;
    }

protected:

    const char * GetChannelNameInternal( int channelIndex ) const
    {
        return "reliable message channel";
    }

    protocol::Channel * CreateChannelInternal( int channelIndex )
    {
        return CORE_NEW( GetChannelAllocator(), protocol::ReliableMessageChannel, m_config );
    }

    protocol::ChannelData * CreateChannelDataInternal( int channelIndex )
    {   
        return CORE_NEW( GetChannelDataAllocator(), protocol::ReliableMessageChannelData, m_config );
    }
};

void soak_test()
{
#if PROFILE
    printf( "[profile protocol]\n" );
#else
    printf( "[soak protocol]\n" );
#endif

    TestMessageFactory messageFactory( core::memory::default_allocator() );

    TestChannelStructure channelStructure( messageFactory );

    TestPacketFactory packetFactory( core::memory::default_allocator() );

    const void * context[protocol::MaxContexts];
    memset( context, 0, sizeof( context ) );
    context[protocol::CONTEXT_CONNECTION] = &channelStructure;

    const int MaxPacketSize = 4096;

    network::SimulatorConfig simulatorConfig;
    simulatorConfig.maxPacketSize = MaxPacketSize;
    simulatorConfig.packetFactory = &packetFactory;
    network::Simulator simulator( simulatorConfig );
    simulator.SetContext( context );
    simulator.AddState( { 0.0f, 0.0f, 0.0f } );
    simulator.AddState( { 0.1f, 0.01f, 1.0f } );
    simulator.AddState( { 0.1f, 0.01f, 1.0f } );
    simulator.AddState( { 0.2f, 0.10f, 5.0f } );
    simulator.AddState( { 0.3f, 0.20f, 10.0f } );
    simulator.AddState( { 0.5f, 0.25f, 25.0f } );
    simulator.AddState( { 1.0f, 0.50f, 50.0f } );
    simulator.AddState( { 1.0f, 1.00f, 100.0f } );

    network::Address address( "::1" );

    protocol::ConnectionConfig connectionConfig;
    connectionConfig.maxPacketSize = MaxPacketSize;
    connectionConfig.packetFactory = &packetFactory;
    connectionConfig.slidingWindowSize = 1024;
    connectionConfig.channelStructure = &channelStructure;

    protocol::Connection connection( connectionConfig );

    auto messageChannel = static_cast<protocol::ReliableMessageChannel*>( connection.GetChannel( 0 ) );

    auto messageChannelConfig = channelStructure.GetConfig(); 
    
    uint16_t sendMessageId = 0;
    uint64_t numMessagesSent = 0;
    uint64_t numMessagesReceived = 0;

    core::TimeBase timeBase;
    timeBase.time = 0.0;
    timeBase.deltaTime = 0.01;

//    for ( int i = 0; i < 10000; ++i )
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
                auto message = (TestMessage*) messageFactory.Create( MESSAGE_TEST );
                CORE_CHECK( message );
                message->sequence = sendMessageId;
                messageChannel->SendMessage( message );
            }
            else if ( value < 9999 )
            {
                // small block
                int index = sendMessageId % 32;
                protocol::Block block( core::memory::default_allocator(), index + 1 );
                uint8_t * data = block.GetData();
                for ( int i = 0; i < block.GetSize(); ++i )
                    data[i] = ( index + i ) % 256;
                messageChannel->SendBlock( block );
            }
            else
            {
                // large block
                int index = sendMessageId % 4;
                protocol::Block block( core::memory::default_allocator(), (index+1) * 1024 * 1000 + index );
                uint8_t * data = block.GetData();
                for ( int i = 0; i < block.GetSize(); ++i )
                    data[i] = ( index + i ) % 256;
                messageChannel->SendBlock( block );
            }
            
            sendMessageId++;
            numMessagesSent++;
        }

        protocol::ConnectionPacket * writePacket = connection.WritePacket();

        simulator.SendPacket( writePacket->GetAddress(), writePacket );

        simulator.Update( timeBase );

        while ( true )
        {
            auto packet = simulator.ReceivePacket();
            if ( !packet )
                break;

            connection.ReadPacket( static_cast<protocol::ConnectionPacket*>( packet ) );

            packetFactory.Destroy( packet );
        }

        connection.Update( timeBase );

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            CORE_CHECK( message->GetId() == numMessagesReceived % 65536 );
            CORE_CHECK( message->GetType() == MESSAGE_BLOCK || message->GetType() == MESSAGE_TEST );

            if ( message->GetType() == MESSAGE_TEST )
            {
#if !PROFILE
                printf( "%09.2f - received message %d - test message\n", timeBase.time, message->GetId() );
#endif
            }
            else
            {
                auto blockMessage = static_cast<protocol::BlockMessage*>( message );
                
                protocol::Block & block = blockMessage->GetBlock();                

                if ( block.GetSize() <= messageChannelConfig.maxSmallBlockSize )
                {
                    const int index = numMessagesReceived % 32;
                    CORE_CHECK( block.GetSize() == index + 1 );
                    const uint8_t * data = block.GetData();
                    for ( int i = 0; i < block.GetSize(); ++i )
                        CORE_CHECK( data[i] == ( index + i ) % 256 );
#if !PROFILE
                    printf( "%09.2f - received message %d - small block\n", timeBase.time, message->GetId() );
#endif
                }
                else
                {
                    const int index = numMessagesReceived % 4;
                    CORE_CHECK( block.GetSize() == ( index + 1 ) * 1024 * 1000 + index );
                    const uint8_t * data = block.GetData();
                    for ( int i = 0; i < block.GetSize(); ++i )
                        CORE_CHECK( data[i] == ( index + i ) % 256 );
#if !PROFILE
                    printf( "%09.2f - received message %d - large block\n", timeBase.time, message->GetId() );
#endif
                }
            }

            numMessagesReceived++;

            messageFactory.Release( message );
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

        CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT ) == numMessagesSent );
        CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == numMessagesReceived );
        CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY ) == 0 );

        timeBase.time += timeBase.deltaTime;
    }
}

int main()
{
    srand( time( nullptr ) );

    core::memory::initialize();

    soak_test();

    core::memory::shutdown();

    return 0;
}
