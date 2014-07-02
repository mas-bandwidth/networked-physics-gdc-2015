/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#include "ReliableMessageChannel.h"
#include "Memory.h"

namespace protocol
{
    ReliableMessageChannelData::ReliableMessageChannelData( const ReliableMessageChannelConfig & _config ) 
        : config( _config ), numMessages(0), fragmentId(0), blockSize(0), blockId(0), largeBlock(0)
    {
//      printf( "create reliable message channel data: %p\n", this );
    }

    ReliableMessageChannelData::~ReliableMessageChannelData()
    {
        Allocator & a = memory::default_scratch_allocator();

        if ( fragment )
        {
            a.Free( fragment );
            fragment = nullptr;
        }

        if ( messages )
        {
            for ( int i = 0; i < numMessages; ++i )
            {
                assert( messages[i] );
                config.messageFactory->Release( messages[i] );
                messages[i] = nullptr;
            }

            a.Free( messages );
            messages = nullptr;
        }
    }

    template <typename Stream> void ReliableMessageChannelData::Serialize( Stream & stream )
    {
        serialize_bits( stream, largeBlock, 1 );

        if ( config.align )
            stream.Align();

        if ( largeBlock )
        {
            if ( Stream::IsWriting )
            {
                assert( fragment );
            }
            else
            {
                Allocator & a = memory::default_scratch_allocator();
                fragment = (uint8_t*) a.Allocate( config.blockFragmentSize );
                assert( fragment );
            }

            serialize_bits( stream, blockId, 16 );
            serialize_bits( stream, fragmentId, 16 );
            serialize_bits( stream, blockSize, 32 );
            serialize_bytes( stream, fragment, config.blockFragmentSize );
        }
        else
        {
            assert( config.messageFactory );

            if ( Stream::IsWriting )
                assert( numMessages > 0 );

            serialize_int( stream, numMessages, 1, config.maxMessagesPerPacket );

            assert( numMessages > 0 );

            if ( Stream::IsReading )
            {
                Allocator & a = memory::default_scratch_allocator();
                messages = (Message**) a.Allocate( numMessages * sizeof( Message* ) );
            }

            assert( messages );

            int messageTypes[numMessages];
            uint16_t messageIds[numMessages];

            if ( Stream::IsWriting )
            {
                for ( int i = 0; i < numMessages; ++i )
                {
                    assert( messages[i] );
                    messageTypes[i] = messages[i]->GetType();
                    messageIds[i] = messages[i]->GetId();
                }
            }

            if ( config.align )
                stream.Align();

            serialize_bits( stream, messageIds[0], 16 );

            for ( int i = 1; i < numMessages; ++i )
            {
                if ( Stream::IsWriting )
                {
                    uint32_t a = messageIds[i-1];
                    uint32_t b = messageIds[i] + ( messageIds[i-1] > messageIds[i] ? 65536 : 0 );
                    serialize_int_relative( stream, a, b );
                }
                else
                {
                    uint32_t a = messageIds[i-1];
                    uint32_t b;
                    serialize_int_relative( stream, a, b );
                    if ( b >= 65536 )
                        b -= 65536;
                    messageIds[i] = uint16_t( b );
                }
            }

            for ( int i = 0; i < numMessages; ++i )
            {
                if ( config.align )
                    stream.Align();

                serialize_int( stream, messageTypes[i], 0, config.messageFactory->GetMaxType() );

                if ( config.align )
                    stream.Align();

                if ( Stream::IsReading )
                {
                    messages[i] = config.messageFactory->Create( messageTypes[i] );

                    assert( messages[i] );
                    assert( messages[i]->GetType() == messageTypes[i] );

                    messages[i]->SetId( messageIds[i] );

                    if ( Stream::IsReading && messageTypes[i] == BlockMessageType )
                    {
                        assert( config.smallBlockAllocator );
                        BlockMessage * blockMessage = static_cast<BlockMessage*>( messages[i] );
                        blockMessage->SetAllocator( *config.smallBlockAllocator );
                    }
                }

                assert( messages[i] );

                serialize_object( stream, *messages[i] );
            }
        }
    }

    void ReliableMessageChannelData::SerializeRead( ReadStream & stream )
    {
        Serialize( stream );
    }

    void ReliableMessageChannelData::SerializeWrite( WriteStream & stream )
    {
        Serialize( stream );
    }

    void ReliableMessageChannelData::SerializeMeasure( MeasureStream & stream )
    {
        Serialize( stream );
    }

