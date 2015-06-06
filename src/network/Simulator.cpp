// Network Library - Copyright (c) 2008-2015, Glenn Fiedler

#include "network/Simulator.h"
#include "core/Memory.h"
#include "protocol/PacketFactory.h"

namespace network
{
    Simulator::Simulator( const SimulatorConfig & config ) 
        : m_config( config ), m_bandwidthSlidingWindow( *config.allocator, config.bandwidthSize )
    {
        CORE_ASSERT( m_config.allocator );
        CORE_ASSERT( m_config.numPackets > 0 );
        CORE_ASSERT( m_config.packetFactory );

        m_packets = CORE_NEW_ARRAY( *m_config.allocator, PacketData, config.numPackets );

        m_packetNumberSend = 0;
        m_packetNumberReceive = 0;

        m_tcpMode = false;
        m_bandwidthExclude = false;

        m_bandwidth = 0.0f;

        m_numStates = 0;

        m_context = nullptr;
    }

    Simulator::~Simulator()
    {
        CORE_ASSERT( m_config.allocator );
        CORE_ASSERT( m_packets );

        Reset();

        CORE_DELETE_ARRAY( *m_config.allocator, m_packets, m_config.numPackets );

        m_packets = nullptr;
    }

    void Simulator::Reset()
    {
        CORE_ASSERT( m_packets );

        m_packetNumberSend = 0;
        m_packetNumberReceive = 0;

        for ( int i = 0; i < m_config.numPackets; ++i )
        {
            if ( m_packets[i].packet )
            {
                m_config.packetFactory->Destroy( m_packets[i].packet );
                m_packets[i].packet = nullptr;
            }
        }
    }

    int Simulator::AddState( const SimulatorState & state )
    {
        CORE_ASSERT( m_numStates < MaxSimulatorStates - 1 );
        const int index = m_numStates;
        m_states[m_numStates++] = state;
        if ( m_numStates == 1 )
            m_state = m_states[0];
        return index;
    }

    void Simulator::ClearStates()
    {
        m_state = SimulatorState();
        m_numStates = 0;
    }

    void Simulator::SendPacket( const Address & address, protocol::Packet * packet )
    {
        CORE_ASSERT( packet );

        const int index = m_packetNumberSend % m_config.numPackets;

        const bool loss = core::random_float( 0.0f, 100.0f ) <= m_state.packetLoss;

        const float jitter = core::random_float( -m_state.jitter, +m_state.jitter );

        if ( m_config.serializePackets )
        {
            BandwidthEntry entry;
            entry.time = m_timeBase.time;
            packet = SerializePacket( packet, entry.packetSize );
            if ( !m_bandwidthExclude )
            {
                if ( m_bandwidthSlidingWindow.IsFull() )
                    m_bandwidthSlidingWindow.Ack( m_bandwidthSlidingWindow.GetAck() + 1 );
                m_bandwidthSlidingWindow.Insert( entry );
            }
        }

        if ( m_tcpMode )
        {
            // TCP mode. Don't drop packets on send. TCP-like behavior is simulated on receive
            // by only dequeing the next expected packet and blocking until it is ready.
            // RTT * 2 latency is added to "lost" packets to simulate TCP retransmit.

            const float delay = m_state.latency + jitter + ( loss ? ( 4.0f * m_state.latency ) : 0.0f );

            CORE_ASSERT( m_packets[index].packet == nullptr );      // In TCP mode we cannot drop any packets!

            m_packets[index].packet = packet;
            m_packets[index].packetNumber = m_packetNumberSend;
            m_packets[index].dequeueTime = m_timeBase.time + delay;
            
            packet->SetAddress( address );

            m_packetNumberSend++;
        }
        else
        {
            // UDP mode. drop packets on send. randomly delay time of packet delivery to simulate latency and jitter.

            if ( loss )
            {
                m_config.packetFactory->Destroy( packet );
                return;
            }

            if ( m_packets[index].packet )
            {
                m_config.packetFactory->Destroy( m_packets[index].packet );
                m_packets[index].packet = nullptr;
            }

            const float delay = m_state.latency + jitter;

            m_packets[index].packet = packet;
            m_packets[index].packetNumber = m_packetNumberSend;
            m_packets[index].dequeueTime = m_timeBase.time + delay;
            
            packet->SetAddress( address );

            m_packetNumberSend++;
        }
    }

