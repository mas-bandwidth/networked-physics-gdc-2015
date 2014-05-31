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

    class ConnectionInterface
    {
    public:

        virtual int GetNumChannels() = 0;

        virtual shared_ptr<ChannelData> CreateChannelData( int index ) = 0;
    };

    struct ConnectionPacket : public Packet
    {
        uint16_t sequence;
        uint16_t ack;
        uint32_t ack_bits;
        shared_ptr<ConnectionInterface> interface;
        vector<shared_ptr<ChannelData>> channelData;

        ConnectionPacket( int type, shared_ptr<ConnectionInterface> _interface ) : Packet( type )
        {
            assert( _interface );
            sequence = 0;
            ack = 0;
            ack_bits = 0;
            interface = _interface;
            channelData.resize( interface->GetNumChannels() );
        }

        void Serialize( Stream & stream )
        {
            assert( interface );

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
                        channelData[i] = interface->CreateChannelData( i );
                        
                        if ( !channelData[i] )
                            throw runtime_error( format_string( "serialize read failed to create channel data [%d]", i ) );

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
        ConnectionConfig()
        {
            packetType = 0;
            maxPacketSize = 1024;
            slidingWindowSize = 256;
        }

        int packetType;
        int maxPacketSize;
        int slidingWindowSize;
        shared_ptr<Factory<Packet>> packetFactory;
    };

    class Connection
    {
        struct InterfaceImplementation : public ConnectionInterface
        {
            vector<shared_ptr<Channel>> channels;

            int GetNumChannels() 
            { 
                return channels.size();
            }

            shared_ptr<ChannelData> CreateChannelData( int index )
            {
                assert( index >= 0 );
                assert( index < channels.size() );
                if ( index >= 0 || index < channels.size() )
                    return channels[index]->CreateData();
                else
                    return nullptr;
            }
        };

        const ConnectionConfig m_config;                    // const configuration data
        TimeBase m_timeBase;                                // network time base
        shared_ptr<SentPackets> m_sentPackets;              // sliding window of recently sent packets
        shared_ptr<ReceivedPackets> m_receivedPackets;      // sliding window of recently received packets
        vector<uint64_t> m_counters;                        // counters for unit testing, stats etc.
        shared_ptr<InterfaceImplementation> m_interface;    // connection interface shared with connection packet.

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

        shared_ptr<ConnectionInterface> GetInterface()
        {
            assert( m_interface );
            return m_interface;
        }

        Connection( const ConnectionConfig & config )
            : m_config( config )
        {
            assert( config.packetFactory );         // IMPORTANT: You must supply a packet factory!

            m_sentPackets = make_shared<SentPackets>( m_config.slidingWindowSize );
            m_receivedPackets = make_shared<ReceivedPackets>( m_config.slidingWindowSize );
            m_counters.resize( NumCounters, 0 );
            m_interface = make_shared<InterfaceImplementation>();

            Reset();
        }

        void AddChannel( shared_ptr<Channel> channel )
        {
            assert( channel );
            m_interface->channels.push_back( channel );
        }

        shared_ptr<Channel> GetChannel( int index )
        {
            if ( index >= 0 && index < m_interface->channels.size() )
                return m_interface->channels[index];
            else
                return nullptr;
        }

        void Reset()
        {
            m_timeBase = TimeBase();
            m_sentPackets->Reset();
            m_receivedPackets->Reset();
            for ( int i = 0; i < m_counters.size(); ++i )
                m_counters[i] = 0;
        }

        void Update( const TimeBase & timeBase )
        {
            m_timeBase = timeBase;
            for ( auto channel : m_interface->channels )
                channel->Update( timeBase );
        }

        const TimeBase & GetTimeBase() const
        {
            return m_timeBase;
        }

        shared_ptr<ConnectionPacket> WritePacket()
        {
            auto packet = static_pointer_cast<ConnectionPacket>( m_config.packetFactory->Create( m_config.packetType ) );
            packet->sequence = m_sentPackets->GetSequence();

            GenerateAckBits( *m_receivedPackets, packet->ack, packet->ack_bits );

            for ( int i = 0; i < m_interface->channels.size(); ++i )
                packet->channelData[i] = m_interface->channels[i]->GetData( packet->sequence );

            m_sentPackets->Insert( packet->sequence );

            m_counters[PacketsWritten]++;

            return packet;
        }

        void ReadPacket( shared_ptr<ConnectionPacket> packet )
        {
            assert( packet );
            assert( packet->GetType() == m_config.packetType );
            assert( packet->channelData.size() == m_interface->channels.size() );

//            cout << "read packet " << packet->sequence << endl;

            m_counters[PacketsRead]++;

            try
            {
                for ( int i = 0; i < packet->channelData.size(); ++i )
                {
                    if ( packet->channelData[i] )
                        m_interface->channels[i]->ProcessData( packet->sequence, packet->channelData[i] );
                }
            }
            catch ( runtime_error & e )
            {
                // todo: rename this counter
//                cout << "read packet failure" << endl;
                m_counters[ReadPacketFailures]++;
                return;                
            }

            if ( !m_receivedPackets->Insert( packet->sequence ) )
            {
                m_counters[PacketsDiscarded]++;
                return;
            }

            ProcessAcks( packet->ack, packet->ack_bits );
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
            for ( auto channel : m_interface->channels )
                channel->ProcessAck( sequence );
        }
    };
}

#endif
