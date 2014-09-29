// Protocol Library - Copyright (c) 2014, The Network Protocol Company, Inc.

#ifndef PROTOCOL_CONNECTION_H
#define PROTOCOL_CONNECTION_H

#include "core/Core.h"
#include "protocol/Stream.h"
#include "protocol/Packets.h"
#include "protocol/Channel.h"
#include "protocol/SlidingWindow.h"

namespace protocol
{
    class PacketFactory;
    class ChannelStructure;

    struct ConnectionConfig
    {
        core::Allocator * allocator = nullptr;
        int packetType = 0;
        int maxPacketSize = 1024;
        int slidingWindowSize = 256;
        PacketFactory * packetFactory = nullptr;
        ChannelStructure * channelStructure = nullptr;
        const void ** context = nullptr;
    };

    struct SentPacketData
    {
        SentPacketData()
            : valid(0), acked(0), sequence(0) {}

        SentPacketData( uint16_t _sequence )
            : valid(1), acked(0), sequence( _sequence ) {}

        uint32_t valid : 1;                     // is this packet entry valid?
        uint32_t acked : 1;                     // has packet been acked?
        uint32_t sequence : 16;                 // packet sequence #
    };

    struct ReceivedPacketData
    {
        ReceivedPacketData()
            : valid(0), sequence(0) {}

        ReceivedPacketData( uint16_t _sequence )
            : valid(1), sequence( _sequence ) {}

        uint32_t valid : 1;                     // is this packet entry valid?
        uint32_t sequence : 16;                 // packet sequence #
    };

    typedef SlidingWindow<SentPacketData> SentPackets;
    typedef SlidingWindow<ReceivedPacketData> ReceivedPackets;

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
