/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_NETWORK_SIMULATOR_H
#define PROTOCOL_NETWORK_SIMULATOR_H

#include "Common.h"
#include "NetworkInterface.h"

namespace protocol
{
    class Allocator;
    class PacketFactory;

    struct NetworkSimulatorConfig
    {
        Allocator * allocator;
        PacketFactory * packetFactory;
        int stateChance;                    // 1 in n chance to change state per-update
        int numPackets;                     // number of packets to buffer

        NetworkSimulatorConfig()
        {   
            allocator = nullptr;            // allocator used for allocations with the same life cycle as this object.
            packetFactory = nullptr;        // packet factory. must be specified -- we need it to destroy buffered packets in destructor.
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
    public:

        NetworkSimulator( const NetworkSimulatorConfig & config = NetworkSimulatorConfig() );

        ~NetworkSimulator();

        void AddState( const NetworkSimulatorState & state );

        void SendPacket( const Address & address, Packet * packet );

        Packet * ReceivePacket();

        void Update( const TimeBase & timeBase );

        uint32_t GetMaxPacketSize() const;

        PacketFactory & GetPacketFactory() const;

    private:

        struct PacketData
        {
            Packet * packet = nullptr;
            double dequeueTime = 0.0;
            uint32_t packetNumber = 0;
        };

        const NetworkSimulatorConfig m_config;

        Allocator * m_allocator;

        TimeBase m_timeBase;
        uint32_t m_packetNumber;

        PacketData * m_packets;

        int m_numStates;
        NetworkSimulatorState m_state;
        NetworkSimulatorState m_states[MaxSimulatorStates];

        NetworkSimulator( const NetworkSimulator & other );
        const NetworkSimulator & operator = ( const NetworkSimulator & other );
    };
}

#endif
