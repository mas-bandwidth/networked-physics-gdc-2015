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

        m_packetNumber = 0;
        m_numStates = 0;
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

        for ( int i = 0; i < m_config.numPackets; ++i )
        {
            if ( m_packets[i].packet )
            {
                m_config.packetFactory->Destroy( m_packets[i].packet );
                m_packets[i].packet = nullptr;
            }
        }
    }

    void Simulator::AddState( const SimulatorState & state )
    {
        CORE_ASSERT( m_numStates < MaxSimulatorStates - 1 );
        m_states[m_numStates++] = state;
        if ( m_numStates == 1 )
            m_state = m_states[0];
    }

    void Simulator::SendPacket( const Address & address, protocol::Packet * packet )
    {
        CORE_ASSERT( packet );

        if ( core::random_int( 0, 99 ) < m_state.packetLoss )
        {
            m_config.packetFactory->Destroy( packet );
            return;
        }

        const int index = m_packetNumber % m_config.numPackets;

        if ( m_packets[index].packet )
        {
            m_config.packetFactory->Destroy( m_packets[index].packet );
            m_packets[index].packet = nullptr;
        }

        const float delay = m_state.latency + core::random_float( -m_state.jitter, +m_state.jitter );

        m_packets[index].packet = packet;
        m_packets[index].packetNumber = m_packetNumber;
        m_packets[index].dequeueTime = m_timeBase.time + delay;
        
        packet->SetAddress( address );

        m_packetNumber++;
    }

    protocol::Packet * Simulator::ReceivePacket()
    {
        PacketData * oldestPacket = nullptr;

        for ( int i = 0; i < m_config.numPackets; ++i )
        {
            if ( m_packets[i].packet == nullptr || m_packets[i].dequeueTime >= m_timeBase.time )
                continue;

            if ( !oldestPacket || ( oldestPacket && m_packets[i].dequeueTime < oldestPacket->dequeueTime ) )
                oldestPacket = &m_packets[i];
        }

        if ( oldestPacket )
        {
            protocol::Packet * packet = oldestPacket->packet;
            oldestPacket->packet = nullptr;
            return packet;
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
