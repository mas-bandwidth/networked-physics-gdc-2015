/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_CONNECTION_H
#define PROTOCOL_CONNECTION_H

#include "Common.h"
#include "Stream.h"
#include "Packets.h"
#include "Channel.h"
#include "SlidingWindow.h"

namespace protocol
{
    struct ConnectionConfig
    {
        uint64_t protocolId = 0;
        int packetType = 0;
        int maxPacketSize = 1024;
        int slidingWindowSize = 256;
        Factory<Packet> * packetFactory = nullptr;
        ChannelStructure * channelStructure = nullptr;
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
        TimeBase m_timeBase;                                        // network time base
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

        void Update( const TimeBase & timeBase );

        ConnectionError GetError() const;

        int GetChannelError( int channelIndex ) const;

        const TimeBase & GetTimeBase() const;

        ConnectionPacket * WritePacket();

        bool ReadPacket( ConnectionPacket * packet );

        uint64_t GetCounter( int index ) const;

        void ProcessAcks( uint16_t ack, uint32_t ack_bits );

        void PacketAcked( uint16_t sequence );
    };
}

#endif
