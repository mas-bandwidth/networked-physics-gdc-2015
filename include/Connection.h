/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_CONNECTION_H
#define PROTOCOL_CONNECTION_H

#include "Common.h"
#include "Stream.h"
#include "Packet.h"
#include "Channel.h"

namespace protocol
{
    struct ConnectionPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint16_t sequence = 0;
        uint16_t ack = 0;
        uint32_t ack_bits = 0;
        int numChannels = 0;
        ChannelData * channelData[MaxChannels];
        ChannelStructure * channelStructure = nullptr;

        ConnectionPacket( int type, ChannelStructure * _channelStructure ) : Packet( type )
        {
//            printf( "ConnectionPacket\n" );
            assert( _channelStructure );
            channelStructure = _channelStructure;
            numChannels = channelStructure->GetNumChannels();
            memset( channelData, 0, sizeof( ChannelData* ) * numChannels );
        }

        ~ConnectionPacket()
        {
//            printf( "~ConnectionPacket\n" );

            assert( channelStructure );

            for ( int i = 0; i < numChannels; ++i )
            {
                if ( channelData[i] )
                {
//                    printf( "connection packet delete channel data\n" );
                    delete channelData[i];
                    channelData[i] = nullptr;
                }
            }

            channelStructure = nullptr;
        }

        template <typename Stream> void Serialize( Stream & stream )
        {
            assert( channelStructure );

            // IMPORTANT: Insert non-frequently changing values here
            // This helps LZ dictionary based compressors do a good job!

            serialize_uint64( stream, protocolId );

            bool perfect;
            if ( Stream::IsWriting )
                 perfect = ack_bits == 0xFFFFFFFF;

            serialize_bool( stream, perfect );

            if ( !perfect )
                serialize_bits( stream, ack_bits, 32 );
            else
                ack_bits = 0xFFFFFFFF;

            stream.Align();

            if ( Stream::IsWriting )
            {
                for ( int i = 0; i < numChannels; ++i )
                {
                    bool has_data = channelData[i] != nullptr;
                    serialize_bool( stream, has_data );
                }
            }
            else                
            {
                for ( int i = 0; i < numChannels; ++i )
                {
                    bool has_data;
                    serialize_bool( stream, has_data );
                    if ( has_data )
                    {
                        channelData[i] = channelStructure->CreateChannelData( i );
                        assert( channelData[i] );
                    }
                }
            }

            // IMPORTANT: Insert frequently changing values below

            serialize_bits( stream, sequence, 16 );

            int ack_delta = 0;
            bool ack_in_range = false;

            if ( Stream::IsWriting )
            {
                if ( ack < sequence )
                    ack_delta = sequence - ack;
                else
                    ack_delta = (int)sequence + 65536 - ack;

                assert( ack_delta > 0 );
                
                ack_in_range = ack_delta <= 128;
            }

            serialize_bool( stream, ack_in_range );
    
            if ( ack_in_range )
            {
                serialize_int( stream, ack_delta, 1, 128 );
                if ( Stream::IsReading )
                    ack = sequence - ack_delta;
            }
            else
                serialize_bits( stream, ack, 16 );

            // now serialize per-channel data

            for ( int i = 0; i < numChannels; ++i )
            {
                if ( channelData[i] )
                {
                    stream.Align();

                    serialize_object( stream, *channelData[i] );
                }
            }
        }

        void SerializeRead( ReadStream & stream )
        {
            return Serialize( stream );
        }

        void SerializeWrite( WriteStream & stream )
        {
            return Serialize( stream );
        }

        void SerializeMeasure( MeasureStream & stream )
        {
            return Serialize( stream );
        }

        bool operator ==( const ConnectionPacket & other ) const
        {
            return sequence == other.sequence &&
                        ack == other.ack &&
                   ack_bits == other.ack_bits;
        }

        bool operator !=( const ConnectionPacket & other ) const
        {
            return !( *this == other );
        }

    private:

