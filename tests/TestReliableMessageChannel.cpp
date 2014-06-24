#include "Connection.h"
#include "TestMessages.h"
#include "TestPackets.h"
#include "NetworkSimulator.h"
#include "ReliableMessageChannel.h"
#include "TestChannelStructure.h"

using namespace protocol;

void test_reliable_message_channel_messages()
{
    printf( "test_reliable_message_channel_messages\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        TestPacketFactory packetFactory( &channelStructure );

        const int MaxPacketSize = 256;

        ConnectionConfig connectionConfig;
        connectionConfig.packetType = PACKET_CONNECTION;
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
            auto readPacket = new ConnectionPacket( PACKET_CONNECTION, &channelStructure );
            readPacket->SerializeRead( readStream );

            simulator.SendPacket( address, readPacket );

            simulator.Update( timeBase );

            auto packet = simulator.ReceivePacket();

            if ( packet )
            {
                connection.ReadPacket( static_cast<ConnectionPacket*>( packet ) );
                delete packet;
            }

            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_READ ) <= iteration + 1 );
            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_WRITTEN ) == iteration + 1 );
            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_ACKED ) <= iteration + 1 );

            while ( true )
            {
                auto message = messageChannel->ReceiveMessage();

                if ( !message )
                    break;

                assert( message->GetId() == numMessagesReceived );
                assert( message->GetType() == MESSAGE_TEST );

                auto testMessage = static_cast<TestMessage*>( message );

                assert( testMessage->sequence == numMessagesReceived );

                ++numMessagesReceived;

                delete message;
            }

            if ( numMessagesReceived == NumMessagesSent )
                break;

            connection.Update( timeBase );

            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT ) == NumMessagesSent );
            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == numMessagesReceived );
            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY ) == 0 );

            timeBase.time += timeBase.deltaTime;

            if ( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == NumMessagesSent )
                break;

            iteration++;
        }
    }
    memory::shutdown();
}

void test_reliable_message_channel_small_blocks()
{
    printf( "test_reliable_message_channel_small_blocks\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        TestPacketFactory packetFactory( &channelStructure );
        
        TestMessageFactory messageFactory;

        const int MaxPacketSize = 256;

        ConnectionConfig connectionConfig;
        connectionConfig.packetType = PACKET_CONNECTION;
        connectionConfig.maxPacketSize = MaxPacketSize;
        connectionConfig.packetFactory = &packetFactory;
        connectionConfig.channelStructure = &channelStructure;

        Connection connection( connectionConfig );

        auto messageChannel = static_cast<ReliableMessageChannel*>( connection.GetChannel( 0 ) );

        auto channelConfig = channelStructure.GetConfig(); 

        const int NumMessagesSent = channelConfig.maxSmallBlockSize;

        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            Block block( memory::default_allocator(), i + 1 );
            uint8_t * data = block.GetData();
            for ( int j = 0; j < block.GetSize(); ++j )
                data[j] = ( i + j ) % 256;
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

            delete writePacket;

            ReadStream readStream( buffer, MaxPacketSize );
            auto readPacket = new ConnectionPacket( PACKET_CONNECTION, &channelStructure );
            readPacket->SerializeRead( readStream );

            simulator.SendPacket( address, readPacket );

            simulator.Update( timeBase );

            auto packet = simulator.ReceivePacket();

            if ( packet )
            {
                connection.ReadPacket( static_cast<ConnectionPacket*>( packet ) );
                delete packet;
            }

            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_READ ) <= iteration + 1 );
            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_WRITTEN ) == iteration + 1 );
            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_ACKED ) <= iteration + 1 );

            while ( true )
            {
                auto message = messageChannel->ReceiveMessage();

                if ( !message )
                    break;

                assert( message->GetId() == numMessagesReceived );
                assert( message->GetType() == MESSAGE_BLOCK );

                auto blockMessage = static_cast<BlockMessage*>( message );

                Block & block = blockMessage->GetBlock();

                assert( block.GetSize() == numMessagesReceived + 1 );
                const uint8_t * data = block.GetData();
                for ( int i = 0; i < block.GetSize(); ++i )
                    assert( data[i] == ( numMessagesReceived + i ) % 256 );

                ++numMessagesReceived;

                delete message;
            }

            if ( numMessagesReceived == NumMessagesSent )
                break;

            connection.Update( timeBase );

            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT ) == NumMessagesSent );
            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == numMessagesReceived );
            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY ) == 0 );

            timeBase.time += timeBase.deltaTime;

            if ( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == NumMessagesSent )
                break;

            iteration++;
        }
    }
    memory::shutdown();
}

