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

#ifndef PROTOCOL_CONNECTION_H
#define PROTOCOL_CONNECTION_H

#include "core/Core.h"
#include "protocol/Stream.h"
#include "protocol/Channel.h"
#include "protocol/SequenceBuffer.h"
#include "protocol/ConnectionPacket.h"

namespace protocol
{
    class PacketFactory;
    class ChannelStructure;

    struct ConnectionConfig
    {
        core::Allocator * allocator = nullptr;
        int packetType = protocol::CONNECTION_PACKET;
        int maxPacketSize = 1024;
        int slidingWindowSize = 256;
        PacketFactory * packetFactory = nullptr;
        ChannelStructure * channelStructure = nullptr;
        const void ** context = nullptr;
    };

    struct SentPacketData { uint8_t acked; };
    struct ReceivedPacketData {};
    typedef SequenceBuffer<SentPacketData> SentPackets;
    typedef SequenceBuffer<ReceivedPacketData> ReceivedPackets;

    class Connection
    {
        ConnectionError m_error = CONNECTION_ERROR_NONE;

        const ConnectionConfig m_config;                            // const configuration data

        core::Allocator * m_allocator;                              // allocator for allocations matching life cycle of object.

        core::TimeBase m_timeBase;                                  // network time base
        SentPackets * m_sentPackets = nullptr;                      // sliding window of recently sent packets
        ReceivedPackets * m_receivedPackets = nullptr;              // sliding window of recently received packets
        int m_numChannels = 0;                                      // cached number of channels
        Channel * m_channels[MaxChannels];                          // array of channels created according to channel structure
        uint64_t m_counters[CONNECTION_COUNTER_NUM_COUNTERS];       // counters for unit testing, stats etc.

    public:

        Connection( const ConnectionConfig & config );

        ~Connection();

        Channel * GetChannel( int index );

        void Reset();

        void Update( const core::TimeBase & timeBase );

        ConnectionError GetError() const;

        int GetChannelError( int channelIndex ) const;

        const core::TimeBase & GetTimeBase() const;

        ConnectionPacket * WritePacket();

        bool ReadPacket( ConnectionPacket * packet );

        uint64_t GetCounter( int index ) const;

        void ProcessAcks( uint16_t ack, uint32_t ack_bits );

        void PacketAcked( uint16_t sequence );
    };
}

#endif
