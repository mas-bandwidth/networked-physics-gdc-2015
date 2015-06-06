// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef PROTOCOL_RELIABLE_MESSAGE_CHANNEL_H
#define PROTOCOL_RELIABLE_MESSAGE_CHANNEL_H

#include "Message.h"
#include "BlockMessage.h"
#include "MessageFactory.h"
#include "MessageChannel.h"
#include "SequenceBuffer.h"
#include <math.h>

namespace protocol
{
    struct ReliableMessageChannelConfig
    {
        ReliableMessageChannelConfig()
        {
            allocator = nullptr;
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

        core::Allocator * allocator;    // allocator used for allocations matching life cycle of this object. if null falls back to default allocator.

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

        MessageFactory * messageFactory = nullptr;

        core::Allocator * messageAllocator = nullptr;
        core::Allocator * smallBlockAllocator = nullptr;
        core::Allocator * largeBlockAllocator = nullptr;
    };

    class ReliableMessageChannelData : public ChannelData
    {
        ReliableMessageChannelData( const ReliableMessageChannelData & other );
        ReliableMessageChannelData & operator = ( const ReliableMessageChannelData & other );

    public:

        const ReliableMessageChannelConfig & config;

        Message ** messages = nullptr;          // array of messages.
        uint8_t * fragment = nullptr;           // the  fragment data. only valid if sending large block.
        uint64_t numMessages : 16;              // number of messages in array.
        uint64_t fragmentId : 16;               // fragment id. valid if sending large block.
        uint64_t blockSize : 32;                // block size in bytes. valid if sending large block.
        uint64_t blockId : 16;                  // block id. valid if sending large block.
        uint64_t largeBlock : 1;                // true if currently sending a large block.
       
        ReliableMessageChannelData( const ReliableMessageChannelConfig & _config );

        ~ReliableMessageChannelData();

        template <typename Stream> void Serialize( Stream & stream );

        void SerializeRead( ReadStream & stream );

        void SerializeWrite( WriteStream & stream );

        void SerializeMeasure( MeasureStream & stream );

        void DisconnectMessages();
    };

    class ReliableMessageChannel : public MessageChannel
    {
        struct SendQueueEntry
        {
            Message * message;
            double timeLastSent;
            uint32_t largeBlock : 1;
            uint32_t measuredBits : 30;
        };

        struct SentPacketEntry
        {
            double timeSent;
            uint16_t * messageIds;
            uint64_t numMessageIds : 16;                 // number of messages in this packet
            uint64_t blockId : 16;                       // block id. valid only when sending large block.
            uint64_t fragmentId : 16;                    // fragment id. valid only when sending large block.
            uint64_t acked : 1;                          // 1 if this sent packet has been acked
            uint64_t largeBlock : 1;                     // 1 if this sent packet contains a large block fragment
        };

        struct ReceiveQueueEntry
        {
            Message * message;
        };

        struct SendLargeBlockData
        {
            SendLargeBlockData()
            {
                acked_fragment = nullptr;
                time_fragment_last_sent = nullptr;
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
            BitArray * acked_fragment;                  // has fragment n been received?
            double * time_fragment_last_sent;           // time fragment last sent in seconds.
        };

        struct ReceiveLargeBlockData
        {
            ReceiveLargeBlockData()
            {
                received_fragment = nullptr;
                Reset();
            }

            void Reset()
            {
                active = false;
                numFragments = 0;
                numReceivedFragments = 0;
                blockId = 0;
                blockSize = 0;
                block.Destroy();
            }

            bool active;                                // true if we are currently receiving a large block
            int numFragments;                           // number of fragments in this block
            int numReceivedFragments;                   // number of fragments received.
            uint16_t blockId;                           // block id being currently received.
            uint32_t blockSize;                         // block size in bytes.
            BitArray * received_fragment;               // has fragment n been received?
            Block block;                                // the block being received.
        };

    public:

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

        core::Allocator * m_allocator = nullptr;                            // allocator for allocations matching life cycle of object.

        int m_error = 0;                                                    // current error state. set to non-zero if an error occurs.

        int m_maxBlockFragments;                                            // maximum number of fragments per-block
        int m_messageOverheadBits;                                          // number of bits overhead per-serialized message

        core::TimeBase m_timeBase;                                          // current time base from last update
        uint16_t m_sendMessageId;                                           // id for next message added to send queue
        uint16_t m_receiveMessageId;                                        // id for next message to be received
        uint16_t m_oldestUnackedMessageId;                                  // id for oldest unacked message in send queue

        SequenceBuffer<SendQueueEntry> * m_sendQueue;                        // message send queue
        SequenceBuffer<SentPacketEntry> * m_sentPackets;                     // sent packets (for acks)
        SequenceBuffer<ReceiveQueueEntry> * m_receiveQueue;                  // message receive queue

        SendLargeBlockData m_sendLargeBlock;                                // data for large block being sent
        ReceiveLargeBlockData m_receiveLargeBlock;                          // data for large block being received

        uint16_t * m_sentPacketMessageIds;                                  // array of message ids, n ids per-sent packet

        uint64_t m_counters[RELIABLE_MESSAGE_CHANNEL_COUNTER_NUM_COUNTERS]; // counters used for unit testing and validation

        ReliableMessageChannel( const ReliableMessageChannel & other );
        ReliableMessageChannel & operator = ( const ReliableMessageChannel & other );

    public:

        ReliableMessageChannel( const ReliableMessageChannelConfig & config );

        ~ReliableMessageChannel();

        void Reset();

        bool CanSendMessage() const;

        void SendMessage( Message * message );

        void SendBlock( Block & block );

        Message * ReceiveMessage();

        int GetError() const;

        ChannelData * CreateData();

        ChannelData * GetData( uint16_t sequence );

        bool ProcessData( uint16_t sequence, ChannelData * channelData );

        void UpdateOldestUnackedMessageId();

        void ProcessAck( uint16_t ack );

        void Update( const core::TimeBase & timeBase );

        uint64_t GetCounter( int index ) const;

        SendLargeBlockStatus GetSendLargeBlockStatus() const;

        ReceiveLargeBlockStatus GetReceiveLargeBlockStatus() const;
    };
}

#endif
