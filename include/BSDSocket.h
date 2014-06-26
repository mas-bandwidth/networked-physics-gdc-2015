/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_BSD_SOCKET_H
#define PROTOCOL_BSD_SOCKET_H

#include "PacketFactory.h"
#include "NetworkInterface.h"

// todo: replace family with enum ADDRESS_
#include <netinet/in.h>

namespace protocol 
{     
    struct BSDSocketConfig
    {
        BSDSocketConfig()
        {
            protocolId = 0x12345;
            port = 10000;
            // todo: this should move somewhere so we don't need this definition at the header
            family = AF_INET6;                      // default to IPv6.
            maxPacketSize = 10*1024;
            packetFactory = nullptr;
        }

        uint64_t protocolId;                        // the protocol id. packets sent are prefixed with this id and discarded on receive if the protocol id does not match
        uint16_t port;                              // port to bind UDP socket to
        int family;                                 // socket family: eg. AF_INET (IPv4 only), AF_INET6 (IPv6 only)
        int maxPacketSize;                          // maximum packet size
        PacketFactory * packetFactory;              // packet factory (required)
    };

    class BSDSocket : public NetworkInterface
    {
        const BSDSocketConfig m_config;

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
