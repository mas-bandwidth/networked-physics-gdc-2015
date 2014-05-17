/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_MESSAGE_CHANNEL_H
#define PROTOCOL_MESSAGE_CHANNEL_H

#include "Common.h"
#include "Channel.h"
#include "Message.h"

namespace protocol
{
    struct MessageChannelConfig
    {
        MessageChannelConfig()
        {
            resendRate = 0.1f;
            sendQueueSize = 1024;
            receiveQueueSize = 1024;
            sentPacketsSize = 256;
            maxMessagesPerPacket = 32;
        }

        float resendRate;
        int sendQueueSize;
        int receiveQueueSize;
        int maxMessagesPerPacket;
        int sentPacketsSize;
        shared_ptr<Factory<Message>> messageFactory;
    };

    class MessageChannelData : public ChannelData
    {
        const MessageChannelConfig * config;

    public:

        MessageChannelData( const MessageChannelConfig & _config ) : config( &_config ) {}

        vector<shared_ptr<Message>> messages;

        void Serialize( Stream & stream )
        {
            assert( config );
            assert( config->messageFactory );           // IMPORTANT: You must supply a message factory!

            int numMessages = stream.IsWriting() ? messages.size() : 0;
            serialize_int( stream, numMessages, 0, config->maxMessagesPerPacket );
            if ( stream.IsReading() )
                messages.resize( numMessages );

            for ( int i = 0; i < numMessages; ++i )
            {
                #ifdef _DEBUG
                if ( stream.IsWriting() )
                    assert( messages[i] );
                #endif

                int messageType = stream.IsWriting() ? messages[i]->GetType() : 0;
                serialize_int( stream, messageType, 0, config->messageFactory->GetMaxType() );
                if ( stream.IsReading() )
                {
                    messages[i] = config->messageFactory->Create( messageType );
                    assert( messages[i] );
                    assert( messages[i]->GetType() == messageType );
                }

                uint16_t messageId = stream.IsWriting() ? messages[i]->GetId() : 0;
                serialize_int( stream, messageId, 0, 65535 );
                if ( stream.IsReading() )
                {
                    messages[i]->SetId( messageId );
                    assert( messages[i]->GetId() == messageId );
                }

                messages[i]->Serialize( stream );
            }
        }
    };

    class MessageChannel : public Channel
    {
        struct SendQueueEntry
        {
            shared_ptr<Message> message;
            uint32_t sequence : 16;                      // this is actually the message id      
            uint32_t valid : 1;
            double timeLastSent;
            SendQueueEntry()
                : valid(0) {}
            SendQueueEntry( shared_ptr<Message> _message, uint16_t _sequence )
                : message( _message ), sequence( _sequence ), valid(1), timeLastSent(0) {}
        };

        struct SentPacketEntry
        {
            uint32_t sequence : 16;                      // this is the packet sequence #
            uint32_t acked : 1;
            uint32_t valid : 1;
            double timeSent;
            vector<uint16_t> messageIds;
            SentPacketEntry() : valid(0) {}
        };

        struct ReceiveQueueEntry
        {
            shared_ptr<Message> message;
            uint32_t sequence : 16;                      // this is actually the message id      
            uint32_t valid : 1;
            double timeReceived;
            ReceiveQueueEntry()
                : valid(0) {}
            ReceiveQueueEntry( shared_ptr<Message> _message, uint16_t _sequence, double _timeReceived )
                : message( _message ), sequence( _sequence ), valid(1), timeReceived( _timeReceived ) {}
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

        MessageChannel( const MessageChannelConfig & config ) : m_config( config )
        {
            assert( config.messageFactory );       // IMPORTANT: You must supply a message factory!

            m_sendMessageId = 0;
            m_receiveMessageId = 0;

            m_sendQueue = make_shared<SlidingWindow<SendQueueEntry>>( m_config.sendQueueSize );
            m_sentPackets = make_shared<SlidingWindow<SentPacketEntry>>( m_config.sentPacketsSize );
            m_receiveQueue = make_shared<SlidingWindow<ReceiveQueueEntry>>( m_config.receiveQueueSize );

            m_counters.resize( NumCounters, 0 );
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

//            cout << "message id is " << message->GetId() << endl;

            m_sendQueue->Insert( SendQueueEntry( message, m_sendMessageId ) );

            #ifdef _DEBUG
            auto entry = m_sendQueue->Find( m_sendMessageId );
            assert( entry );
            assert( entry->valid );
            assert( entry->sequence == m_sendMessageId );
            assert( entry->message );
            assert( entry->message->GetId() == m_sendMessageId );
            #endif

            m_counters[MessagesSent]++;

            m_sendMessageId++;
        }

