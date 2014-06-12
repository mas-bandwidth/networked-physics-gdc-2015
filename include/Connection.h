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

    struct ConnectionPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint16_t sequence = 0;
        uint16_t ack = 0;
        uint32_t ack_bits = 0;
        vector<ChannelData*> channelData;
        ChannelStructure * channelStructure;

        ConnectionPacket( int type, ChannelStructure * _channelStructure ) : Packet( type )
        {
//            printf( "ConnectionPacket\n" );
            assert( _channelStructure );
            channelStructure = _channelStructure;
            channelData.resize( channelStructure->GetNumChannels() );
        }

        ~ConnectionPacket()
        {
//            printf( "~ConnectionPacket\n" );
            assert( channelStructure );     // double delete?
            channelStructure = nullptr;
            for ( int i = 0; i < channelData.size(); ++i )
            {
                if ( channelData[i] )
                {
                    delete channelData[i];
                    channelData[i] = nullptr;
                }
            }
            channelData.clear();
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
                for ( auto data : channelData )
                {
                    bool has_data = data != nullptr;
                    serialize_bool( stream, has_data );
                }
            }
            else                
            {
                for ( int i = 0; i < channelData.size(); ++i )
                {
                    bool has_data;
                    serialize_bool( stream, has_data );
                    if ( has_data )
                    {
                        channelData[i] = channelStructure->CreateChannelData( i );
                        assert( channelData[i] );

                        // todo: need a way to set an error indicating that serialization has failed    
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

            for ( int i = 0; i < channelData.size(); ++i )
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

    class Connection
    {
        const ConnectionConfig m_config;                    // const configuration data
        TimeBase m_timeBase;                                // network time base
        SentPackets * m_sentPackets = nullptr;              // sliding window of recently sent packets
        ReceivedPackets * m_receivedPackets = nullptr;      // sliding window of recently received packets
        vector<Channel*> m_channels;                        // array of channels created according to channel structure
        vector<uint64_t> m_counters;                        // counters for unit testing, stats etc.

    public:

        enum Counters
        {
            PacketsRead,                            // number of packets read
            PacketsWritten,                         // number of packets written
            PacketsAcked,                           // number of packets acked
            PacketsDiscarded,                       // number of read packets that we discarded (eg. not acked)
            NumCounters
        };

        Connection( const ConnectionConfig & config ) : m_config( config )
        {
            assert( config.packetFactory );
            assert( config.channelStructure );

            m_sentPackets = new SentPackets( m_config.slidingWindowSize );
            m_receivedPackets = new ReceivedPackets( m_config.slidingWindowSize );

            const int numChannels = config.channelStructure->GetNumChannels();
            m_channels.resize( numChannels );
            for ( int i = 0; i < numChannels; ++i )
            {
                m_channels[i] = config.channelStructure->CreateChannel( i );
                assert( m_channels[i] );
            }

            m_counters.resize( NumCounters, 0 );

            Reset();
        }

        ~Connection()
        {
            assert( m_sentPackets );
            assert( m_receivedPackets );

            delete m_sentPackets;
            delete m_receivedPackets;

            m_sentPackets = nullptr;
            m_receivedPackets = nullptr;
        }

        Channel * GetChannel( int index )
        {
            if ( index >= 0 && index < m_channels.size() )
                return m_channels[index];
            else
                return nullptr;
        }

        void Reset()
        {
            m_timeBase = TimeBase();
            m_sentPackets->Reset();
            m_receivedPackets->Reset();
            for ( auto channel : m_channels )
                channel->Reset();
            for ( int i = 0; i < m_counters.size(); ++i )
                m_counters[i] = 0;
        }

        void Update( const TimeBase & timeBase )
        {
            m_timeBase = timeBase;
            for ( auto channel : m_channels )
                channel->Update( timeBase );
        }

        const TimeBase & GetTimeBase() const
        {
            return m_timeBase;
        }

        ConnectionPacket * WritePacket()
        {
            auto packet = static_cast<ConnectionPacket*>( m_config.packetFactory->Create( m_config.packetType ) );

            packet->protocolId = m_config.protocolId;
            packet->sequence = m_sentPackets->GetSequence();

            GenerateAckBits( *m_receivedPackets, packet->ack, packet->ack_bits );

            for ( int i = 0; i < m_channels.size(); ++i )
                packet->channelData[i] = m_channels[i]->GetData( packet->sequence );

            m_sentPackets->Insert( packet->sequence );

            m_counters[PacketsWritten]++;

            return packet;
        }

        bool ReadPacket( ConnectionPacket * packet )
        {
            assert( packet );
            assert( packet->GetType() == m_config.packetType );
            assert( packet->channelData.size() == m_channels.size() );

            if ( packet->channelData.size() != m_channels.size() )
                return false;

            if ( packet->protocolId != m_config.protocolId )
                return false;

//            printf( "read packet %d\n", (int) packet->sequence );

            ProcessAcks( packet->ack, packet->ack_bits );

            m_counters[PacketsRead]++;

            bool discardPacket = false;

            for ( int i = 0; i < packet->channelData.size(); ++i )
            {
                if ( !packet->channelData[i] )
                    continue;

                auto result = m_channels[i]->ProcessData( packet->sequence, packet->channelData[i] );

                if ( !result )
                    discardPacket = true;
            }

            if ( discardPacket || !m_receivedPackets->Insert( packet->sequence ) )
            {
                m_counters[PacketsDiscarded]++;
                return false;            
            }

            return true;
        }

        uint64_t GetCounter( int index ) const
        {
            assert( index >= 0 );
            assert( index < NumCounters );
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

            m_counters[PacketsAcked]++;
            for ( auto channel : m_channels )
                channel->ProcessAck( sequence );
        }
    };
}

#endif