    // ----------------------------------------------------------------

    ReliableMessageChannel::ReliableMessageChannel( const ReliableMessageChannelConfig & config ) : m_config( config )
    {
        assert( config.messageFactory );
        assert( config.messageAllocator );
        assert( config.smallBlockAllocator );
        assert( config.largeBlockAllocator );
        assert( config.maxSmallBlockSize <= MaxSmallBlockSize );

        m_allocator = config.allocator ? config.allocator : &memory::default_allocator();

        m_sendQueue = PROTOCOL_NEW( *m_allocator, SlidingWindow<SendQueueEntry>, *m_allocator, m_config.sendQueueSize );
        m_sentPackets = PROTOCOL_NEW( *m_allocator, SlidingWindow<SentPacketEntry>, *m_allocator, m_config.sentPacketsSize );
        m_receiveQueue = PROTOCOL_NEW( *m_allocator, SlidingWindow<ReceiveQueueEntry>, *m_allocator, m_config.receiveQueueSize );

        const int MessageIdBits = 16;
        const int MessageTypeBits = bits_required( 0, m_config.messageFactory->GetMaxType() );
        const int MessageAlignOverhead = m_config.align ? 14 : 0;

        m_messageOverheadBits = MessageIdBits + MessageTypeBits + MessageAlignOverhead;

        m_maxBlockFragments = (int) ceil( m_config.maxLargeBlockSize / (float)m_config.blockFragmentSize );

        m_sendLargeBlock.fragments = PROTOCOL_NEW_ARRAY( *m_allocator, SendFragmentData, m_maxBlockFragments );
        m_receiveLargeBlock.fragments = PROTOCOL_NEW_ARRAY( *m_allocator, ReceiveFragmentData, m_maxBlockFragments );
        m_sentPacketMessageIds = PROTOCOL_NEW_ARRAY( *m_allocator, uint16_t, m_config.maxMessagesPerPacket * m_config.sendQueueSize );

        Reset();
    }

    ReliableMessageChannel::~ReliableMessageChannel()
    {
        Reset();

        assert( m_sendQueue );
        assert( m_sentPackets );
        assert( m_receiveQueue );

        PROTOCOL_DELETE( *m_allocator, SlidingWindow<SendQueueEntry>, m_sendQueue );
        PROTOCOL_DELETE( *m_allocator, SlidingWindow<SentPacketEntry>, m_sentPackets );
        PROTOCOL_DELETE( *m_allocator, SlidingWindow<ReceiveQueueEntry>, m_receiveQueue );

        assert( m_sentPacketMessageIds );
        assert( m_sendLargeBlock.fragments );
        assert( m_receiveLargeBlock.fragments );

        PROTOCOL_DELETE_ARRAY( *m_allocator, m_sentPacketMessageIds, m_config.maxMessagesPerPacket * m_config.sendQueueSize );
        PROTOCOL_DELETE_ARRAY( *m_allocator, m_sendLargeBlock.fragments, m_maxBlockFragments );
        PROTOCOL_DELETE_ARRAY( *m_allocator, m_receiveLargeBlock.fragments, m_maxBlockFragments );

        m_sendQueue = nullptr;
        m_sentPackets = nullptr;
        m_receiveQueue = nullptr;
        m_sentPacketMessageIds = nullptr;
        m_sendLargeBlock.fragments = nullptr;
        m_receiveLargeBlock.fragments = nullptr;
    }

    void ReliableMessageChannel::Reset()
    {
        assert( m_sendQueue );
        assert( m_sentPackets );
        assert( m_receiveQueue );
        assert( m_sentPacketMessageIds );

        m_error = 0;

        m_sendMessageId = 0;
        m_receiveMessageId = 0;
        m_oldestUnackedMessageId = 0;

        for ( int i = 0; i < m_sendQueue->GetSize(); ++i )
        {
            auto entry = m_sendQueue->GetAtIndex( i );
            if ( entry && entry->message )
                m_config.messageFactory->Release( entry->message );
        }

        for ( int i = 0; i < m_receiveQueue->GetSize(); ++i )
        {
            auto entry = m_receiveQueue->GetAtIndex( i );
            if ( entry && entry->message )
                m_config.messageFactory->Release( entry->message );
        }

        m_sendQueue->Reset();
        m_sentPackets->Reset();
        m_receiveQueue->Reset();

        memset( m_counters, 0, sizeof( m_counters ) );

        m_timeBase = TimeBase();

        m_sendLargeBlock.Reset();
        m_receiveLargeBlock.Reset();
    }

