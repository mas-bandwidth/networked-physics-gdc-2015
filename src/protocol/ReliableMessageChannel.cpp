// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#include "protocol/ReliableMessageChannel.h"
#include "protocol/BitArray.h"
#include "core/Memory.h"

namespace protocol
{
    ReliableMessageChannelData::ReliableMessageChannelData( const ReliableMessageChannelConfig & _config ) 
        : config( _config ), numMessages(0), fragmentId(0), blockSize(0), blockId(0), largeBlock(0)
    {
//      printf( "create reliable message channel data: %p\n", this );
    }

    ReliableMessageChannelData::~ReliableMessageChannelData()
    {
        core::Allocator & a = core::memory::scratch_allocator();

        if ( fragment )
        {
            a.Free( fragment );
            fragment = nullptr;
        }

        if ( messages )
        {
            for ( int i = 0; i < numMessages; ++i )
            {
                CORE_ASSERT( messages[i] );
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
                CORE_ASSERT( fragment );
            }
            else
            {
                core::Allocator & a = core::memory::scratch_allocator();
                fragment = (uint8_t*) a.Allocate( config.blockFragmentSize );
                CORE_ASSERT( fragment );
            }

            serialize_bits( stream, blockId, 16 );
            serialize_bits( stream, fragmentId, 16 );
            serialize_bits( stream, blockSize, 32 );
            serialize_bytes( stream, fragment, config.blockFragmentSize );
        }
        else
        {
            CORE_ASSERT( config.messageFactory );

            if ( Stream::IsWriting )
                CORE_ASSERT( numMessages > 0 );

            serialize_int( stream, numMessages, 1, config.maxMessagesPerPacket );

            CORE_ASSERT( numMessages > 0 );

            if ( Stream::IsReading )
            {
                core::Allocator & a = core::memory::scratch_allocator();
                messages = (Message**) a.Allocate( numMessages * sizeof( Message* ) );
            }

            CORE_ASSERT( messages );

            int messageTypes[numMessages];
            uint16_t messageIds[numMessages];

            if ( Stream::IsWriting )
            {
                for ( int i = 0; i < numMessages; ++i )
                {
                    CORE_ASSERT( messages[i] );
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

                const int maxMessageType = config.messageFactory->GetNumTypes() - 1;

                serialize_int( stream, messageTypes[i], 0, maxMessageType );

                if ( config.align )
                    stream.Align();

                if ( Stream::IsReading )
                {
                    messages[i] = config.messageFactory->Create( messageTypes[i] );

                    CORE_ASSERT( messages[i] );
                    CORE_ASSERT( messages[i]->GetType() == messageTypes[i] );

                    messages[i]->SetId( messageIds[i] );

                    if ( Stream::IsReading && messageTypes[i] == BlockMessageType )
                    {
                        CORE_ASSERT( config.smallBlockAllocator );
                        BlockMessage * blockMessage = static_cast<BlockMessage*>( messages[i] );
                        blockMessage->SetAllocator( *config.smallBlockAllocator );
                    }
                }

                CORE_ASSERT( messages[i] );

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
        CORE_ASSERT( config.messageFactory );
        CORE_ASSERT( config.messageAllocator );
        CORE_ASSERT( config.smallBlockAllocator );
        CORE_ASSERT( config.largeBlockAllocator );
        CORE_ASSERT( config.maxSmallBlockSize <= MaxSmallBlockSize );

        m_allocator = config.allocator ? config.allocator : &core::memory::default_allocator();

        m_sendQueue = CORE_NEW( *m_allocator, SequenceBuffer<SendQueueEntry>, *m_allocator, m_config.sendQueueSize );
        m_sentPackets = CORE_NEW( *m_allocator, SequenceBuffer<SentPacketEntry>, *m_allocator, m_config.sentPacketsSize );
        m_receiveQueue = CORE_NEW( *m_allocator, SequenceBuffer<ReceiveQueueEntry>, *m_allocator, m_config.receiveQueueSize );

        const int maxMessageType = m_config.messageFactory->GetNumTypes() - 1;

        const int MessageIdBits = 16;
        const int MessageTypeBits = core::bits_required( 0, maxMessageType );
        const int MessageAlignOverhead = m_config.align ? 14 : 0;

        m_messageOverheadBits = MessageIdBits + MessageTypeBits + MessageAlignOverhead;

        m_maxBlockFragments = (int) ceil( m_config.maxLargeBlockSize / (float)m_config.blockFragmentSize );

        m_sendLargeBlock.time_fragment_last_sent = CORE_NEW_ARRAY( *m_allocator, double, m_maxBlockFragments );
        m_sendLargeBlock.acked_fragment = CORE_NEW( *m_allocator, BitArray, *m_allocator, m_maxBlockFragments );
        m_receiveLargeBlock.received_fragment = CORE_NEW( *m_allocator, BitArray, *m_allocator, m_maxBlockFragments );
        m_sentPacketMessageIds = CORE_NEW_ARRAY( *m_allocator, uint16_t, m_config.maxMessagesPerPacket * m_config.sendQueueSize );

        Reset();
    }

    ReliableMessageChannel::~ReliableMessageChannel()
    {
        Reset();

        CORE_ASSERT( m_sendQueue );
        CORE_ASSERT( m_sentPackets );
        CORE_ASSERT( m_receiveQueue );

        CORE_DELETE( *m_allocator, SequenceBuffer<SendQueueEntry>, m_sendQueue );
        CORE_DELETE( *m_allocator, SequenceBuffer<SentPacketEntry>, m_sentPackets );
        CORE_DELETE( *m_allocator, SequenceBuffer<ReceiveQueueEntry>, m_receiveQueue );

        CORE_ASSERT( m_sentPacketMessageIds );
        CORE_ASSERT( m_sendLargeBlock.time_fragment_last_sent );
        CORE_ASSERT( m_sendLargeBlock.acked_fragment );
        CORE_ASSERT( m_receiveLargeBlock.received_fragment );

        CORE_DELETE_ARRAY( *m_allocator, m_sentPacketMessageIds, m_config.maxMessagesPerPacket * m_config.sendQueueSize );
        CORE_DELETE_ARRAY( *m_allocator, m_sendLargeBlock.time_fragment_last_sent, m_maxBlockFragments );
        CORE_DELETE( *m_allocator, BitArray, m_sendLargeBlock.acked_fragment );
        CORE_DELETE( *m_allocator, BitArray, m_receiveLargeBlock.received_fragment );

        m_sendQueue = nullptr;
        m_sentPackets = nullptr;
        m_receiveQueue = nullptr;
        m_sentPacketMessageIds = nullptr;
        m_sendLargeBlock.time_fragment_last_sent = nullptr;
        m_sendLargeBlock.acked_fragment = nullptr;
        m_receiveLargeBlock.received_fragment = nullptr;
    }

    void ReliableMessageChannel::Reset()
    {
        CORE_ASSERT( m_sendQueue );
        CORE_ASSERT( m_sentPackets );
        CORE_ASSERT( m_receiveQueue );
        CORE_ASSERT( m_sentPacketMessageIds );

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

        m_timeBase = core::TimeBase();

        m_sendLargeBlock.Reset();
        m_receiveLargeBlock.Reset();
    }

    bool ReliableMessageChannel::CanSendMessage() const
    {
        return m_sendQueue->IsAvailable( m_sendMessageId );
    }

    void ReliableMessageChannel::SendMessage( Message * message )
    {
        CORE_ASSERT( message );

//      printf( "queue message for send: %d\n", m_sendMessageId );

        CORE_ASSERT( CanSendMessage() );

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

        auto entry = m_sendQueue->Insert( m_sendMessageId );
        CORE_ASSERT( entry );
        entry->message = message;
        entry->largeBlock = largeBlock;
        entry->measuredBits = 0;
        entry->timeLastSent = -1.0;

        if ( !largeBlock )
        {
            typedef MeasureStream Stream;

            const int SmallBlockOverhead = 8;
            
            MeasureStream measureStream( core::max( m_config.maxMessageSize, m_config.maxSmallBlockSize + SmallBlockOverhead ) );
            measureStream.SetContext( GetContext() );
            message->SerializeMeasure( measureStream );
            if ( measureStream.IsOverflow() )
            {
                printf( "measure stream overflow on message type %d: %d bits written, max is %d\n", message->GetType(), measureStream.GetBitsProcessed(), measureStream.GetTotalBits() );
            }
            
            CORE_ASSERT( !measureStream.IsOverflow() );
            
            entry->measuredBits = measureStream.GetBitsProcessed() + m_messageOverheadBits;

//              printf( "message %d is %d bits\n", (int) m_sendMessageId, entity->measuredBits );
        }

        m_counters[RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT]++;

        m_sendMessageId++;
    }

    void ReliableMessageChannel::SendBlock( Block & block )
    {
//            printf( "send block: %d bytes\n", block->size() );

        auto blockMessage = (BlockMessage*) m_config.messageFactory->Create( BlockMessageType );
        CORE_ASSERT( blockMessage );
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
        CORE_ASSERT( message );
        CORE_ASSERT( message->GetId() == m_receiveMessageId );
        #endif

//            printf( "dequeue for receive: %d\n", message->GetId() );

        m_receiveQueue->Remove( m_receiveMessageId );

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
        return CORE_NEW( core::memory::scratch_allocator(), ReliableMessageChannelData, m_config );
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

            CORE_ASSERT( firstEntry->message->GetType() == BlockMessageType );

            BlockMessage & blockMessage = static_cast<BlockMessage&>( *firstEntry->message );

            Block & block = blockMessage.GetBlock();

            CORE_ASSERT( block.GetSize() > m_config.maxSmallBlockSize );

            if ( !m_sendLargeBlock.active )
            {
                m_sendLargeBlock.active = true;
                m_sendLargeBlock.blockId = m_oldestUnackedMessageId;
                m_sendLargeBlock.blockSize = block.GetSize();
                m_sendLargeBlock.numFragments = (int) ceil( block.GetSize() / (float)m_config.blockFragmentSize );
                m_sendLargeBlock.numAckedFragments = 0;

//                    printf( "sending block %d in %d fragments\n", (int) firstMessageId, m_sendLargeBlock.numFragments );

                CORE_ASSERT( m_sendLargeBlock.numFragments >= 0 );
                CORE_ASSERT( m_sendLargeBlock.numFragments <= m_maxBlockFragments );

                m_sendLargeBlock.acked_fragment->Clear();

                for ( int i = 0; i < m_maxBlockFragments; ++i )
                    m_sendLargeBlock.time_fragment_last_sent[i] = -1.0;
            }

            CORE_ASSERT( m_sendLargeBlock.active );

            int fragmentId = -1;
            for ( int i = 0; i < m_sendLargeBlock.numFragments; ++i )
            {
                if ( !m_sendLargeBlock.acked_fragment->GetBit(i) && 
                      m_sendLargeBlock.time_fragment_last_sent[i] + m_config.resendRate < m_timeBase.time )
                {
                    fragmentId = i;
                    m_sendLargeBlock.time_fragment_last_sent[i] = m_timeBase.time;
                    break;
                }
            }

            if ( fragmentId == -1 )
                return nullptr;

//                printf( "sending fragment %d\n", (int) fragmentId );

            auto data = CORE_NEW( core::memory::scratch_allocator(), ReliableMessageChannelData, m_config );
            data->largeBlock = 1;
            data->blockSize = block.GetSize();
            data->blockId = m_oldestUnackedMessageId;
            data->fragmentId = fragmentId;
            core::Allocator & a = core::memory::scratch_allocator();
            data->fragment = (uint8_t*) a.Allocate( m_config.blockFragmentSize );
            CORE_ASSERT( data->fragment );
//                printf( "allocate fragment %p (send fragment)\n", data->fragment );

            //printf( "create fragment %p\n", data->fragment );

            int fragmentBytes = m_config.blockFragmentSize;
            int fragmentRemainder = block.GetSize() % m_config.blockFragmentSize;
            if ( fragmentRemainder && fragmentId == m_sendLargeBlock.numFragments - 1 )
                fragmentBytes = fragmentRemainder;

            CORE_ASSERT( fragmentBytes >= 0 );
            CORE_ASSERT( fragmentBytes <= m_config.blockFragmentSize );
            uint8_t * src = &( block.GetData()[fragmentId*m_config.blockFragmentSize] );
            uint8_t * dst = data->fragment;
            memcpy( dst, src, fragmentBytes );

            auto sentPacketData = m_sentPackets->Insert( sequence );
            CORE_ASSERT( sentPacketData );
            sentPacketData->acked = 0;
            sentPacketData->largeBlock = 1;
            sentPacketData->blockId = m_oldestUnackedMessageId;
            sentPacketData->fragmentId = fragmentId;
            sentPacketData->timeSent = m_timeBase.time;
            sentPacketData->messageIds = nullptr;
            sentPacketData->numMessageIds = 0;

            return data;
        }
        else
        {
            /*
                Message and small block mode.
                Iterate across send queue and include multiple messages 
                per-packet, but stop before the next large block.
            */

            CORE_ASSERT( !m_sendLargeBlock.active );

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

            CORE_ASSERT( numMessageIds >= 0 );
            CORE_ASSERT( numMessageIds <= m_config.maxMessagesPerPacket );

            // if there are no messages then we don't have any data to send

            if ( numMessageIds == 0 )
                return nullptr;

            // add sent packet data containing message ids included in this packet

            auto sentPacketData = m_sentPackets->Insert( sequence );
            CORE_ASSERT( sentPacketData );
            sentPacketData->acked = 0;
            sentPacketData->largeBlock = 0;
            sentPacketData->blockId = 0;
            sentPacketData->fragmentId = 0;
            sentPacketData->timeSent = m_timeBase.time;
            const int sentPacketIndex = m_sentPackets->GetIndex( sequence );
            sentPacketData->messageIds = &m_sentPacketMessageIds[sentPacketIndex*m_config.maxMessagesPerPacket];
            sentPacketData->numMessageIds = numMessageIds;
            for ( int i = 0; i < numMessageIds; ++i )
                sentPacketData->messageIds[i] = messageIds[i];

            // update counter: num messages written

            m_counters[RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_WRITTEN] += numMessageIds;

            // construct channel data for packet

            core::Allocator & allocator = core::memory::scratch_allocator();

            auto data = CORE_NEW( allocator, ReliableMessageChannelData, m_config );

            data->messages = (Message**) allocator.Allocate( numMessageIds * sizeof( Message* ) );
            CORE_ASSERT( data->messages );
//                printf( "allocate messages %p (get data)\n", data->messages );
            data->numMessages = numMessageIds;
            for ( int i = 0; i < numMessageIds; ++i )
            {
                auto entry = m_sendQueue->Find( messageIds[i] );
                CORE_ASSERT( entry );
                CORE_ASSERT( entry->message );
                data->messages[i] = entry->message;
                m_config.messageFactory->AddRef( entry->message );
            }

//                printf( "sent %d messages in packet\n", data->messages.size() );

            return data;
        }
    }

    bool ReliableMessageChannel::ProcessData( uint16_t sequence, ChannelData * channelData )
    {
        CORE_ASSERT( channelData );

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

        if ( data->largeBlock && core::sequence_less_than( data->blockId, m_receiveQueue->GetSequence() ) )
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

                CORE_ASSERT( numFragments >= 0 );
                CORE_ASSERT( numFragments <= m_maxBlockFragments );

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

                CORE_ASSERT( m_config.largeBlockAllocator );
                uint8_t * blockData = (uint8_t*) m_config.largeBlockAllocator->Allocate( data->blockSize );
                m_receiveLargeBlock.block.Connect( *m_config.largeBlockAllocator, blockData, data->blockSize );
                
                m_receiveLargeBlock.received_fragment->Clear();
            }

            CORE_ASSERT( m_receiveLargeBlock.active );

            if ( data->blockId != m_receiveLargeBlock.blockId )
            {
//                    printf( "unexpected large block id. got %d but was expecting %d\n", data->blockId, m_receiveLargeBlock.blockId );
                return false;
            }

            CORE_ASSERT( data->blockId == m_receiveLargeBlock.blockId );
            CORE_ASSERT( data->blockSize == m_receiveLargeBlock.blockSize );
            CORE_ASSERT( data->fragmentId < m_receiveLargeBlock.numFragments );

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

            if ( !m_receiveLargeBlock.received_fragment->GetBit( data->fragmentId ) )
            {
/*
                printf( "received fragment " << data->fragmentId << " of large block " << m_receiveLargeBlock.blockId
                     << " (" << m_receiveLargeBlock.numReceivedFragments+1 << "/" << m_receiveLargeBlock.numFragments << ")" << endl;
                     */

                m_receiveLargeBlock.received_fragment->SetBit( data->fragmentId );

                Block & block = m_receiveLargeBlock.block;

                int fragmentBytes = m_config.blockFragmentSize;
                int fragmentRemainder = block.GetSize() % m_config.blockFragmentSize;
                if ( fragmentRemainder && data->fragmentId == m_receiveLargeBlock.numFragments - 1 )
                    fragmentBytes = fragmentRemainder;

//                    printf( "fragment bytes = %d\n", fragmentBytes );

                CORE_ASSERT( fragmentBytes >= 0 );
                CORE_ASSERT( fragmentBytes <= m_config.blockFragmentSize );
                uint8_t * src = data->fragment;
                uint8_t * dst = &( block.GetData()[data->fragmentId*m_config.blockFragmentSize] );
                memcpy( dst, src, fragmentBytes );

                m_receiveLargeBlock.numReceivedFragments++;

                if ( m_receiveLargeBlock.numReceivedFragments == m_receiveLargeBlock.numFragments )
                {
//                        printf( "received large block %d (%d bytes)\n", m_receiveLargeBlock.blockId, m_receiveLargeBlock.block->size() );

                    auto blockMessage = (BlockMessage*) m_config.messageFactory->Create( BlockMessageType );
                    CORE_ASSERT( blockMessage );
                    blockMessage->Connect( m_receiveLargeBlock.block );
                    blockMessage->SetId( m_receiveLargeBlock.blockId );

                    auto entry = m_receiveQueue->Insert( m_receiveLargeBlock.blockId );
                    CORE_ASSERT( entry );
                    entry->message = blockMessage;

                    m_receiveLargeBlock.active = false;

                    CORE_ASSERT( !m_receiveLargeBlock.block.IsValid() );
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

            CORE_ASSERT( data->messages );

            for ( int i = 0; i < data->numMessages; ++i )
            {
                auto message = data->messages[i];

                CORE_ASSERT( message );

                const uint16_t messageId = message->GetId();

                if ( core::sequence_less_than( messageId, minMessageId ) )
                {
                    m_counters[RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_LATE]++;
                }
                else if ( core::sequence_greater_than( messageId, maxMessageId ) )
                {
                    m_counters[RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY]++;
                    earlyMessage = true;
                }
                else if ( !m_receiveQueue->Find( messageId ) )
                {
                    auto entry = m_receiveQueue->Insert( messageId );
                    entry->message = message;
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

        CORE_ASSERT( !core::sequence_greater_than( m_oldestUnackedMessageId, stopMessageId ) );
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
                    CORE_ASSERT( sendQueueEntry->message );
                    CORE_ASSERT( sendQueueEntry->message->GetId() == messageId );

//                        printf( "acked message %d\n", messageId );

                    m_config.messageFactory->Release( sendQueueEntry->message );

                    m_sendQueue->Remove( messageId );
                }
            }

            UpdateOldestUnackedMessageId();
        }
        else if ( m_sendLargeBlock.active && m_sendLargeBlock.blockId == sentPacket->blockId )
        {
            CORE_ASSERT( sentPacket->fragmentId < m_sendLargeBlock.numFragments );

            if ( !m_sendLargeBlock.acked_fragment->GetBit( sentPacket->fragmentId ) )
            {
                /*
                printf( "acked fragment " << sentPacket->fragmentId << " of large block " << m_sendLargeBlock.blockId 
                     << " (" << m_sendLargeBlock.numAckedFragments+1 << "/" << m_sendLargeBlock.numFragments << ")" << endl;
                     */

                m_sendLargeBlock.acked_fragment->SetBit( sentPacket->fragmentId );
                
                m_sendLargeBlock.numAckedFragments++;

                if ( m_sendLargeBlock.numAckedFragments == m_sendLargeBlock.numFragments )
                {
//                        printf( "acked large block %d\n", (int) m_sendLargeBlock.blockId );

                    m_sendLargeBlock.active = false;

                    auto sendQueueEntry = m_sendQueue->Find( sentPacket->blockId );
                    CORE_ASSERT( sendQueueEntry );

                    m_config.messageFactory->Release( sendQueueEntry->message );                    

                    m_sendQueue->Remove( sentPacket->blockId );

                    UpdateOldestUnackedMessageId();
                }
            }
        }
        
        sentPacket->acked = 1;
    }

    void ReliableMessageChannel::Update( const core::TimeBase & timeBase )
    {
        m_timeBase = timeBase;
    }

    uint64_t ReliableMessageChannel::GetCounter( int index ) const
    {
        CORE_ASSERT( index >= 0 );
        CORE_ASSERT( index < RELIABLE_MESSAGE_CHANNEL_COUNTER_NUM_COUNTERS );
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
