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
        vector<shared_ptr<ChannelData>> channelData;
        shared_ptr<ChannelStructure> channelStructure;

        ConnectionPacket( int type, shared_ptr<ChannelStructure> _channelStructure ) : Packet( type )
        {
            assert( _channelStructure );
            channelStructure = _channelStructure;
            channelData.resize( channelStructure->GetNumChannels() );
        }

        void Serialize( Stream & stream )
        {
            assert( channelStructure );

            serialize_uint64( stream, protocolId );

            serialize_bits( stream, sequence, 16 );
            serialize_bits( stream, ack, 16 );

            bool perfect = ack_bits == 0xFFFFFFFF;
            serialize_bool( stream, perfect );
            if ( !perfect )
                serialize_bits( stream, ack_bits, 32 );
            else
                ack_bits = 0xFFFFFFFF;

            if ( stream.IsWriting() )
            {
                for ( auto data : channelData )
                {
                    bool has_data = data != nullptr;
                    serialize_bool( stream, has_data );
                    if ( data )
                        data->Serialize( stream );
                }
            }
            else                
            {
                for ( int i = 0; i < channelData.size(); ++i )
                {
                    bool has_data = 0;
                    serialize_bool( stream, has_data );
                    if ( has_data )
                    {
                        channelData[i] = channelStructure->CreateChannelData( i );
                        
                        if ( !channelData[i] )
                        {
                            cout << "failed to create channel data?!" << endl;
                            throw runtime_error( format_string( "serialize read failed to create channel data [%d]", i ) );
                        }

                        channelData[i]->Serialize( stream );
                    }
                }
            }
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
        shared_ptr<Factory<Packet>> packetFactory;
        shared_ptr<ChannelStructure> channelStructure;
    };

    class Connection
    {
        const ConnectionConfig m_config;                    // const configuration data
        TimeBase m_timeBase;                                // network time base
        shared_ptr<SentPackets> m_sentPackets;              // sliding window of recently sent packets
        shared_ptr<ReceivedPackets> m_receivedPackets;      // sliding window of recently received packets
        vector<shared_ptr<Channel>> m_channels;             // array of channels created according to channel structure
        vector<uint64_t> m_counters;                        // counters for unit testing, stats etc.

    public:

        enum Counters
        {
            PacketsRead,                            // number of packets read
            PacketsWritten,                         // number of packets written
            PacketsDiscarded,                       // number of packets discarded
            PacketsAcked,                           // number of packets acked
            ReadPacketFailures,                     // number of read packet failures
            NumCounters
        };

        Connection( const ConnectionConfig & config ) : m_config( config )
        {
            assert( config.packetFactory );
            assert( config.channelStructure );

            m_sentPackets = make_shared<SentPackets>( m_config.slidingWindowSize );
 
            m_receivedPackets = make_shared<ReceivedPackets>( m_config.slidingWindowSize );

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

        shared_ptr<Channel> GetChannel( int index )
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

        shared_ptr<ConnectionPacket> WritePacket()
        {
            auto packet = static_pointer_cast<ConnectionPacket>( m_config.packetFactory->Create( m_config.packetType ) );

            packet->protocolId = m_config.protocolId;
            packet->sequence = m_sentPackets->GetSequence();

            GenerateAckBits( *m_receivedPackets, packet->ack, packet->ack_bits );

            for ( int i = 0; i < m_channels.size(); ++i )
                packet->channelData[i] = m_channels[i]->GetData( packet->sequence );

            m_sentPackets->Insert( packet->sequence );

            m_counters[PacketsWritten]++;

            return packet;
        }

        bool ReadPacket( shared_ptr<ConnectionPacket> packet )
        {
            assert( packet );
            assert( packet->GetType() == m_config.packetType );
            assert( packet->channelData.size() == m_channels.size() );

            if ( packet->protocolId != m_config.protocolId )
                return false;

//            cout << "read packet " << packet->sequence << endl;

            m_counters[PacketsRead]++;

            try
            {
                for ( int i = 0; i < packet->channelData.size(); ++i )
                {
                    if ( packet->channelData[i] )
                        m_channels[i]->ProcessData( packet->sequence, packet->channelData[i] );
                }
            }
            catch ( runtime_error & e )
            {
                // todo: rename this counter
//                cout << "read packet failure" << endl;
                m_counters[ReadPacketFailures]++;
                return false;            
            }

            if ( !m_receivedPackets->Insert( packet->sequence ) )
            {
                m_counters[PacketsDiscarded]++;
                return false;
            }

            ProcessAcks( packet->ack, packet->ack_bits );

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
//            cout << format_string( "process acks: %d - %x", (int)ack, ack_bits ) << endl;

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
//            cout << "packet " << sequence << " acked" << endl;

            m_counters[PacketsAcked]++;
            for ( auto channel : m_channels )
                channel->ProcessAck( sequence );
        }
    };
}

#endif