    bool ReliableMessageChannel::CanSendMessage() const
    {
        return m_sendQueue->HasSlotAvailable( m_sendMessageId );
    }

    void ReliableMessageChannel::SendMessage( Message * message )
    {
        assert( message );

//      printf( "queue message for send: %d\n", m_sendMessageId );

        assert( CanSendMessage() );

        if ( !CanSendMessage() )
        {
            printf( "reliable message send queue overflow\n" );
            m_error = RELIABLE_MESSAGE_CHANNEL_ERROR_SEND_QUEUE_FULL;
            m_config.messageFactory->Release( message );
            return;
        }

        message->SetId( m_sendMessageId );

        bool largeBlock = false;
        if ( message->IsBlock() )
        {
            BlockMessage & blockMessage = static_cast<BlockMessage&>( *message );
            Block & block = blockMessage.GetBlock();
            if ( block.GetSize() > m_config.maxSmallBlockSize )
                largeBlock = true;
        }

/*
        if ( largeBlock )
            printf( "sent large block %d\n", (int) m_sendMessageId );
*/

        bool result = m_sendQueue->Insert( SendQueueEntry( message, m_sendMessageId, largeBlock ) );
        assert( result );

        auto entry = m_sendQueue->Find( m_sendMessageId );

        assert( entry );
        assert( entry->valid );
        assert( entry->sequence == m_sendMessageId );
        assert( entry->message );
        assert( entry->message->GetId() == m_sendMessageId );

        if ( !largeBlock )
        {
            typedef MeasureStream Stream;
            const int SmallBlockOverhead = 8;
            MeasureStream measureStream( std::max( m_config.maxMessageSize, m_config.maxSmallBlockSize + SmallBlockOverhead ) );
            message->SerializeMeasure( measureStream );
            if ( measureStream.IsOverflow() )
            {
                printf( "measure stream overflow on message type %d: %d bits written, max is %d\n", message->GetType(), measureStream.GetBitsWritten(), measureStream.GetTotalBits() );
            }
            assert( !measureStream.IsOverflow() );
            entry->measuredBits = measureStream.GetBitsWritten() + m_messageOverheadBits;

//              printf( "message %d is %d bits\n", (int) m_sendMessageId, entity->measuredBits );
        }

        m_counters[RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT]++;

        m_sendMessageId++;
    }

    void ReliableMessageChannel::SendBlock( Block & block )
    {
//            printf( "send block: %d bytes\n", block->size() );

        auto blockMessage = (BlockMessage*) m_config.messageFactory->Create( BlockMessageType );
        assert( blockMessage );
        blockMessage->Connect( block );

        SendMessage( blockMessage );
    }        

    Message * ReliableMessageChannel::ReceiveMessage()
    {
        auto entry = m_receiveQueue->Find( m_receiveMessageId );
        if ( !entry )
            return nullptr;

        auto message = entry->message;

        #ifndef NDEBUG
        assert( message );
        assert( message->GetId() == m_receiveMessageId );
        #endif

//            printf( "dequeue for receive: %d\n", message->GetId() );

        entry->valid = 0;
        entry->message = nullptr;
        
        m_counters[RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED]++;

        m_receiveMessageId++;

        return message;
    }

    int ReliableMessageChannel::GetError() const 
    {
        return m_error;
    }

    ChannelData * ReliableMessageChannel::CreateData()
    {
        return PROTOCOL_NEW( memory::default_scratch_allocator(), ReliableMessageChannelData, m_config );
    }

