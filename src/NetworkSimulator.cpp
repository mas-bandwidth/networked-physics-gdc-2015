/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#include "NetworkSimulator.h"

namespace protocol
{
    NetworkSimulator::NetworkSimulator( const NetworkSimulatorConfig & config )
        : m_config( config )
    {
        m_packetNumber = 0;
        m_packets.resize( config.numPackets );
    }

    NetworkSimulator::~NetworkSimulator()
    {
        for ( int i = 0; i < m_packets.size(); ++i )
        {
            if ( m_packets[i].packet )
            {
                delete m_packets[i].packet;
                m_packets[i].packet = nullptr;
            }
        }
    }

    void NetworkSimulator::AddState( const NetworkSimulatorState & state )
    {
        m_states.push_back( state );
    }

    void NetworkSimulator::SendPacket( const Address & address, Packet * packet )
    {
//            printf( "send packet %d\n", m_packetNumber );

        assert( packet );

        packet->SetAddress( address );

        const int index = m_packetNumber % m_config.numPackets;

        if ( random_int( 0, 100 ) <= m_state.packetLoss )
        {
            delete packet;
            return;
        }

        const float delay = m_state.latency + random_float( -m_state.jitter, +m_state.jitter );

        m_packets[index].packet = packet;
        m_packets[index].packetNumber = m_packetNumber;
        m_packets[index].dequeueTime = m_timeBase.time + delay;
        
        m_packetNumber++;
    }

    Packet * NetworkSimulator::ReceivePacket()
    {
        PacketData * oldestPacket = nullptr;

        for ( int i = 0; i < m_packets.size(); ++i )
        {
            if ( m_packets[i].packet == nullptr )
                continue;

            if ( !oldestPacket || ( oldestPacket && m_packets[i].dequeueTime < oldestPacket->dequeueTime ) )
                oldestPacket = &m_packets[i];
        }

        if ( oldestPacket && oldestPacket->dequeueTime <= m_timeBase.time )
        {
//                printf( "receive packet %d\n", (int) oldestPacket->packetNumber );
            auto packet = oldestPacket->packet;
            oldestPacket->packet = nullptr;
            return packet;
        }

        return nullptr;
    }

    void NetworkSimulator::Update( const TimeBase & timeBase )
    {
        m_timeBase = timeBase;

        if ( m_states.size() && ( rand() % m_config.stateChance ) == 0 )
        {
            int stateIndex = rand() % m_states.size();
//                printf( "*** network simulator switching to state %d ***\n", stateIndex );
            m_state = m_states[stateIndex];
        }
    }

    uint32_t NetworkSimulator::GetMaxPacketSize() const
    {
        return 0xFFFFFFFF;
    }
}
