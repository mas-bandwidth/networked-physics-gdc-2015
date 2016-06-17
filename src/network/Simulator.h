/*
    Networked Physics Demo

    Copyright © 2008 - 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef NETWORK_SIMULATOR_H
#define NETWORK_SIMULATOR_H

#include "core/Core.h"
#include "core/Memory.h"
#include "network/Constants.h"
#include "network/Interface.h"
#include "protocol/SlidingWindow.h"

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
        int bandwidthSize;                  // number of entries in bandwidth sliding window
        float bandwidthTime;                // average bandwidth over this amount of time in the past

        SimulatorConfig()
        {   
            allocator = &core::memory::default_allocator();
            packetFactory = nullptr;
            stateChance = 1000;
            numPackets = 1024;
            serializePackets = true;
            maxPacketSize = 1024;
            packetHeaderSize = 28;
            bandwidthSize = 1024;
            bandwidthTime = 0.5f;
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

    struct BandwidthEntry
    {
        double time = 0.0;
        int packetSize = 0;
    };

    typedef protocol::SlidingWindow<BandwidthEntry> BandwidthSlidingWindow;

    class Simulator : public Interface
    {
    public:

        Simulator( const SimulatorConfig & config = SimulatorConfig() );

        ~Simulator();

        void Reset();

        int AddState( const SimulatorState & state );

        void ClearStates();

        void SetBandwidthExclude( bool flag ) { m_bandwidthExclude = flag; }

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

        float GetBandwidth() const
        {
            return m_bandwidth;     // kbps
        }

    protected:

        protocol::Packet * SerializePacket( protocol::Packet * input, int & packetSize );

    private:

        struct PacketData
        {
            protocol::Packet * packet = nullptr;
            double dequeueTime = 0.0;
            uint32_t packetNumber = 0;
        };

        const SimulatorConfig m_config;

        const void ** m_context;

        core::TimeBase m_timeBase;

        uint32_t m_packetNumberSend;
        uint16_t m_packetNumberReceive;

        PacketData * m_packets;

        bool m_tcpMode;
        bool m_bandwidthExclude;

        float m_bandwidth;

        int m_numStates;
        SimulatorState m_state;
        SimulatorState m_states[MaxSimulatorStates];

        BandwidthSlidingWindow m_bandwidthSlidingWindow;

        Simulator( const Simulator & other );
        const Simulator & operator = ( const Simulator & other );
    };
}

#endif