void test_reliable_message_channel_large_blocks()
{
    printf( "test_reliable_message_channel_large_blocks\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        TestPacketFactory packetFactory( &channelStructure );

        const int MaxPacketSize = 256;

        ConnectionConfig connectionConfig;
        connectionConfig.packetType = PACKET_CONNECTION;
        connectionConfig.maxPacketSize = MaxPacketSize;
        connectionConfig.packetFactory = &packetFactory;
        connectionConfig.channelStructure = &channelStructure;

        Connection connection( connectionConfig );

        auto messageChannel = static_cast<ReliableMessageChannel*>( connection.GetChannel( 0 ) );

        const int NumMessagesSent = 16;

        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            Block block( memory::default_allocator(), ( i + 1 ) * 1024 + i );
            uint8_t * data = block.GetData();
            for ( int j = 0; j < block.GetSize(); ++j )
                data[j] = ( i + j ) % 256;
            messageChannel->SendBlock( block );
        }

        TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        uint64_t numMessagesReceived = 0;

        int iteration = 0;

        Address address( "::1" );

        NetworkSimulator simulator;
        simulator.AddState( { 1.0f, 1.0f, 50 } );

        while ( true )
        {  
            auto writePacket = connection.WritePacket();

            uint8_t buffer[MaxPacketSize];

            WriteStream writeStream( buffer, MaxPacketSize );
            writePacket->SerializeWrite( writeStream );
            writeStream.Flush();

            delete writePacket;

            ReadStream readStream( buffer, MaxPacketSize );
            auto readPacket = new ConnectionPacket( PACKET_CONNECTION, &channelStructure );
            readPacket->SerializeRead( readStream );

            simulator.SendPacket( address, readPacket );

            simulator.Update( timeBase );

            auto packet = simulator.ReceivePacket();

            if ( packet )
            {
                connection.ReadPacket( static_cast<ConnectionPacket*>( packet ) );

                delete packet;
            }
        
            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_READ ) <= iteration + 1 );
            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_WRITTEN ) == iteration + 1 );
            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_ACKED ) <= iteration + 1 );

            while ( true )
            {
                auto message = messageChannel->ReceiveMessage();

                if ( !message )
                    break;

                assert( message->GetId() == numMessagesReceived );
                assert( message->GetType() == MESSAGE_BLOCK );

                auto blockMessage = static_cast<BlockMessage*>( message );

                Block & block = blockMessage->GetBlock();

//                printf( "received block %d (%d bytes)\n", blockMessage->GetId(), (int) block->size() );

                assert( block.GetSize() == ( numMessagesReceived + 1 ) * 1024 + numMessagesReceived );
                const uint8_t * data = block.GetData();
                for ( int i = 0; i < block.GetSize(); ++i )
                    assert( data[i] == ( numMessagesReceived + i ) % 256 );

                ++numMessagesReceived;

                delete message;
            }

            if ( numMessagesReceived == NumMessagesSent )
                break;

            connection.Update( timeBase );

            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT ) == NumMessagesSent );
            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == numMessagesReceived );
            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY ) == 0 );

            timeBase.time += timeBase.deltaTime;

            iteration++;
        }

        assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == NumMessagesSent );
    }
    memory::shutdown();
}

void test_reliable_message_channel_mixture()
{
    printf( "test_reliable_message_channel_mixture\n" );

    memory::initialize();
    {
        TestChannelStructure channelStructure;

        TestPacketFactory packetFactory( &channelStructure );

        const int MaxPacketSize = 256;

        ConnectionConfig connectionConfig;
        connectionConfig.packetType = PACKET_CONNECTION;
        connectionConfig.maxPacketSize = MaxPacketSize;
        connectionConfig.packetFactory = &packetFactory;
        connectionConfig.channelStructure = &channelStructure;

        Connection connection( connectionConfig );

        auto messageChannel = static_cast<ReliableMessageChannel*>( connection.GetChannel( 0 ) );

        const int NumMessagesSent = 32;

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
                Block block( memory::default_allocator(), ( i + 1 ) * 8 + i );
                uint8_t * data = block.GetData();
                for ( int j = 0; j < block.GetSize(); ++j )
                    data[j] = ( i + j ) % 256;
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

            delete writePacket;

            ReadStream readStream( buffer, MaxPacketSize );
            auto readPacket = new ConnectionPacket( PACKET_CONNECTION, &channelStructure );
            readPacket->SerializeRead( readStream );

            simulator.SendPacket( address, readPacket );

            simulator.Update( timeBase );

            auto packet = simulator.ReceivePacket();

            if ( packet )
            {
                connection.ReadPacket( static_cast<ConnectionPacket*>( packet ) );
                delete packet;
            }
            
            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_READ ) <= iteration + 1 );
            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_WRITTEN ) == iteration + 1 );
            assert( connection.GetCounter( CONNECTION_COUNTER_PACKETS_ACKED ) <= iteration + 1 );

            while ( true )
            {
                auto message = messageChannel->ReceiveMessage();

                if ( !message )
                    break;

                assert( message->GetId() == numMessagesReceived );

                if ( message->GetType() == MESSAGE_BLOCK )
                {
                    assert( message->GetType() == MESSAGE_BLOCK );

                    auto blockMessage = static_cast<BlockMessage*>( message );

                    Block & block = blockMessage->GetBlock();

    //                printf( "received block %d (%d bytes)\n", blockMessage->GetId(), (int) block->size() );

                    assert( block.GetSize() == ( numMessagesReceived + 1 ) * 8 + numMessagesReceived );
                    const uint8_t * data = block.GetData();
                    for ( int i = 0; i < block.GetSize(); ++i )
                        assert( data[i] == ( numMessagesReceived + i ) % 256 );
                }
                else
                {
                    assert( message->GetType() == MESSAGE_TEST );

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

            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT ) == NumMessagesSent );
            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == numMessagesReceived );
            assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY ) == 0 );

            timeBase.time += timeBase.deltaTime;

            iteration++;
        }

        assert( messageChannel->GetCounter( RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == NumMessagesSent );
    }
    memory::shutdown();
}