    ChannelData * ReliableMessageChannel::GetData( uint16_t sequence )
    {
        SendQueueEntry * firstEntry = m_sendQueue->Find( m_oldestUnackedMessageId );
        if ( !firstEntry )
            return nullptr;

        if ( firstEntry->largeBlock )
        {
            /*
                Large block mode. Split large blocks into fragments 
                and send these fragments until they are all acked.
            */

            assert( firstEntry->message->GetType() == BlockMessageType );

            BlockMessage & blockMessage = static_cast<BlockMessage&>( *firstEntry->message );

            Block & block = blockMessage.GetBlock();

            assert( block.GetSize() > m_config.maxSmallBlockSize );

            if ( !m_sendLargeBlock.active )
            {
                m_sendLargeBlock.active = true;
                m_sendLargeBlock.blockId = m_oldestUnackedMessageId;
                m_sendLargeBlock.blockSize = block.GetSize();
                m_sendLargeBlock.numFragments = (int) ceil( block.GetSize() / (float)m_config.blockFragmentSize );
                m_sendLargeBlock.numAckedFragments = 0;

//                    printf( "sending block %d in %d fragments\n", (int) firstMessageId, m_sendLargeBlock.numFragments );

                assert( m_sendLargeBlock.numFragments >= 0 );
                assert( m_sendLargeBlock.numFragments <= m_maxBlockFragments );

                memset( &m_sendLargeBlock.fragments[0], 0, sizeof( SendFragmentData ) * m_sendLargeBlock.numFragments );
            }

            assert( m_sendLargeBlock.active );

            int fragmentId = -1;
            for ( int i = 0; i < m_sendLargeBlock.numFragments; ++i )
            {
                auto & fragment = m_sendLargeBlock.fragments[i];
                if ( !fragment.acked && fragment.timeLastSent + m_config.resendRate < m_timeBase.time )
                {
                    fragmentId = i;
                    fragment.timeLastSent = m_timeBase.time;
                    break;
                }
            }

            if ( fragmentId == -1 )
                return nullptr;

//                printf( "sending fragment %d\n", (int) fragmentId );

            auto data = PROTOCOL_NEW( memory::default_scratch_allocator(), ReliableMessageChannelData, m_config );
            data->largeBlock = 1;
            data->blockSize = block.GetSize();
            data->blockId = m_oldestUnackedMessageId;
            data->fragmentId = fragmentId;
            Allocator & a = memory::default_scratch_allocator();
            data->fragment = (uint8_t*) a.Allocate( m_config.blockFragmentSize );
            assert( data->fragment );
//                printf( "allocate fragment %p (send fragment)\n", data->fragment );

            //printf( "create fragment %p\n", data->fragment );

            int fragmentBytes = m_config.blockFragmentSize;
            int fragmentRemainder = block.GetSize() % m_config.blockFragmentSize;
            if ( fragmentRemainder && fragmentId == m_sendLargeBlock.numFragments - 1 )
                fragmentBytes = fragmentRemainder;

            assert( fragmentBytes >= 0 );
            assert( fragmentBytes <= m_config.blockFragmentSize );
            uint8_t * src = &( block.GetData()[fragmentId*m_config.blockFragmentSize] );
            uint8_t * dst = data->fragment;
            memcpy( dst, src, fragmentBytes );

            auto sentPacketData = m_sentPackets->InsertFast( sequence );
            assert( sentPacketData );
            sentPacketData->acked = 0;
            sentPacketData->largeBlock = 1;
            sentPacketData->blockId = m_oldestUnackedMessageId;
            sentPacketData->fragmentId = fragmentId;
            sentPacketData->timeSent = m_timeBase.time;
            sentPacketData->messageIds = nullptr;

            return data;
        }
        else
        {
            /*
                Message and small block mode.
                Iterate across send queue and include multiple messages 
                per-packet, but stop before the next large block.
            */

            assert( !m_sendLargeBlock.active );

            // gather messages to include in the packet

            int availableBits = m_config.packetBudget * 8;
            if ( m_config.align )
                availableBits -= 3 * 8;

            int numMessageIds = 0;
            uint16_t messageIds[m_config.maxMessagesPerPacket];
            for ( int i = 0; i < m_config.receiveQueueSize; ++i )
            {
                if ( availableBits < m_config.giveUpBits )
                    break;
                
                const uint16_t messageId = m_oldestUnackedMessageId + i;

                SendQueueEntry * entry = m_sendQueue->Find( messageId );
                
                if ( !entry )
                    break;

                if ( entry->largeBlock )
                    break;

                if ( entry->timeLastSent + m_config.resendRate <= m_timeBase.time && availableBits - entry->measuredBits >= 0 )
                {
                    messageIds[numMessageIds++] = messageId;
                    entry->timeLastSent = m_timeBase.time;
                    availableBits -= entry->measuredBits;
                }

                if ( numMessageIds == m_config.maxMessagesPerPacket )
                    break;
            }

            assert( numMessageIds >= 0 );
            assert( numMessageIds <= m_config.maxMessagesPerPacket );

            // if there are no messages then we don't have any data to send

            if ( numMessageIds == 0 )
                return nullptr;

            // add sent packet data containing message ids included in this packet

            auto sentPacketData = m_sentPackets->InsertFast( sequence );
            assert( sentPacketData );
            sentPacketData->largeBlock = 0;
            sentPacketData->acked = 0;
            sentPacketData->timeSent = m_timeBase.time;
            const int sentPacketIndex = m_sentPackets->GetIndex( sequence );
            sentPacketData->messageIds = &m_sentPacketMessageIds[sentPacketIndex*m_config.maxMessagesPerPacket];
            sentPacketData->numMessageIds = numMessageIds;
            for ( int i = 0; i < numMessageIds; ++i )
                sentPacketData->messageIds[i] = messageIds[i];

            // update counter: num messages written

            m_counters[RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_WRITTEN] += numMessageIds;

            // construct channel data for packet

            auto data = PROTOCOL_NEW( memory::default_scratch_allocator(), ReliableMessageChannelData, m_config );

            Allocator & a = memory::default_scratch_allocator();
            data->messages = (Message**) a.Allocate( numMessageIds * sizeof( Message* ) );
            assert( data->messages );
//                printf( "allocate messages %p (get data)\n", data->messages );
            data->numMessages = numMessageIds;
            for ( int i = 0; i < numMessageIds; ++i )
            {
                auto entry = m_sendQueue->Find( messageIds[i] );
                assert( entry );
                assert( entry->message );
                data->messages[i] = entry->message;
                m_config.messageFactory->AddRef( entry->message );
            }

//                printf( "sent %d messages in packet\n", data->messages.size() );

            return data;
        }
    }

