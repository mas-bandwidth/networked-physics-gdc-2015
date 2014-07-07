/*
    Network Protocol Library.
    Copyright (c) 2014 The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_BSD_SOCKET_H
#define PROTOCOL_BSD_SOCKET_H

#include "PacketFactory.h"
#include "NetworkInterface.h"

namespace protocol 
{     
    class Allocator;

    struct BSDSocketConfig
    {
        BSDSocketConfig()
        {
            allocator = nullptr;
            protocolId = 0x12345;
            port = 10000;
            ipv6 = true;
            maxPacketSize = 10*1024;
            packetFactory = nullptr;
        }

        Allocator * allocator;                      // allocator for long term allocations matching object life cycle. if nullptr then the default allocator is used.
        uint64_t protocolId;                        // the protocol id. packets sent are prefixed with this id and discarded on receive if the protocol id does not match
        uint16_t port;                              // port to bind UDP socket to
        bool ipv6;                                  // use ipv6 sockets if true
        int maxPacketSize;                          // maximum packet size
        PacketFactory * packetFactory;              // packet factory (required)
    };

    class BSDSocket : public NetworkInterface
    {
        const BSDSocketConfig m_config;

        Allocator * m_allocator;        
        
        int m_socket;
        BSDSocketError m_error;
        PacketQueue m_send_queue;
        PacketQueue m_receive_queue;
        uint8_t * m_receiveBuffer;
        uint64_t m_counters[BSD_SOCKET_COUNTER_NUM_COUNTERS];

        BSDSocket( BSDSocket & other );
        BSDSocket & operator = ( BSDSocket & other );

    public:

        BSDSocket( const BSDSocketConfig & config = BSDSocketConfig() );

        ~BSDSocket();

        bool IsError() const;

        bool GetError() const;

        void SendPacket( const Address & address, Packet * packet );

        Packet * ReceivePacket();

        void Update( const TimeBase & timeBase );

        uint32_t GetMaxPacketSize() const;

        PacketFactory & GetPacketFactory() const;

        uint64_t GetCounter( int index ) const;

    private:

        void SendPackets();

        void ReceivePackets();

        bool SendPacketInternal( const Address & address, const uint8_t * data, size_t bytes );
    
        int ReceivePacketInternal( Address & sender, void * data, int size );
    };
}

#endif
