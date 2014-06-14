/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_RELIABLE_MESSAGE_CHANNEL_H
#define PROTOCOL_RELIABLE_MESSAGE_CHANNEL_H

#include "MessageChannel.h"    
#include "Memory.h"
#include <math.h>

namespace protocol
{
    struct ReliableMessageChannelConfig
    {
        ReliableMessageChannelConfig()
        {
            resendRate = 0.1f;
            sendQueueSize = 1024;
            receiveQueueSize = 256;
            sentPacketsSize = 256;
            maxMessagesPerPacket = 32;
            maxMessageSize = 64;
            maxSmallBlockSize = 64;
            maxLargeBlockSize = 256 * 1024;
            blockFragmentSize = 64;
            packetBudget = 128;
            giveUpBits = 128;
            align = true;
        }

        float resendRate;               // message max resend rate in seconds, until acked.
        int sendQueueSize;              // send queue size in # of entries
        int receiveQueueSize;           // receive queue size in # of entries
        int sentPacketsSize;            // sent packets sliding window size in # of entries
        int maxMessagesPerPacket;       // maximum number of messages included in a packet
        int maxMessageSize;             // maximum message size allowed in iserialized bytes, eg. post bitpacker
        int maxSmallBlockSize;          // maximum small block size allowed. messages above this size are fragmented and reassembled.
        int maxLargeBlockSize;          // maximum large block size. these blocks are split up into fragments.
        int blockFragmentSize;          // fragment size that large blocks are split up to for transmission.
        int packetBudget;               // maximum number of bytes this channel may take per-packet. 
        int giveUpBits;                 // give up trying to add more messages to packet if we have less than this # of bits available.
        bool align;                     // if true then insert align at key points, eg. before messages etc. good for dictionary based LZ compressors

        Factory<Message> * messageFactory = nullptr;
    };

    class ReliableMessageChannelData : public ChannelData
    {
        const ReliableMessageChannelConfig & config;

        ReliableMessageChannelData( const ReliableMessageChannelData & other );
        ReliableMessageChannelData & operator = ( const ReliableMessageChannelData & other );

    public:

        // todo: replace this! should be just a pointer to an array
        std::vector<Message*> messages;        // array of messages. valid if *not* sending large block.

        uint64_t largeBlock : 1;               // true if currently sending a large block.
        uint64_t blockSize : 32;               // block size in bytes. valid if sending large block.
        uint64_t blockId : 16;                 // block id. valid if sending large block.
        uint64_t fragmentId : 16;              // fragment id. valid if sending large block.
        uint64_t releaseMessages : 1;          // 1 if this channel data should release the messages in destructor.
        uint8_t * fragment;                    // the actual fragment data. valid if sending large block.

        ReliableMessageChannelData( const ReliableMessageChannelConfig & _config ) 
            : config( _config ), largeBlock(0), blockSize(0), blockId(0), fragmentId(0), releaseMessages(0)
        {
//            printf( "create reliable message channel data: %p\n", this );
            fragment = nullptr;
        }

        ~ReliableMessageChannelData()
        {
//            printf( "destroy reliable message channel data: %p\n", this );

            if ( fragment )
            {
//                printf( "deallocate fragment %p (channel data dtor)\n", fragment );
                Allocator & a = memory::default_scratch_allocator();
                a.Deallocate( fragment );
                fragment = nullptr;
            }

            if ( releaseMessages )
            {
                for ( int i = 0; i < messages.size(); ++i )
                {
                    assert( messages[i] );
                    messages[i]->Release();
                    if ( messages[i]->GetRefCount() == 0 )
                        delete messages[i];
                }
            }

            messages.resize( 0 );
        }

        template <typename Stream> void Serialize( Stream & stream )
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
//                    printf( "allocate fragment %p (read stream)\n", fragment );
                }

                serialize_bits( stream, blockId, 16 );
                serialize_bits( stream, fragmentId, 16 );
                serialize_bits( stream, blockSize, 32 );