    bool ReliableMessageChannel::ProcessData( uint16_t sequence, ChannelData * channelData )
    {
        assert( channelData );

//          printf( "process data %d\n", sequence );

        auto data = static_cast<ReliableMessageChannelData*>( channelData );

//            printf( "process message channel data: %d\n", sequence );

        /*
            IMPORTANT: If this is a large block but the message id is *older*
            than the message id we are expecting, the sender has missed some acks
            for packets we have actually received, due to extreme packet loss.
            As a result the sender hasn't realized that we have fully received 
            the large block, and they are still trying to send it to us.

            To resolve this situation, do nothing! The ack system will inform 
            the sender that these fragments sent to us have been received, and 
            once all fragments are acked the sender moves on to the next block
            or message.
        */

        if ( data->largeBlock && sequence_less_than( data->blockId, m_receiveQueue->GetSequence() ) )
            return true;

        /*
            If we are currently receiving a large block, discard any packets
            that contain bitpacked messages or small blocks. It is necessary
            to discard the packet so the sender doesn't think these messages
            are acked, when in fact we were not able to process them 
            (otherwise the sender may never resend them, and stall out...)
        */

        if ( !data->largeBlock && m_receiveLargeBlock.active )
        {
//                printf( "received unexpected bitpacked message or small block while receiving large block\n" );
            return false;
        }

        // process the packet data according to its contents

        if ( data->largeBlock )
        {
            /*
                Large block mode.
                This packet includes a fragment for the large block currently 
                being received. Only one large block is sent at a time.
            */

            if ( !m_receiveLargeBlock.active )
            {
                const uint16_t expectedBlockId = m_receiveQueue->GetSequence();

                if ( data->blockId != expectedBlockId )
                {
//                        printf( "unexpected large block id\n" );
                    return false;
                }

                const int numFragments = (int) ceil( data->blockSize / (float)m_config.blockFragmentSize );

                assert( numFragments >= 0 );
                assert( numFragments <= m_maxBlockFragments );

                if ( numFragments < 0 || numFragments > m_maxBlockFragments )
                {
                    //printf( "large block num fragments outside of range\n" );
                    return false;
                }

//                    printf( "receiving large block %d (%d bytes)\n", data->blockId, data->blockSize );

                m_receiveLargeBlock.active = true;
                m_receiveLargeBlock.numFragments = numFragments;
                m_receiveLargeBlock.numReceivedFragments = 0;
                m_receiveLargeBlock.blockId = data->blockId;
                m_receiveLargeBlock.blockSize = data->blockSize;

                assert( m_config.largeBlockAllocator );
                uint8_t * blockData = (uint8_t*) m_config.largeBlockAllocator->Allocate( data->blockSize );
                m_receiveLargeBlock.block.Connect( *m_config.largeBlockAllocator, blockData, data->blockSize );
                
                memset( &m_receiveLargeBlock.fragments[0], 0, sizeof( ReceiveFragmentData ) * numFragments );
            }

            assert( m_receiveLargeBlock.active );

            if ( data->blockId != m_receiveLargeBlock.blockId )
            {
//                    printf( "unexpected large block id. got %d but was expecting %d\n", data->blockId, m_receiveLargeBlock.blockId );
                return false;
            }

            assert( data->blockId == m_receiveLargeBlock.blockId );
            assert( data->blockSize == m_receiveLargeBlock.blockSize );
            assert( data->fragmentId < m_receiveLargeBlock.numFragments );

            if ( data->blockId != m_receiveLargeBlock.blockId )
            {
//                    printf( "recieve large block id mismatch. got %d but was expecting %d\n", data->blockId, m_receiveLargeBlock.blockId );
                return false;
            }

            if ( data->blockSize != m_receiveLargeBlock.blockSize )
            {
//                    printf( "large block size mismatch. got %d but was expecting %d\n", data->blockSize, m_receiveLargeBlock.blockSize );
                return false;
            }

            if ( data->fragmentId >= m_receiveLargeBlock.numFragments )
            {
//                    printf( "large block fragment out of bounds.\n" );
                return false;
            }

            auto & fragment = m_receiveLargeBlock.fragments[data->fragmentId];

            if ( !fragment.received )
            {
/*
                printf( "received fragment " << data->fragmentId << " of large block " << m_receiveLargeBlock.blockId
                     << " (" << m_receiveLargeBlock.numReceivedFragments+1 << "/" << m_receiveLargeBlock.numFragments << ")" << endl;
                     */

                fragment.received = 1;

                Block & block = m_receiveLargeBlock.block;

                int fragmentBytes = m_config.blockFragmentSize;
                int fragmentRemainder = block.GetSize() % m_config.blockFragmentSize;
                if ( fragmentRemainder && data->fragmentId == m_receiveLargeBlock.numFragments - 1 )
                    fragmentBytes = fragmentRemainder;

//                    printf( "fragment bytes = %d\n", fragmentBytes );

                assert( fragmentBytes >= 0 );
                assert( fragmentBytes <= m_config.blockFragmentSize );
                uint8_t * src = data->fragment;
                uint8_t * dst = &( block.GetData()[data->fragmentId*m_config.blockFragmentSize] );
                memcpy( dst, src, fragmentBytes );

                m_receiveLargeBlock.numReceivedFragments++;

                if ( m_receiveLargeBlock.numReceivedFragments == m_receiveLargeBlock.numFragments )
                {
//                        printf( "received large block %d (%d bytes)\n", m_receiveLargeBlock.blockId, m_receiveLargeBlock.block->size() );

                    auto blockMessage = (BlockMessage*) m_config.messageFactory->Create( BlockMessageType );
                    assert( blockMessage );
                    blockMessage->Connect( m_receiveLargeBlock.block );
                    blockMessage->SetId( m_receiveLargeBlock.blockId );

                    m_receiveQueue->Insert( ReceiveQueueEntry( blockMessage, m_receiveLargeBlock.blockId ) );

                    m_receiveLargeBlock.active = false;

                    assert( !m_receiveLargeBlock.block.IsValid() );
                }
            }
        }
        else
        {
            /*
                Bit-packed message and small block mode.
                Multiple messages and small blocks are included
                in each packet.  Each needs to be be processed
                and inserted into a sliding window, so they can 
                be dequeued reliably and in-order.
            */

            bool earlyMessage = false;

            const uint16_t minMessageId = m_receiveMessageId;
            const uint16_t maxMessageId = m_receiveMessageId + m_config.receiveQueueSize - 1;

            // process messages included in this packet data

            assert( data->messages );

            for ( int i = 0; i < data->numMessages; ++i )
            {
                auto message = data->messages[i];

                assert( message );

                const uint16_t messageId = message->GetId();

                if ( sequence_less_than( messageId, minMessageId ) )
                {
                    m_counters[RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_LATE]++;
                }
                else if ( sequence_greater_than( messageId, maxMessageId ) )
                {
                    m_counters[RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY]++;
                    earlyMessage = true;
                }
                else if ( !m_receiveQueue->Find( messageId ) )
                {
                    bool result = m_receiveQueue->Insert( ReceiveQueueEntry( message, messageId ) );
                    assert( result );
                    m_config.messageFactory->AddRef( message );
                }
                
                m_counters[RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_READ]++;
            }

            if ( earlyMessage )
                return false;
        }

        return true;
    }

