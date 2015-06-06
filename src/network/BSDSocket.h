// Network Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef NETWORK_BSD_SOCKET_H
#define NETWORK_BSD_SOCKET_H

#include "core/Types.h"
#include "network/Interface.h"
#include "protocol/PacketFactory.h"

namespace core { class Allocator; }

namespace network 
{     
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
            sendQueueSize = 256;
            receiveQueueSize = 256;
        }

        core::Allocator * allocator;                // allocator for long term allocations matching object life cycle. if nullptr then the default allocator is used.
        uint64_t protocolId;                        // the protocol id. packets sent are prefixed with this id and discarded on receive if the protocol id does not match
        uint16_t port;                              // port to bind UDP socket to
        bool ipv6;                                  // use ipv6 sockets if true
        int maxPacketSize;                          // maximum packet size
        int sendQueueSize;                          // send queue size between "SendPacket" and sendto. additional sent packets will be dropped.
        int receiveQueueSize;                       // send queue size between "recvfrom" and "ReceivePacket" function. additional received packets will be dropped.
        protocol::PacketFactory * packetFactory;    // packet factory (required)
    };

    class BSDSocket : public Interface
    {
    public:

        BSDSocket( const BSDSocketConfig & config = BSDSocketConfig() );

        ~BSDSocket();

        bool IsError() const;

        bool GetError() const;

        void SendPacket( const Address & address, protocol::Packet * packet );

        protocol::Packet * ReceivePacket();

        void Update( const core::TimeBase & timeBase );

        uint32_t GetMaxPacketSize() const;

        protocol::PacketFactory & GetPacketFactory() const;

        void SetContext( const void ** context );

        uint64_t GetCounter( int index ) const;

        uint16_t GetPort() const;

    private:

        void SendPackets();

        void ReceivePackets();

        bool SendPacketInternal( const Address & address, const uint8_t * data, size_t bytes );
    
        int ReceivePacketInternal( Address & sender, void * data, int size );

    private:

        const BSDSocketConfig m_config;

        core::Allocator * m_allocator;        
        
        int m_socket;
        uint16_t m_port;
        BSDSocketError m_error;
        core::Queue<protocol::Packet*> m_send_queue;
        core::Queue<protocol::Packet*> m_receive_queue;
        uint8_t * m_receiveBuffer;
        const void ** m_context;
        uint64_t m_counters[BSD_SOCKET_COUNTER_NUM_COUNTERS];

        BSDSocket( BSDSocket & other );
        BSDSocket & operator = ( BSDSocket & other );
    };
}

#endif
