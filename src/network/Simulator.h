// Network Library - Copyright (c) 2008-2015, The Network Protocol Company, Inc.

#ifndef NETWORK_SIMULATOR_H
#define NETWORK_SIMULATOR_H

#include "core/Core.h"
#include "network/Constants.h"
#include "network/Interface.h"

namespace core { class Allocator; }

// todo: network library should not depend on protocol
namespace protocol
{
    class Packet; 
    class PacketFactory; 
}

namespace network
{
    struct SimulatorConfig
    {
        core::Allocator * allocator;
        protocol::PacketFactory * packetFactory;
        int stateChance;                    // 1 in n chance to change state per-update
        int numPackets;                     // number of packets to buffer
        int maxPacketSize;                  // maximum packet size in bytes
        int packetHeaderSize;               // packet header size in bytes (for bandwidth calculations)
        bool serializePackets;              // if true then serialize read/writ packets

        SimulatorConfig()
        {   
            allocator = nullptr;            // allocator used for allocations with the same life cycle as this object.
            packetFactory = nullptr;        // packet factory. must be specified -- we need it to destroy buffered packets in destructor.
            stateChance = 1000;             // 1 in every 1000 chance per-update by default
            numPackets = 1024;              // buffer up to 1024 packets by default
            serializePackets = true;
            maxPacketSize = 1024;
            packetHeaderSize = 24;
        }
    };

    struct SimulatorState
    {
        float latency;                      // amount of latency in seconds
        float jitter;                       // amount of jitter +/- in seconds
        float packetLoss;                   // packet loss (%)

        SimulatorState()
        {
            latency = 0.0f;
            jitter = 0.0f;
            packetLoss = 0.0f;
        }

        SimulatorState( float _latency,
                        float _jitter,
                        float _packetLoss )
        {
            latency = _latency;
            jitter = _jitter;
            packetLoss = _packetLoss;
        }
    };

    class Simulator : public Interface
    {
    public:

        Simulator( const SimulatorConfig & config = SimulatorConfig() );

        ~Simulator();

        void Reset();

        int AddState( const SimulatorState & state );

        void ClearStates();

        void SendPacket( const Address & address, protocol::Packet * packet );

        protocol::Packet * ReceivePacket();

        void Update( const core::TimeBase & timeBase );

        void SetContext( const void ** context ) { m_context = context; }

        uint32_t GetMaxPacketSize() const
        {
            return m_config.maxPacketSize;
        }

        protocol::PacketFactory & GetPacketFactory() const
        {
            CORE_ASSERT( m_config.packetFactory );
            return *m_config.packetFactory;
        }

        void SetTCPMode( bool value ) 
        {
            Reset(); 
            m_tcpMode = value; 
        }

        bool GetTCPMode() const 
        { 
            return m_tcpMode; 
        }

    protected:

        protocol::Packet * SerializePacket( protocol::Packet * input );

    private:

        struct PacketData
        {
            protocol::Packet * packet = nullptr;
            double dequeueTime = 0.0;
            uint32_t packetNumber = 0;
        };

        const SimulatorConfig m_config;

        const void ** m_context;

        core::Allocator * m_allocator;

        core::TimeBase m_timeBase;

        uint32_t m_packetNumberSend;
        uint16_t m_packetNumberReceive;

        PacketData * m_packets;

        bool m_tcpMode;         // note: simulate TCP behavior. deliver packets reliably and in-order. 
                                // delay packets until simulated retransmission of lost packets @ 2X RTT

        int m_numStates;
        SimulatorState m_state;
        SimulatorState m_states[MaxSimulatorStates];

        Simulator( const Simulator & other );
        const Simulator & operator = ( const Simulator & other );
    };
}

#endif
