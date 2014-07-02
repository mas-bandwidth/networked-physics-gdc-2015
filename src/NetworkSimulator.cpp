/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#include "NetworkSimulator.h"
#include "PacketFactory.h"

namespace protocol
{
    NetworkSimulator::NetworkSimulator( const NetworkSimulatorConfig & config )
        : m_config( config )
    {
        assert( m_config.packetFactory );
        m_packetNumber = 0;
        // todo: convert to using custom allocator
        m_packets = new PacketData[config.numPackets];
        memset( m_packets, 0, sizeof(PacketData) * config.numPackets );
        m_numStates = 0;
    }

    NetworkSimulator::~NetworkSimulator()
    {
        assert( m_packets );

        for ( int i = 0; i < m_config.numPackets; ++i )
        {
            if ( m_packets[i].packet )
            {
                m_config.packetFactory->Destroy( m_packets[i].packet );
                m_packets[i].packet = nullptr;
            }
        }

        delete [] m_packets;
        m_packets = nullptr;
    }

    void NetworkSimulator::AddState( const NetworkSimulatorState & state )
    {
        assert( m_numStates < MaxSimulatorStates - 1 );
        m_states[m_numStates++] = state;
        if ( m_numStates == 1 )
            m_state = m_states[0];
    }

    void NetworkSimulator::SendPacket( const Address & address, Packet * packet )
    {
        assert( packet );

        if ( random_int( 0, 99 ) < m_state.packetLoss )
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

        const float delay = m_state.latency + random_float( -m_state.jitter, +m_state.jitter );

        m_packets[index].packet = packet;
        m_packets[index].packetNumber = m_packetNumber;
        m_packets[index].dequeueTime = m_timeBase.time + delay;
        
        packet->SetAddress( address );

        m_packetNumber++;
    }

    Packet * NetworkSimulator::ReceivePacket()
    {
        PacketData * oldestPacket = nullptr;

        for ( int i = 0; i < m_config.numPackets; ++i )
        {
            if ( m_packets[i].packet == nullptr || m_packets[i].dequeueTime > m_timeBase.time )
                continue;

            if ( !oldestPacket || ( oldestPacket && m_packets[i].dequeueTime < oldestPacket->dequeueTime ) )
                oldestPacket = &m_packets[i];
        }

        if ( oldestPacket )
        {
            Packet * packet = oldestPacket->packet;
            oldestPacket->packet = nullptr;
            return packet;
        }

        return nullptr;
    }

    void NetworkSimulator::Update( const TimeBase & timeBase )
    {
        m_timeBase = timeBase;

        if ( m_numStates && ( rand() % m_config.stateChance ) == 0 )
        {
            int stateIndex = rand() % m_numStates;
            m_state = m_states[stateIndex];
        }
    }

    uint32_t NetworkSimulator::GetMaxPacketSize() const
    {
        return 0xFFFFFFFF;
    }

    PacketFactory & NetworkSimulator::GetPacketFactory() const
    {
        assert( m_config.packetFactory );
        return *m_config.packetFactory;
    }
}