    void ReliableMessageChannel::UpdateOldestUnackedMessageId()
    {
        const uint16_t stopMessageId = m_sendQueue->GetSequence();

        while ( true )
        {
            if ( m_oldestUnackedMessageId == stopMessageId )
                break;

            SendQueueEntry * entry = m_sendQueue->Find( m_oldestUnackedMessageId );
            if ( entry )
                break;
           
            ++m_oldestUnackedMessageId;
        }

        assert( !sequence_greater_than( m_oldestUnackedMessageId, stopMessageId ) );
    }

    void ReliableMessageChannel::ProcessAck( uint16_t ack )
    {
//            printf( "process ack: %d\n", (int) ack );

        auto sentPacket = m_sentPackets->Find( ack );
        if ( !sentPacket || sentPacket->acked )
            return;

        if ( !sentPacket->largeBlock )
        {
            for ( int i = 0; i < sentPacket->numMessageIds; ++i )
            {
                const uint16_t messageId = sentPacket->messageIds[i];

                auto sendQueueEntry = m_sendQueue->Find( messageId );
                
                if ( sendQueueEntry )
                {
                    assert( sendQueueEntry->message );
                    assert( sendQueueEntry->message->GetId() == messageId );

//                        printf( "acked message %d\n", messageId );

                    m_config.messageFactory->Release( sendQueueEntry->message );
                    sendQueueEntry->message = nullptr;
                    sendQueueEntry->valid = 0;
                }
            }

            UpdateOldestUnackedMessageId();
        }
        else if ( m_sendLargeBlock.active && m_sendLargeBlock.blockId == sentPacket->blockId )
        {
            assert( sentPacket->fragmentId < m_sendLargeBlock.numFragments );

            auto & fragment = m_sendLargeBlock.fragments[sentPacket->fragmentId];

            if ( !fragment.acked )
            {
                /*
                printf( "acked fragment " << sentPacket->fragmentId << " of large block " << m_sendLargeBlock.blockId 
                     << " (" << m_sendLargeBlock.numAckedFragments+1 << "/" << m_sendLargeBlock.numFragments << ")" << endl;
                     */

                fragment.acked = true;
                
                m_sendLargeBlock.numAckedFragments++;

                if ( m_sendLargeBlock.numAckedFragments == m_sendLargeBlock.numFragments )
                {
//                        printf( "acked large block %d\n", (int) m_sendLargeBlock.blockId );

                    m_sendLargeBlock.active = false;

                    auto sendQueueEntry = m_sendQueue->Find( sentPacket->blockId );
                    assert( sendQueueEntry );

                    m_config.messageFactory->Release( sendQueueEntry->message );                    
                    sendQueueEntry->message = nullptr;
                    sendQueueEntry->valid = 0;

                    UpdateOldestUnackedMessageId();
                }
            }
        }
        
        sentPacket->acked = 1;
    }