                // todo: this can be made a lot faster!!!
                for ( int i = 0; i < config.blockFragmentSize; ++i )
                    serialize_bits( stream, fragment[i], 8 );
            }
            else
            {
                assert( config.messageFactory );

                int numMessages;
                if ( Stream::IsWriting )
                    numMessages = messages.size();

                if ( Stream::IsWriting )
                    assert( numMessages > 0 );

                serialize_int( stream, numMessages, 0, config.maxMessagesPerPacket );

                // blegh!!!!                
                if ( Stream::IsReading )
                    messages.resize( numMessages );

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
                    }

                    serialize_object( stream, *messages[i] );
                }
            }
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
    };

    class ReliableMessageChannel : public MessageChannel
    {
        struct SendQueueEntry
        {
            Message * message = nullptr;
            double timeLastSent;
            uint64_t sequence : 16;                      // this is the message id      
            uint64_t valid : 1;
            uint64_t largeBlock : 1;
            uint64_t measuredBits : 15;
            SendQueueEntry()
                : valid(0) {}
            SendQueueEntry( Message * _message, uint16_t _sequence, bool _largeBlock )
                : message( _message ), timeLastSent(-1), sequence( _sequence ), valid(1), largeBlock(_largeBlock), measuredBits(0) { assert( _message ); }
        };

        struct SentPacketEntry
        {
            std::vector<uint16_t> messageIds;
            double timeSent;
            uint64_t sequence : 16;                      // this is the packet sequence #
            uint64_t acked : 1;
            uint64_t valid : 1;
            uint64_t largeBlock : 1;                     // if 1 then this sent packet contains a large block fragment
            uint64_t blockId : 16;                       // block id. valid only when sending large block.
            uint64_t fragmentId : 16;                    // fragment id. valid only when sending large block.
            SentPacketEntry() : valid(0) {}
        };

        struct ReceiveQueueEntry
        {
            Message * message = nullptr;
            uint32_t sequence : 16;                      // this is the message id      
            uint32_t valid : 1;
            ReceiveQueueEntry()
                : valid(0) {}
            ReceiveQueueEntry( Message * _message, uint16_t _sequence )
                : message( _message ), sequence( _sequence ), valid(1) { assert( _message ); }
        };

        struct SendFragmentData
        {
            double timeLastSent;
            uint32_t acked : 1;
            SendFragmentData()
                : timeLastSent(-1), acked(0) {}
        };

        struct ReceiveFragmentData
        {
            uint32_t received : 1;
            ReceiveFragmentData()
                : received(0) {}
        };

        struct SendLargeBlockData
        {
            SendLargeBlockData()
            {
                Reset();
            }

            void Reset()
            {
                active = false;
                numFragments = 0;
                numAckedFragments = 0;
                blockId = 0;
                blockSize = 0;
            }

            bool active;                                // true if we are currently sending a large block
            int numFragments;                           // number of fragments in the current large block being sent
            int numAckedFragments;                      // number of acked fragments in current block being sent
            int blockSize;                              // send block size in bytes
            uint16_t blockId;                           // the message id for the current large block being sent
            std::vector<SendFragmentData> fragments;    // per-fragment data for send
        };

        struct ReceiveLargeBlockData
        {
            ReceiveLargeBlockData()
            {
                Reset();
            }

            void Reset()
            {
                active = false;
                numFragments = 0;
                numReceivedFragments = 0;
                blockId = 0;
                blockSize = 0;                
            }

            bool active;                                // true if we are currently receiving a large block
            int numFragments;                           // number of fragments in this block
            int numReceivedFragments;                   // number of fragments received.
            uint16_t blockId;                           // block id being currently received.
            uint32_t blockSize;                         // block size in bytes.
            std::vector<ReceiveFragmentData> fragments; // per-fragment data for receive
            Block * block;                              // the block being received.
        };

    public:

        enum Counters
        {
            MessagesSent,
            MessagesWritten,
            MessagesRead,
            MessagesReceived,
            MessagesLate,
            MessagesEarly,
            NumCounters
        };

        struct SendLargeBlockStatus
        {
            bool sending;
            int blockId;
            int blockSize;
            int numFragments;
            int numAckedFragments;
        };

        struct ReceiveLargeBlockStatus
        {
            bool receiving;
            int blockId;
            int blockSize;
            int numFragments;
            int numReceivedFragments;
        };

    private:

        const ReliableMessageChannelConfig m_config;                        // constant configuration data

        int m_maxBlockFragments;                                            // maximum number of fragments per-block
        int m_messageOverheadBits;                                          // number of bits overhead per-serialized message

        TimeBase m_timeBase;                                                // current time base from last update
        uint16_t m_sendMessageId;                                           // id for next message added to send queue
        uint16_t m_receiveMessageId;                                        // id for next message to be received
        uint16_t m_oldestUnackedMessageId;                                  // id for oldest unacked message in send queue

        SlidingWindow<SendQueueEntry> * m_sendQueue;                        // message send queue
        SlidingWindow<SentPacketEntry> * m_sentPackets;                     // sent packets (for acks)
        SlidingWindow<ReceiveQueueEntry> * m_receiveQueue;                  // message receive queue

        SendLargeBlockData m_sendLargeBlock;                                // data for large block being sent
        ReceiveLargeBlockData m_receiveLargeBlock;                          // data for large block being received

        uint64_t m_counters[NumCounters];                                   // counters used for unit testing and validation

        ReliableMessageChannel( const ReliableMessageChannel & other );
        ReliableMessageChannel & operator = ( const ReliableMessageChannel & other );

    public:

        ReliableMessageChannel( const ReliableMessageChannelConfig & config ) : m_config( config )
        {
            assert( config.messageFactory );
            assert( config.maxSmallBlockSize <= MaxSmallBlockSize );

            m_sendQueue = new SlidingWindow<SendQueueEntry>( m_config.sendQueueSize );
            m_sentPackets = new SlidingWindow<SentPacketEntry>( m_config.sentPacketsSize );
            m_receiveQueue = new SlidingWindow<ReceiveQueueEntry>( m_config.receiveQueueSize );

            const int MessageIdBits = 16;
            const int MessageTypeBits = bits_required( 0, m_config.messageFactory->GetMaxType() );
            const int MessageAlignOverhead = m_config.align ? 14 : 0;

            m_messageOverheadBits = MessageIdBits + MessageTypeBits + MessageAlignOverhead;

            m_maxBlockFragments = (int) ceil( m_config.maxLargeBlockSize / (float)m_config.blockFragmentSize );

            m_sendLargeBlock.fragments.resize( m_maxBlockFragments );
            m_receiveLargeBlock.fragments.resize( m_maxBlockFragments );

            Reset();
        }

        ~ReliableMessageChannel()
        {
            // todo: if there are any messages in the send queue or receive queue they need to be released
            // and deleted if their ref count has reached zero.

            assert( m_sendQueue );
            assert( m_sentPackets );
            assert( m_receiveQueue );

            delete m_sendQueue;
            delete m_sentPackets;
            delete m_receiveQueue;

            m_sendQueue = nullptr;
            m_sentPackets = nullptr;
            m_receiveQueue = nullptr;
        }

        void Reset()
        {
            m_sendMessageId = 0;
            m_receiveMessageId = 0;
            m_oldestUnackedMessageId = 0;

            m_sendQueue->Reset();
            m_sentPackets->Reset();
            m_receiveQueue->Reset();

            memset( m_counters, 0, sizeof( m_counters ) );

            m_timeBase = TimeBase();

            m_sendLargeBlock.Reset();
            m_receiveLargeBlock.Reset();
        }

        bool CanSendMessage() const
        {
            return m_sendQueue->HasSlotAvailable( m_sendMessageId );
        }

        void SendMessage( Message * message )
        {
//            printf( "queue message for send: %d\n", m_sendMessageId );

            assert( CanSendMessage() );
            if ( !CanSendMessage() )
            {
                // todo: we need a way to set a fatal error here
                // the connection must be torn down at this point
                // we cannot continue.
                delete message;
                return;
            }

            message->SetId( m_sendMessageId );

            bool largeBlock = false;
            if ( message->IsBlock() )
            {
                BlockMessage & blockMessage = static_cast<BlockMessage&>( *message );
                auto block = blockMessage.GetBlock();
                assert( block );
                if ( block->size() > m_config.maxSmallBlockSize )
                    largeBlock = true;
            }

    /*
            if ( largeBlock )
                printf( "sent large block %d\n", (int) m_sendMessageId );
*/

            message->AddRef();
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

            m_counters[MessagesSent]++;

            m_sendMessageId++;
        }

        void SendBlock( Block * block )
        {
//            printf( "send block: %d bytes\n", block->size() );

            auto blockMessage = new BlockMessage( block );

            SendMessage( blockMessage );
        }        

        Message * ReceiveMessage()
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
            
            m_counters[MessagesReceived]++;

            m_receiveMessageId++;

            assert( message->GetRefCount() == 0 );      // must be a normal object. delete when done with it

            return message;
        }

        ChannelData * CreateData()
        {
            return new ReliableMessageChannelData( m_config );
        }

        ChannelData * GetData( uint16_t sequence )
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

                auto block = blockMessage.GetBlock();

                assert( block );
                assert( block->size() > m_config.maxSmallBlockSize );

                if ( !m_sendLargeBlock.active )
                {
                    m_sendLargeBlock.active = true;
                    m_sendLargeBlock.blockId = m_oldestUnackedMessageId;
                    m_sendLargeBlock.blockSize = block->size();
                    m_sendLargeBlock.numFragments = (int) ceil( block->size() / (float)m_config.blockFragmentSize );
                    m_sendLargeBlock.numAckedFragments = 0;

//                    printf( "sending block %d in %d fragments\n", (int) firstMessageId, m_sendLargeBlock.numFragments );

                    assert( m_sendLargeBlock.numFragments >= 0 );
                    assert( m_sendLargeBlock.numFragments <= m_maxBlockFragments );

                    for ( int i = 0; i < m_sendLargeBlock.numFragments; ++i )
                        m_sendLargeBlock.fragments[i] = SendFragmentData();
                }

                assert( m_sendLargeBlock.active );

                // todo: don't walk across all fragments here -- start at the 
                // oldest unacked fragment and walk across only n entries max

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

                auto data = new ReliableMessageChannelData( m_config );
                data->largeBlock = 1;
                data->blockSize = block->size();
                data->blockId = m_oldestUnackedMessageId;
                data->fragmentId = fragmentId;
                Allocator & a = memory::default_scratch_allocator();
                data->fragment = (uint8_t*) a.Allocate( m_config.blockFragmentSize );
