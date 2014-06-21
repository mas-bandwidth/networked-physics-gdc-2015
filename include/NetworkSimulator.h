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

        PacketData * m_packets;

        int m_numStates;
        NetworkSimulatorState m_state;
        NetworkSimulatorState m_states[MaxSimulatorStates];

        NetworkSimulator( const NetworkSimulator & other );
        const NetworkSimulator & operator = ( const NetworkSimulator & other );

    public:

        NetworkSimulator( const NetworkSimulatorConfig & config = NetworkSimulatorConfig() );

        ~NetworkSimulator();

        void AddState( const NetworkSimulatorState & state );

        void SendPacket( const Address & address, Packet * packet );

        Packet * ReceivePacket();

        void Update( const TimeBase & timeBase );

        uint32_t GetMaxPacketSize() const;
    };
}

#endif
