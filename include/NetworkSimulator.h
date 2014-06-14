/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_NETWORK_SIMULATOR_H
#define PROTOCOL_NETWORK_SIMULATOR_H

#include "Common.h"
#include "NetworkInterface.h"

namespace protocol
{
    struct NetworkSimulatorConfig
    {
        int stateChance;                    // 1 in n chance to pick a new state per-update
        int numPackets;                     // number of packets to buffer

        NetworkSimulatorConfig()
        {
            stateChance = 1000;             // 1 in every 1000 chance per-update by default
            numPackets = 1024;              // buffer up to 1024 packets by default
        }
    };

    struct NetworkSimulatorState
    {
        float latency;                      // amount of latency in seconds
        float jitter;                       // amount of jitter +/- in seconds
        float packetLoss;                   // packet loss (%)

        NetworkSimulatorState()
        {
            latency = 0.0f;
            jitter = 0.0f;
            packetLoss = 0.0f;
        }

        NetworkSimulatorState( float _latency,
                               float _jitter,
                               float _packetLoss )
        {
            latency = _latency;
            jitter = _jitter;
            packetLoss = _packetLoss;
        }
    };

    class NetworkSimulator : public NetworkInterface
    {
        struct PacketData
        {
            Packet * packet;
            double dequeueTime;
            uint32_t packetNumber;
        };

        const NetworkSimulatorConfig m_config;

        TimeBase m_timeBase;
        uint32_t m_packetNumber;
        std::vector<PacketData> m_packets;
        NetworkSimulatorState m_state;
        std::vector<NetworkSimulatorState> m_states;

        NetworkSimulator( const NetworkSimulator & other );
        const NetworkSimulator & operator = ( const NetworkSimulator & other );

    public:

        NetworkSimulator( const NetworkSimulatorConfig & config = NetworkSimulatorConfig() )
            : m_config( config )
        {
            m_packetNumber = 0;
            m_packets.resize( config.numPackets );
        }

        ~NetworkSimulator()
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

        void AddState( const NetworkSimulatorState & state )
        {
            m_states.push_back( state );
        }

        void SendPacket( const Address & address, Packet * packet )
        {
//            printf( "send packet %d\n", m_packetNumber );

            assert( packet );

            packet->SetAddress( address );

            const int index = m_packetNumber % m_config.numPackets;

            // todo: what the fuck? packet loss is disabled?!

//            if ( random_int( 0, 100 ) <= m_config.packetLoss )
//            {
//                delete packet;
//                return;
//            }

            const float delay = m_state.latency + random_float( -m_state.jitter, +m_state.jitter );

            m_packets[index].packet = packet;
            m_packets[index].packetNumber = m_packetNumber;
            m_packets[index].dequeueTime = m_timeBase.time + delay;
            
            m_packetNumber++;
        }

        Packet * ReceivePacket()
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

        void Update( const TimeBase & timeBase )
        {
            m_timeBase = timeBase;

            if ( m_states.size() && ( rand() % m_config.stateChance ) == 0 )
            {
                int stateIndex = rand() % m_states.size();
//                printf( "*** network simulator switching to state %d ***\n", stateIndex );
                m_state = m_states[stateIndex];
            }
        }

        uint32_t GetMaxPacketSize() const
        {
            return 0xFFFFFFFF;
        }
    };
}

#endif