        ConnectionPacket( const ConnectionPacket & other );
        const ConnectionPacket & operator = ( const ConnectionPacket & other );

    };

    struct ConnectionConfig
    {
        uint64_t protocolId = 0;
        int packetType = 0;
        int maxPacketSize = 1024;
        int slidingWindowSize = 256;
        Factory<Packet> * packetFactory = nullptr;
        ChannelStructure * channelStructure = nullptr;
    };

    struct SentPacketData
    {
        SentPacketData()
            : valid(0), acked(0), sequence(0) {}

        SentPacketData( uint16_t _sequence )
            : valid(1), acked(0), sequence( _sequence ) {}

        uint32_t valid : 1;                     // is this packet entry valid?
        uint32_t acked : 1;                     // has packet been acked?
        uint32_t sequence : 16;                 // packet sequence #
    };

    struct ReceivedPacketData
    {
        ReceivedPacketData()
            : valid(0), sequence(0) {}

        ReceivedPacketData( uint16_t _sequence )
            : valid(1), sequence( _sequence ) {}

        uint32_t valid : 1;                     // is this packet entry valid?
        uint32_t sequence : 16;                 // packet sequence #
    };

    typedef SlidingWindow<SentPacketData> SentPackets;
    typedef SlidingWindow<ReceivedPacketData> ReceivedPackets;

    class Connection
    {
        ConnectionError m_error = CONNECTION_ERROR_NONE;

        const ConnectionConfig m_config;                            // const configuration data
        TimeBase m_timeBase;                                        // network time base
        SentPackets * m_sentPackets = nullptr;                      // sliding window of recently sent packets
        ReceivedPackets * m_receivedPackets = nullptr;              // sliding window of recently received packets
        int m_numChannels = 0;                                      // cached number of channels
        Channel * m_channels[MaxChannels];                          // array of channels created according to channel structure
        uint64_t m_counters[CONNECTION_COUNTER_NUM_COUNTERS];       // counters for unit testing, stats etc.

    public:

        Connection( const ConnectionConfig & config ) : m_config( config )
        {
            assert( config.packetFactory );
            assert( config.channelStructure );

            m_sentPackets = new SentPackets( m_config.slidingWindowSize );
            m_receivedPackets = new ReceivedPackets( m_config.slidingWindowSize );

            m_numChannels = config.channelStructure->GetNumChannels();
            for ( int i = 0; i < m_numChannels; ++i )
            {
                m_channels[i] = config.channelStructure->CreateChannel( i );
                assert( m_channels[i] );
            }

            Reset();
        }

        ~Connection()
        {
            assert( m_sentPackets );
            assert( m_receivedPackets );
            assert( m_channels );

            for ( int i = 0; i < m_numChannels; ++i )
            {
                assert( m_channels[i] );
                delete m_channels[i];
            }

            delete m_sentPackets;
            delete m_receivedPackets;

            m_sentPackets = nullptr;
            m_receivedPackets = nullptr;
        }

        Channel * GetChannel( int index )
        {
            assert( index >= 0 );
            assert( index < m_numChannels );
            return m_channels[index];
        }

        void Reset()
        {
            m_error = CONNECTION_ERROR_NONE;

            m_timeBase = TimeBase();

            m_sentPackets->Reset();
            m_receivedPackets->Reset();

            for ( int i = 0; i < m_numChannels; ++i )
                m_channels[i]->Reset();

            memset( m_counters, 0, sizeof( m_counters ) );
        }

        void Update( const TimeBase & timeBase )
        {
            if ( m_error != CONNECTION_ERROR_NONE )
                return;

            m_timeBase = timeBase;

            for ( int i = 0; i < m_numChannels; ++i )
            {
                m_channels[i]->Update( timeBase );

                if ( m_channels[i]->GetError() != 0 )
                {
                    m_error = CONNECTION_ERROR_CHANNEL;
                    return;
                }
            }
        }

        ConnectionError GetError() const
        {
            return m_error;
        }

        int GetChannelError( int channelIndex ) const
        {
            assert( channelIndex >= 0 );
            assert( channelIndex < m_numChannels );
            return m_channels[channelIndex]->GetError();
        }

        const TimeBase & GetTimeBase() const
        {
            return m_timeBase;
        }

        ConnectionPacket * WritePacket()
        {
            if ( m_error != CONNECTION_ERROR_NONE )
                return nullptr;

            auto packet = static_cast<ConnectionPacket*>( m_config.packetFactory->Create( m_config.packetType ) );

            packet->protocolId = m_config.protocolId;
            packet->sequence = m_sentPackets->GetSequence();

            GenerateAckBits( *m_receivedPackets, packet->ack, packet->ack_bits );

            for ( int i = 0; i < m_numChannels; ++i )
                packet->channelData[i] = m_channels[i]->GetData( packet->sequence );

            m_sentPackets->Insert( packet->sequence );

            m_counters[CONNECTION_COUNTER_PACKETS_WRITTEN]++;

            return packet;
        }

        bool ReadPacket( ConnectionPacket * packet )
        {
            if ( m_error != CONNECTION_ERROR_NONE )
                return false;

            assert( packet );
            assert( packet->GetType() == m_config.packetType );
            assert( packet->numChannels == m_numChannels );

            if ( packet->numChannels != m_numChannels )
                return false;

            if ( packet->protocolId != m_config.protocolId )
                return false;

//            printf( "read packet %d\n", (int) packet->sequence );

            ProcessAcks( packet->ack, packet->ack_bits );

            m_counters[CONNECTION_COUNTER_PACKETS_READ]++;

            bool discardPacket = false;

            for ( int i = 0; i < packet->numChannels; ++i )
            {
                if ( !packet->channelData[i] )
                    continue;

                auto result = m_channels[i]->ProcessData( packet->sequence, packet->channelData[i] );

                delete packet->channelData[i];
                packet->channelData[i] = nullptr;

                if ( !result )
                    discardPacket = true;
            }

            if ( discardPacket || !m_receivedPackets->Insert( packet->sequence ) )
            {
                m_counters[CONNECTION_COUNTER_PACKETS_DISCARDED]++;
                return false;            
            }

            return true;
        }

        uint64_t GetCounter( int index ) const
        {
            assert( index >= 0 );
            assert( index < CONNECTION_COUNTER_NUM_COUNTERS );
            return m_counters[index];
        }

    private:

        void ProcessAcks( uint16_t ack, uint32_t ack_bits )
        {
//            printf( "process acks: %d - %x\n", (int)ack, ack_bits );

            for ( int i = 0; i < 32; ++i )
            {
                if ( ack_bits & 1 )
                {                    
                    const uint16_t sequence = ack - i;
                    SentPacketData * packetData = m_sentPackets->Find( sequence );
                    if ( packetData && !packetData->acked )
                    {
                        PacketAcked( sequence );
                        packetData->acked = 1;
                    }
                }
                ack_bits >>= 1;
            }
        }

        void PacketAcked( uint16_t sequence )
        {
//            printf( "packet %d acked\n", (int) sequence );

            m_counters[CONNECTION_COUNTER_PACKETS_ACKED]++;

            for ( int i = 0; i < m_numChannels; ++i )
                m_channels[i]->ProcessAck( sequence );
        }
    };
}

#endif