//                printf( "allocate fragment %p (send fragment)\n", data->fragment );

                //printf( "create fragment %p\n", data->fragment );

                int fragmentBytes = m_config.blockFragmentSize;
                int fragmentRemainder = block->size() % m_config.blockFragmentSize;
                if ( fragmentRemainder && fragmentId == m_sendLargeBlock.numFragments - 1 )
                    fragmentBytes = fragmentRemainder;

                assert( fragmentBytes >= 0 );
                assert( fragmentBytes <= m_config.blockFragmentSize );
                uint8_t * src = &( (*block)[fragmentId*m_config.blockFragmentSize] );
                uint8_t * dst = data->fragment;
                memcpy( dst, src, fragmentBytes );

                auto sentPacketData = m_sentPackets->InsertFast( sequence );
                assert( sentPacketData );
                sentPacketData->acked = 0;
                sentPacketData->largeBlock = 1;
                sentPacketData->blockId = m_oldestUnackedMessageId;
                sentPacketData->fragmentId = fragmentId;
                sentPacketData->timeSent = m_timeBase.time;

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
                sentPacketData->messageIds.resize( numMessageIds );
                for ( int i = 0; i < numMessageIds; ++i )
                    sentPacketData->messageIds[i] = messageIds[i];

                // update counter: num messages written

                m_counters[MessagesWritten] += numMessageIds;

                // construct channel data for packet

                auto data = new ReliableMessageChannelData( m_config );

                data->messages.resize( numMessageIds );
                for ( int i = 0; i < numMessageIds; ++i )
                {
                    auto entry = m_sendQueue->Find( messageIds[i] );
                    assert( entry );
                    assert( entry->message );
                    entry->message->AddRef();
                    data->messages[i] = entry->message;
                }

