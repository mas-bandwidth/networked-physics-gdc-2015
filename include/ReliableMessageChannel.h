/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_RELIABLE_MESSAGE_CHANNEL_H
#define PROTOCOL_RELIABLE_MESSAGE_CHANNEL_H

#include "MessageChannel.h"    
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
            giveUpBits = 64;
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

        // todo: move packet budget into the controlling owner, and pass it in to "GetData". should be *dynamic*

        shared_ptr<Factory<Message>> messageFactory;
    };

    class ReliableMessageChannelData : public ChannelData
    {
        const ReliableMessageChannelConfig & config;

    public:

        ReliableMessageChannelData( const ReliableMessageChannelConfig & _config ) 
            : config( _config ), largeBlock(0), blockSize(0), blockId(0), fragmentId(0) {}

        vector<shared_ptr<Message>> messages;           // array of messages. valid if *not* sending large block.
        uint64_t largeBlock : 1;                        // true if currently sending a large block.
        uint64_t blockSize : 32;                        // block size in bytes. valid if sending large block.
        uint64_t blockId : 16;                          // block id. valid if sending large block.
        uint64_t fragmentId : 16;                       // fragment id. valid if sending large block.
        shared_ptr<Block> fragment;                     // the actual fragment data. valid if sending large block.

        void Serialize( Stream & stream )
        {
            serialize_bits( stream, largeBlock, 1 );

            if ( largeBlock )
            {
                if ( stream.IsWriting() )
                {
                    assert( fragment );
                    assert( fragment->size() == config.blockFragmentSize );
                }
                else
                {
                    fragment = make_shared<Block>( config.blockFragmentSize );
                }

                serialize_bits( stream, blockId, 16 );
                serialize_bits( stream, fragmentId, 16 );
                serialize_bits( stream, blockSize, 32 );

                Block & data = *fragment;
                for ( int i = 0; i < config.blockFragmentSize; ++i )
                    serialize_bits( stream, data[i], 8 );
            }
            else
            {
                assert( config.messageFactory );

                int numMessages = stream.IsWriting() ? messages.size() : 0;
                serialize_int( stream, numMessages, 0, config.maxMessagesPerPacket );
                if ( stream.IsReading() )
                    messages.resize( numMessages );

                for ( int i = 0; i < numMessages; ++i )
                {
                    #ifndef NDEBUG
                    if ( stream.IsWriting() )
                        assert( messages[i] );
                    #endif

                    int messageType = stream.IsWriting() ? messages[i]->GetType() : 0;
                    serialize_int( stream, messageType, 0, config.messageFactory->GetMaxType() );
                    if ( stream.IsReading() )
                    {
                        messages[i] = config.messageFactory->Create( messageType );
                        assert( messages[i] );
                        assert( messages[i]->GetType() == messageType );
                    }

                    uint16_t messageId = stream.IsWriting() ? messages[i]->GetId() : 0;
                    serialize_bits( stream, messageId, 16 );
                    if ( stream.IsReading() )
                    {
                        messages[i]->SetId( messageId );
                        assert( messages[i]->GetId() == messageId );
                    }

                    messages[i]->Serialize( stream );
                }
            }
        }
    };

    class ReliableMessageChannel : public MessageChannel
    {
        struct SendQueueEntry
        {
            shared_ptr<Message> message;
            double timeLastSent;
            uint64_t sequence : 16;                      // this is the message id      
            uint64_t valid : 1;
            uint64_t largeBlock : 1;
            uint64_t measuredBits : 15;
            SendQueueEntry()
                : valid(0) {}
            SendQueueEntry( shared_ptr<Message> _message, uint16_t _sequence, bool _largeBlock )
                : message( _message ), timeLastSent(-1), sequence( _sequence ), valid(1), largeBlock(_largeBlock), measuredBits(0) {}
        };

        struct SentPacketEntry
        {
            vector<uint16_t> messageIds;
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
            shared_ptr<Message> message;
            uint32_t sequence : 16;                      // this is the message id      
            uint32_t valid : 1;
            ReceiveQueueEntry()
                : valid(0) {}
            ReceiveQueueEntry( shared_ptr<Message> _message, uint16_t _sequence )
                : message( _message ), sequence( _sequence ), valid(1) {}
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
                active = false;
                numFragments = 0;
                numAckedFragments = 0;
                blockId = 0;
            }

            bool active;                                // true if we are currently sending a large block
            int numFragments;                           // number of fragments in the current large block being sent
            int numAckedFragments;                      // number of acked fragments in current block being sent
            uint16_t blockId;                           // the message id for the current large block being sent
            vector<SendFragmentData> fragments;         // per-fragment data for send
        };

        struct ReceiveLargeBlockData
        {
            ReceiveLargeBlockData()
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
            vector<ReceiveFragmentData> fragments;      // per-fragment data for receive
            shared_ptr<Block> block;                    // the actual block being received!!!
        };

    public:

        enum Counters
        {
            MessagesSent,
            MessagesWritten,
            MessagesRead,
            MessagesReceived,
            MessagesDiscardedLate,
            MessagesDiscardedEarly,
            NumCounters
        };

        ReliableMessageChannel( const ReliableMessageChannelConfig & config ) : m_config( config )
        {
            assert( config.messageFactory );
            assert( config.maxSmallBlockSize <= MaxSmallBlockSize );

            m_sendMessageId = 0;
            m_receiveMessageId = 0;

            m_sendQueue = make_shared<SlidingWindow<SendQueueEntry>>( m_config.sendQueueSize );
            m_sentPackets = make_shared<SlidingWindow<SentPacketEntry>>( m_config.sentPacketsSize );
            m_receiveQueue = make_shared<SlidingWindow<ReceiveQueueEntry>>( m_config.receiveQueueSize );

            m_counters.resize( NumCounters, 0 );

            const int SmallBlockOverhead = bits_required( 0, MaxSmallBlockSize - 1 );
            m_measureBuffer.resize( max( m_config.maxMessageSize, m_config.maxSmallBlockSize + SmallBlockOverhead ) );

//            cout << "measure buffer is " << m_measureBuffer.size() << " bytes" << endl;

            const int MessageIdBits = 16;
            const int MessageTypeBits = bits_required( 0, m_config.messageFactory->GetMaxType() );
            m_messageOverheadBits = MessageIdBits + MessageTypeBits;

//            cout << "message overhead is " << m_messageOverheadBits << " bits" << endl;

            m_maxBlockFragments = (int) ceil( m_config.maxLargeBlockSize / (float)m_config.blockFragmentSize );

//            cout << "max block fragments = " << m_maxBlockFragments << endl;

            m_sendLargeBlock.fragments.resize( m_maxBlockFragments );
            m_receiveLargeBlock.fragments.resize( m_maxBlockFragments );
        }

        bool CanSendMessage() const
        {
            return m_sendQueue->HasSlotAvailable( m_sendMessageId );
        }

        void SendMessage( shared_ptr<Message> message )
        {
//            cout << "queue message for send: " << m_sendMessageId << endl;

            if ( !m_sendQueue->HasSlotAvailable( m_sendMessageId ) )
                throw runtime_error( "message send queue overflow" );

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
                cout << "sent large block " << m_sendMessageId << endl;
*/
            bool result = m_sendQueue->Insert( SendQueueEntry( message, m_sendMessageId, largeBlock ) );
            assert( result );

            #ifndef NDEBUG
            auto entry = m_sendQueue->Find( m_sendMessageId );
            assert( entry );
            assert( entry->valid );
            assert( entry->sequence == m_sendMessageId );
            assert( entry->message );
            assert( entry->message->GetId() == m_sendMessageId );
            #endif

            if ( !largeBlock )
            {
                // todo: would be nice to have a specialized measure stream that doesn't actually do any work
                // but measure the # of bits required for a specific serialization
                Stream stream( STREAM_Write, &m_measureBuffer[0], m_measureBuffer.size() );
                message->Serialize( stream );
                entry->measuredBits = stream.GetBits() + m_messageOverheadBits;

//              cout << "message " << m_sendMessageId << " is " << entry->measuredBits << " bits " << endl;
            }

            m_counters[MessagesSent]++;

            m_sendMessageId++;
        }

        void SendBlock( shared_ptr<Block> block )
        {
//            cout << "send block: " << block->size() << " bytes" << endl;

            auto blockMessage = make_shared<BlockMessage>( block );

            SendMessage( blockMessage );
        }        

        shared_ptr<Message> ReceiveMessage()
        {
            auto entry = m_receiveQueue->Find( m_receiveMessageId );
            if ( !entry )
                return nullptr;

            auto message = entry->message;

            #ifndef NDEBUG
            assert( message );
            assert( message->GetId() == m_receiveMessageId );
            #endif

//            cout << "dequeue for receive: " << message->GetId() << endl;

            entry->valid = 0;
            entry->message = nullptr;
            
            m_counters[MessagesReceived]++;

            m_receiveMessageId++;

            return message;
        }

        shared_ptr<ChannelData> CreateData()
        {
            return make_shared<ReliableMessageChannelData>( m_config );
        }

        shared_ptr<ChannelData> GetData( uint16_t sequence )
        {
            // todo: would be nice to cache the oldest message id in send queue
            // so we don't have to keep calculating it on every packet sent

            // find first message id in send queue

            bool foundMessage = false;
            uint16_t firstMessageId = 0;
            const uint16_t baseId = m_sendMessageId - m_config.sendQueueSize;
            for ( int i = 0; i < m_config.sendQueueSize; ++i )
            {
                const uint16_t messageId = baseId + i;
                SendQueueEntry * entry = m_sendQueue->Find( messageId );
                if ( entry )
                {
                    if ( !foundMessage || sequence_less_than( messageId, firstMessageId ) )
                    {
                        firstMessageId = messageId;
                        foundMessage = true;
                    }
                }
            }

            if ( !foundMessage )
                return nullptr;

            SendQueueEntry * firstEntry = m_sendQueue->Find( firstMessageId );

            assert( firstEntry );

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
                    m_sendLargeBlock.blockId = firstMessageId;
                    m_sendLargeBlock.numFragments = (int) ceil( block->size() / (float)m_config.blockFragmentSize );
                    m_sendLargeBlock.numAckedFragments = 0;

//                    cout << "sending large block " << firstMessageId << " in " << m_sendLargeBlock.numFragments << " fragments" << endl;

                    assert( m_sendLargeBlock.numFragments >= 0 );
                    assert( m_sendLargeBlock.numFragments <= m_maxBlockFragments );

                    for ( int i = 0; i < m_sendLargeBlock.numFragments; ++i )
                        m_sendLargeBlock.fragments[i] = SendFragmentData();
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

//                cout << "sending fragment " << fragmentId << endl;

                auto data = make_shared<ReliableMessageChannelData>( m_config );
                data->largeBlock = 1;
                data->blockSize = block->size();
                data->blockId = firstMessageId;
                data->fragmentId = fragmentId;
                data->fragment = make_shared<Block>( m_config.blockFragmentSize, 0 );

                int fragmentBytes = m_config.blockFragmentSize;
                int fragmentRemainder = block->size() % m_config.blockFragmentSize;
                if ( fragmentRemainder && fragmentId == m_sendLargeBlock.numFragments - 1 )
                    fragmentBytes = fragmentRemainder;

                assert( fragmentBytes >= 0 );
                assert( fragmentBytes <= m_config.blockFragmentSize );
                uint8_t * src = &( (*block)[fragmentId*m_config.blockFragmentSize] );
                uint8_t * dst = &( (*data->fragment)[0] );
                memcpy( dst, src, fragmentBytes );

                auto sentPacketData = m_sentPackets->InsertFast( sequence );
                assert( sentPacketData );
                sentPacketData->acked = 0;
                sentPacketData->largeBlock = 1;
                sentPacketData->blockId = firstMessageId;
                sentPacketData->fragmentId = fragmentId;
                sentPacketData->timeSent = m_timeBase.time;

                // optimization #1: cache oldest unacked fragment id. start the search
                // from there, instead of walking the entire block left to right.

                // optimization #2: only walk across some window size across the fragments
                // eg. up to 256 (configurable) past the first unacked block. potentially
                // a significant optimization for large block sizes (eg. megabytes)
                // call this "fragmentWindowSize"

                // optimization #3: it would be really nice not to have to actually copy
                // the data on write *at all*. instead use a pointer to the actual block
                // data and serialize direct from that.

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
                int numMessageIds = 0;
                uint16_t messageIds[m_config.maxMessagesPerPacket];
                for ( int i = 0; i < m_config.receiveQueueSize; ++i )
                {
                    if ( availableBits < m_config.giveUpBits )
                        break;
                    const uint16_t messageId = firstMessageId + i;
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

    //            cout << "wrote " << numMessageIds << " of " << m_config.maxMessagesPerPacket << " maximum messages, using " << m_config.packetBudget * 8 - availableBits << " of " << m_config.packetBudget * 8 << " available bits" << endl;

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

                auto data = make_shared<ReliableMessageChannelData>( m_config );

                data->messages.resize( numMessageIds );
                for ( int i = 0; i < numMessageIds; ++i )
                {
                    auto entry = m_sendQueue->Find( messageIds[i] );
                    assert( entry );
                    assert( entry->message );
                    data->messages[i] = entry->message;
                }

                return data;
            }

        }

        void ProcessData( uint16_t sequence, shared_ptr<ChannelData> channelData )
        {
            assert( channelData );

            auto data = reinterpret_cast<ReliableMessageChannelData&>( *channelData );

//            cout << "process message channel data: " << sequence << endl;

            /*
                If we are processing a large block, but we receive a packet that
                contains non-large block data, eg. bitpacked messages or small blocks
                throw an exception to discard the packet.
            */

            if ( !data.largeBlock && m_receiveLargeBlock.active )
                throw runtime_error( "received unexpected bitpacked message or small block while receiving large block" );

            // now process the packet data according to its contents...

            if ( data.largeBlock )
            {
                /*
                    Large block mode.
                    This packet includes a fragment for the large block currently 
                    being received. Only one large block is sent at a time.
                */

                if ( !m_receiveLargeBlock.active )
                {
                    const uint16_t expectedBlockId = m_receiveQueue->GetSequence();

                    if ( data.blockId != expectedBlockId )
                        throw runtime_error( "unexpected large block id" );

                    const int numFragments = (int) ceil( data.blockSize / (float)m_config.blockFragmentSize );

                    assert( numFragments >= 0 );
                    assert( numFragments <= m_maxBlockFragments );

                    if ( numFragments < 0 || numFragments > m_maxBlockFragments )
                        throw runtime_error( "large block num fragments outside of range" );

//                    cout << "receiving large block " << data.blockId << " (" << data.blockSize << " bytes)" << endl;

                    m_receiveLargeBlock.active = true;
                    m_receiveLargeBlock.numFragments = numFragments;
                    m_receiveLargeBlock.numReceivedFragments = 0;
                    m_receiveLargeBlock.blockId = data.blockId;
                    m_receiveLargeBlock.blockSize = data.blockSize;
                    m_receiveLargeBlock.block = make_shared<Block>( m_receiveLargeBlock.blockSize );
                    
                    for ( int i = 0; i < numFragments; ++i )
                        m_receiveLargeBlock.fragments[i] = ReceiveFragmentData();
                }

                assert( m_receiveLargeBlock.active );

                assert( data.blockId == m_receiveLargeBlock.blockId );
                assert( data.blockSize == m_receiveLargeBlock.blockSize );
                assert( data.fragmentId < m_receiveLargeBlock.numFragments );

                if ( data.blockId != m_receiveLargeBlock.blockId )
                    throw runtime_error( "receive large block id mismatch" );

                if ( data.blockSize != m_receiveLargeBlock.blockSize )
                    throw runtime_error( "receive large block size mismatch" );

                if ( data.fragmentId >= m_receiveLargeBlock.numFragments )
                    throw runtime_error( "receive large block fragment id out of bounds" );

                auto & fragment = m_receiveLargeBlock.fragments[data.fragmentId];

                if ( !fragment.received )
                {
//                    cout << "received fragment " << data.fragmentId << endl;

                    fragment.received = 1;

                    auto block = m_receiveLargeBlock.block;

                    int fragmentBytes = m_config.blockFragmentSize;
                    int fragmentRemainder = block->size() % m_config.blockFragmentSize;
                    if ( fragmentRemainder && data.fragmentId == m_sendLargeBlock.numFragments - 1 )
                        fragmentBytes = fragmentRemainder;

//                    cout << "fragment bytes " << fragmentBytes << endl;

                    assert( fragmentBytes >= 0 );
                    assert( fragmentBytes <= m_config.blockFragmentSize );
                    uint8_t * src = &( (*data.fragment)[0] );
                    uint8_t * dst = &( (*block)[data.fragmentId*m_config.blockFragmentSize] );
                    memcpy( dst, src, fragmentBytes );

                    m_receiveLargeBlock.numReceivedFragments++;

                    if ( m_receiveLargeBlock.numReceivedFragments == m_receiveLargeBlock.numFragments )
                    {
//                        cout << "received block " << m_receiveLargeBlock.blockId << endl;

                        auto blockMessage = make_shared<BlockMessage>( m_receiveLargeBlock.block );

                        blockMessage->SetId( m_receiveLargeBlock.blockId );

                        m_receiveQueue->Insert( ReceiveQueueEntry( blockMessage, m_receiveLargeBlock.blockId ) );

                        m_receiveLargeBlock.active = false;
                    }
                }

                // todo: optimization. we don't really need anything except one bit per
                // received fragment. it would be a memory optimization (32x) to get it
                // down to a bit array vs. the 32bit per struct in vector.
            }
            else
            {
                /*
                    Bit-packed message and small block mode.
                    Multiple messages and small blocks are included
                    in each packet and each needs to be be processed
                    in a sliding window and dequeued reliably and in-order.
                */

                bool earlyMessage = false;

                const uint16_t minMessageId = m_receiveMessageId;
                const uint16_t maxMessageId = m_receiveMessageId + m_config.receiveQueueSize - 1;

                // process messages included in this packet data

    //            cout << data.messages.size() << " messages in packet" << endl;

                for ( auto message : data.messages )
                {
                    assert( message );

                    const uint16_t messageId = message->GetId();

    //                cout << "add message to receive queue: " << messageId << endl;

                    if ( sequence_less_than( messageId, minMessageId ) )
                    {
    //                    cout << "old message " << messageId << endl;

                        m_counters[MessagesDiscardedLate]++;
                    }
                    else if ( sequence_greater_than( messageId, maxMessageId ) )
                    {
    //                    cout << "early message " << messageId << endl;

                        earlyMessage = true;

                        m_counters[MessagesDiscardedEarly]++;
                    }
                    else
                    {
                        bool result = m_receiveQueue->Insert( ReceiveQueueEntry( message, messageId ) );

                        assert( result );
                    }

                    m_counters[MessagesRead]++;
                }

                if ( earlyMessage )
                    throw runtime_error( "received early message" );
            }
        }

        void ProcessAck( uint16_t ack )
        {
//            cout << "process ack: " << ack << endl;

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

//                      cout << "acked message " << messageId << endl;

                        sendQueueEntry->valid = 0;
                        sendQueueEntry->message = nullptr;
                    }
                }
            }
            else if ( m_sendLargeBlock.active && m_sendLargeBlock.blockId == sentPacket->blockId )
            {
                assert( sentPacket->fragmentId < m_sendLargeBlock.numFragments );

                auto & fragment = m_sendLargeBlock.fragments[sentPacket->fragmentId];

                if ( !fragment.acked )
                {
//                    cout << "acked fragment " << sentPacket->fragmentId << endl;

                    fragment.acked = true;
                    
                    m_sendLargeBlock.numAckedFragments++;

                    if ( m_sendLargeBlock.numAckedFragments == m_sendLargeBlock.numFragments )
                    {
//                        cout << "large block " << m_sendLargeBlock.blockId << " acked" << endl;

                        m_sendLargeBlock.active = false;

                        auto sendQueueEntry = m_sendQueue->Find( sentPacket->blockId );
                        assert( sendQueueEntry );
                        sendQueueEntry->valid = 0;
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

    private:

        ReliableMessageChannelConfig m_config;                              // constant configuration data

        int m_maxBlockFragments;                                            // maximum number of fragments per-block
        int m_messageOverheadBits;                                          // number of bits overhead per-serialized message

        TimeBase m_timeBase;                                                // current time base from last update
        uint16_t m_sendMessageId;                                           // id for next message added to send queue
        uint16_t m_receiveMessageId;                                        // id for next message to be received

        shared_ptr<SlidingWindow<SendQueueEntry>> m_sendQueue;              // message send queue
        shared_ptr<SlidingWindow<SentPacketEntry>> m_sentPackets;           // sent packets (for acks)
        shared_ptr<SlidingWindow<ReceiveQueueEntry>> m_receiveQueue;        // message receive queue

        vector<uint64_t> m_counters;                                        // counters used for unit testing and validation
        vector<uint8_t> m_measureBuffer;                                    // buffer used for measuring message size in bits

        SendLargeBlockData m_sendLargeBlock;                                // data for large block being sent
        ReceiveLargeBlockData m_receiveLargeBlock;                          // data for large block being received
    };
}

#endif