    void ReliableMessageChannel::Update( const TimeBase & timeBase )
    {
        m_timeBase = timeBase;
    }

    uint64_t ReliableMessageChannel::GetCounter( int index ) const
    {
        assert( index >= 0 );
        assert( index < RELIABLE_MESSAGE_CHANNEL_COUNTER_NUM_COUNTERS );
        return m_counters[index];
    }

    ReliableMessageChannel::SendLargeBlockStatus ReliableMessageChannel::GetSendLargeBlockStatus() const
    {
        SendLargeBlockStatus status;
        status.sending = m_sendLargeBlock.active;
        status.blockId = m_sendLargeBlock.blockId;
        status.blockSize = m_sendLargeBlock.blockSize;
        status.numFragments = m_sendLargeBlock.numFragments;
        status.numAckedFragments = m_sendLargeBlock.numAckedFragments;
        return status;
    }

    ReliableMessageChannel::ReceiveLargeBlockStatus ReliableMessageChannel::GetReceiveLargeBlockStatus() const
    {
        ReceiveLargeBlockStatus status;
        status.receiving = m_receiveLargeBlock.active;
        status.blockId = m_receiveLargeBlock.blockId;
        status.blockSize = m_receiveLargeBlock.blockSize;
        status.numFragments = m_receiveLargeBlock.numFragments;
        status.numReceivedFragments = m_receiveLargeBlock.numReceivedFragments;
        return status;
    }
}