    protocol::Packet * Simulator::ReceivePacket()
    {
        PacketData * oldestPacket = nullptr;

        if ( m_tcpMode )
        {
            // TCP mode. We know the next packet number we must dequeue. 
            // "Lost" packets have extra delay added to simulate TCP retransmits.

            const int index = m_packetNumberReceive % m_config.numPackets;

            if ( m_packets[index].packet && m_packets[index].dequeueTime <= m_timeBase.time )
            {
                auto packet = m_packets[index].packet;
                m_packets[index].packet = nullptr;
                m_packetNumberReceive++;
                return packet;
            }
        }
        else
        {
            // UDP mode. Dequeue the oldest packet we find. Don't worry about ordering at all!

            for ( int i = 0; i < m_config.numPackets; ++i )
            {
                if ( m_packets[i].packet == nullptr || m_packets[i].dequeueTime > m_timeBase.time )
                    continue;

                if ( !oldestPacket || ( oldestPacket && m_packets[i].dequeueTime < oldestPacket->dequeueTime ) )
                    oldestPacket = &m_packets[i];
            }

            if ( oldestPacket )
            {
                auto packet = oldestPacket->packet;
                oldestPacket->packet = nullptr;
                return packet;
            }
        }

        return nullptr;
    }

    void Simulator::Update( const core::TimeBase & timeBase )
    {
        m_timeBase = timeBase;

        if ( m_numStates && ( rand() % m_config.stateChance ) == 0 )
        {
            const int stateIndex = rand() % m_numStates;
            m_state = m_states[stateIndex];
        }

        if ( !m_bandwidthSlidingWindow.IsEmpty() )
        {
            uint64_t bytes = 0;
            int numEntries = 0;
            uint16_t sequence = m_bandwidthSlidingWindow.GetBegin();
            while ( sequence != m_bandwidthSlidingWindow.GetEnd() )
            {
                const BandwidthEntry & entry = m_bandwidthSlidingWindow.Get( sequence );
                if ( entry.time >= m_timeBase.time - m_config.bandwidthTime - 0.001f )
                {
                    bytes += entry.packetSize;
                    numEntries++;
                }
                sequence++;
            }
            m_bandwidth = bytes * 8.0 / m_config.bandwidthTime / 1000.0;     // kilobits per-second
        }
    }

    protocol::Packet * Simulator::SerializePacket( protocol::Packet * input, int & packetSize )
    {
        CORE_ASSERT( input );

        const int packetType = input->GetType();
        const auto packetAddress = input->GetAddress();

        int bytes = 0;
        uint8_t buffer[m_config.maxPacketSize];

        // serialize write
        {
            typedef protocol::WriteStream Stream;

            Stream stream( buffer, m_config.maxPacketSize );

            stream.SetContext( m_context );

            input->SerializeWrite( stream );

            bytes = stream.GetBytesProcessed();

            CORE_ASSERT( bytes <= m_config.maxPacketSize );

            stream.Flush();

            CORE_ASSERT( !stream.IsOverflow() );

            m_config.packetFactory->Destroy( input );
        }

        // serialize read
        {
            protocol::Packet * packet = m_config.packetFactory->Create( packetType );

            packet->SetAddress( packetAddress );

            typedef protocol::ReadStream Stream;

            Stream stream( buffer, m_config.maxPacketSize );

            stream.SetContext( m_context );

            packet->SerializeRead( stream );

            CORE_ASSERT( !stream.IsOverflow() );

            packetSize = bytes + m_config.packetHeaderSize;

            return packet;
        }
    }
}
