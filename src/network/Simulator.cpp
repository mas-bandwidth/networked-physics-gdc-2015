// Network Library - Copyright (c) 2008-2015, The Network Protocol Company, Inc.

#include "network/Simulator.h"
#include "core/Memory.h"
#include "protocol/PacketFactory.h"      // todo: we need to not depend on protocol from network

namespace network
{
    Simulator::Simulator( const SimulatorConfig & config ) : m_config( config )
    {
        CORE_ASSERT( m_config.numPackets > 0 );
        CORE_ASSERT( m_config.packetFactory );

        m_allocator = m_config.allocator ? m_config.allocator : &core::memory::default_allocator();

        m_packets = CORE_NEW_ARRAY( *m_allocator, PacketData, config.numPackets );

        m_packetNumberSend = 0;
        m_packetNumberReceive = 0;

        m_numStates = 0;

        m_tcpMode = false;
    }

    Simulator::~Simulator()
    {
        CORE_ASSERT( m_allocator );
        CORE_ASSERT( m_packets );

        Reset();

        CORE_DELETE_ARRAY( *m_allocator, m_packets, m_config.numPackets );

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
            int stateIndex = rand() % m_numStates;
            m_state = m_states[stateIndex];
        }
    }

    uint32_t Simulator::GetMaxPacketSize() const
    {
        return 0xFFFFFFFF;
    }

    protocol::PacketFactory & Simulator::GetPacketFactory() const
    {
        CORE_ASSERT( m_config.packetFactory );
        return *m_config.packetFactory;
    }
}