//                printf( "sent %d messages in packet\n", data->messages.size() );

                return data;
            }
        }

        bool ProcessData( uint16_t sequence, ChannelData * channelData )
        {
            assert( channelData );

  //          printf( "process data %d\n", sequence );

            // todo: must remember to delete the channel data somewhere.
            // maybe not here. probably after the connection calls "ProcessData"
            // for all channels on the packet.

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
                    m_receiveLargeBlock.block = new Block( m_receiveLargeBlock.blockSize );
                    
                    for ( int i = 0; i < numFragments; ++i )
                        m_receiveLargeBlock.fragments[i] = ReceiveFragmentData();
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

                    auto block = m_receiveLargeBlock.block;

                    int fragmentBytes = m_config.blockFragmentSize;
                    int fragmentRemainder = block->size() % m_config.blockFragmentSize;
                    if ( fragmentRemainder && data->fragmentId == m_receiveLargeBlock.numFragments - 1 )
                        fragmentBytes = fragmentRemainder;

//                    printf( "fragment bytes = %d\n", fragmentBytes );

                    assert( fragmentBytes >= 0 );
                    assert( fragmentBytes <= m_config.blockFragmentSize );
                    uint8_t * src = data->fragment;
                    uint8_t * dst = &( (*block)[data->fragmentId*m_config.blockFragmentSize] );
                    memcpy( dst, src, fragmentBytes );

                    m_receiveLargeBlock.numReceivedFragments++;

                    if ( m_receiveLargeBlock.numReceivedFragments == m_receiveLargeBlock.numFragments )
                    {
//                        printf( "received large block %d (%d bytes)\n", m_receiveLargeBlock.blockId, m_receiveLargeBlock.block->size() );

                        auto blockMessage = new BlockMessage( m_receiveLargeBlock.block );

                        blockMessage->SetId( m_receiveLargeBlock.blockId );

                        m_receiveQueue->Insert( ReceiveQueueEntry( blockMessage, m_receiveLargeBlock.blockId ) );

                        m_receiveLargeBlock.active = false;
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

    //            printf( "%d messages in packet\n", data->messages.size() );

                for ( auto message : data->messages )
                {
                    assert( message );

                    const uint16_t messageId = message->GetId();

    //                printf( "add message to receive queue: %d\n", (int) messageId );

                    if ( sequence_less_than( messageId, minMessageId ) )
                    {
//                        printf( "old message %d, min = %d, max = %d\n", messageId, minMessageId, maxMessageId );

                        m_counters[MessagesLate]++;
                    }
                    else if ( sequence_greater_than( messageId, maxMessageId ) )
                    {
//                        printf( "early message %d\n", messageId );

                        earlyMessage = true;

                        m_counters[MessagesEarly]++;
                    }
                    else
                    {
                        bool result = m_receiveQueue->Insert( ReceiveQueueEntry( message, messageId ) );

                        assert( result );
                    }

                    m_counters[MessagesRead]++;
                }

                if ( earlyMessage )
                    return false;
            }

            return true;
        }

        void UpdateOldestUnackedMessageId()
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

        void ProcessAck( uint16_t ack )
        {
//            printf( "process ack: %d\n", (int) ack );

            auto sentPacket = m_sentPackets->Find( ack );
            if ( !sentPacket || sentPacket->acked )
                return;

            if ( !sentPacket->largeBlock )
            {
                for ( auto messageId : sentPacket->messageIds )
                {
                    auto sendQueueEntry = m_sendQueue->Find( messageId );
                    if ( sendQueueEntry )
                    {
                        assert( sendQueueEntry->message );
                        assert( sendQueueEntry->message->GetId() == messageId );

//                        printf( "acked message %d\n", messageId );

                        sendQueueEntry->message->Release();
                        if ( sendQueueEntry->message->GetRefCount() == 0 )
                            delete sendQueueEntry->message;

                        sendQueueEntry->valid = 0;
                        sendQueueEntry->message = nullptr;
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
                        
                        sendQueueEntry->message->Release();
                        if ( sendQueueEntry->message->GetRefCount() == 0 )
                            delete sendQueueEntry->message;
                        
                        sendQueueEntry->valid = 0;
                        sendQueueEntry->message = nullptr;

                        UpdateOldestUnackedMessageId();
                    }
                }
            }
            
            sentPacket->acked = 1;
        }

        void Update( const TimeBase & timeBase )
        {
            m_timeBase = timeBase;
        }

        uint64_t GetCounter( int index ) const
        {
            assert( index >= 0 );
            assert( index < NumCounters );
            return m_counters[index];
        }

        SendLargeBlockStatus GetSendLargeBlockStatus() const
        {
            SendLargeBlockStatus status;
            status.sending = m_sendLargeBlock.active;
            status.blockId = m_sendLargeBlock.blockId;
            status.blockSize = m_sendLargeBlock.blockSize;
            status.numFragments = m_sendLargeBlock.numFragments;
            status.numAckedFragments = m_sendLargeBlock.numAckedFragments;
            return status;
        }

        ReceiveLargeBlockStatus GetReceiveLargeBlockStatus() const
        {
            ReceiveLargeBlockStatus status;
            status.receiving = m_receiveLargeBlock.active;
            status.blockId = m_receiveLargeBlock.blockId;
            status.blockSize = m_receiveLargeBlock.blockSize;
            status.numFragments = m_receiveLargeBlock.numFragments;
            status.numReceivedFragments = m_receiveLargeBlock.numReceivedFragments;
            return status;
        }
    };
}

#endif