        shared_ptr<Message> ReceiveMessage()
        {
            auto entry = m_receiveQueue->Find( m_receiveMessageId );
            if ( !entry )
                return nullptr;

            m_counters[MessagesReceived]++;
            m_receiveMessageId++;
            
            auto message = entry->message;

            #ifdef _DEBUG
            assert( message );
            assert( message->GetId() == m_receiveMessageId );
            #endif

//            cout << "dequeue for receive: " << message->GetId() << endl;

            entry->valid = 0;
            entry->message = nullptr;
            
            return message;
        }

        shared_ptr<ChannelData> CreateData()
        {
            return make_shared<MessageChannelData>( m_config );
        }

        shared_ptr<ChannelData> GetData( uint16_t sequence )
        {
            // first gather messages to include in the packet

            int numMessageIds = 0;
            uint16_t messageIds[m_config.maxMessagesPerPacket];
            const uint16_t baseId = m_sendMessageId - m_config.sendQueueSize + 1;
            for ( int i = 0; i < m_config.sendQueueSize; ++i )
            {
                uint16_t id = ( baseId + i ) % m_config.sendQueueSize;
                SendQueueEntry * entry = m_sendQueue->Find( id );
                if ( entry && entry->timeLastSent + m_config.resendRate < m_timeBase.time )
                {
                    messageIds[numMessageIds++] = id;
                    entry->timeLastSent = m_timeBase.time;
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
            sentPacketData->acked = 0;
            sentPacketData->timeSent = m_timeBase.time;
            sentPacketData->messageIds.resize( numMessageIds );
            for ( int i = 0; i < numMessageIds; ++i )
                sentPacketData->messageIds[i] = messageIds[i];

            // update counter: nem messages written

            m_counters[MessagesWritten] += numMessageIds;

            // construct channel data for packet

            auto data = make_shared<MessageChannelData>( m_config );

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

        void ProcessData( uint16_t sequence, shared_ptr<ChannelData> channelData )
        {
            assert( channelData );

            auto data = reinterpret_cast<MessageChannelData&>( *channelData );

//            cout << "process message channel data: " << sequence << endl;

            bool earlyMessage = false;

            const uint16_t minMessageId = m_receiveMessageId;
            const uint16_t maxMessageId = m_receiveMessageId + m_config.receiveQueueSize - 1;

            // process messages included in this packet data

//            cout << data.messages.size() << " messages in packet" << endl;

            for ( auto message : data.messages )
            {
                assert( message );

                const uint16_t messageId = message->GetId();

                // todo: likely the message id is wrong here somehow --- yes, it is zero when it should be valid.

//                cout << "add message to receive queue: " << messageId << endl;

                if ( sequence_less_than( messageId, minMessageId ) )
                {
                    // If messages are older than the last received message id we can
                    // ignore them. They have already been queued up and delivered to
                    // the application included via some previously processed packet.

                    m_counters[MessagesDiscardedLate]++;
                }
                else if ( sequence_greater_than( messageId, maxMessageId ) )
                {
                    // If a message arrives that is *newer* than what the receive queue 
                    // can buffer we must discard this packet. Otherwise, early messages
                    // that we cannot buffer will be false acked and never resent.

                    earlyMessage = true;

                    m_counters[MessagesDiscardedEarly]++;
                }
                else
                {
                    bool result = m_receiveQueue->Insert( ReceiveQueueEntry( message, messageId, m_timeBase.time ) );
                    assert( result );
                }

                m_counters[MessagesRead]++;
            }

            if ( earlyMessage )
                throw runtime_error( "received early message" );
        }

        void ProcessAck( uint16_t ack )
        {
//            cout << "process ack: " << ack << endl;

            auto sentPacket = m_sentPackets->Find( ack );
            if ( !sentPacket || sentPacket->acked )
                return;

            for ( auto messageId : sentPacket->messageIds )
            {
                auto sendQueueEntry = m_sendQueue->Find( messageId );
                if ( sendQueueEntry )
                {
                    assert( sendQueueEntry->message );
                    assert( sendQueueEntry->message->GetId() == messageId );

//                    cout << "acked message " << messageId << endl;

                    sendQueueEntry->valid = 0;
                    sendQueueEntry->message = nullptr;
                }
            }

            sentPacket->acked = 1;
        }

        void Update( const TimeBase & timeBase )
        {
            m_timeBase = timeBase;
        }

    private:

        MessageChannelConfig m_config;                                      // constant configuration data
        TimeBase m_timeBase;                                                // current time base from last update
        uint16_t m_sendMessageId;                                           // id for next message added to send queue
        uint16_t m_receiveMessageId;                                        // id for next message to be received
        shared_ptr<SlidingWindow<SendQueueEntry>> m_sendQueue;              // message send queue
        shared_ptr<SlidingWindow<SentPacketEntry>> m_sentPackets;           // sent packets (for acks)
        shared_ptr<SlidingWindow<ReceiveQueueEntry>> m_receiveQueue;        // message receive queue
        vector<uint64_t> m_counters;                                        // counters
    };

}

#endif
