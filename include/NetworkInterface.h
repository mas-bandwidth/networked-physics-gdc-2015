/*
    Network Protocol Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_NETWORK_INTERFACE_H
#define PROTOCOL_NETWORK_INTERFACE_H

#include "Common.h"
#include "Stream.h"
#include "Address.h"
#include "Packet.h"

namespace protocol
{
    class NetworkInterface
    {
    public:

        virtual ~NetworkInterface() {}

        virtual void SendPacket( const Address & address, Packet * packet ) = 0;        // takes ownership of packet pointer

        virtual Packet * ReceivePacket() = 0;                                           // gives you ownership of packet pointer

        virtual void Update( const TimeBase & timeBase ) = 0;

        virtual uint32_t GetMaxPacketSize() const = 0;

        virtual PacketFactory & GetPacketFactory() const = 0;
    };
}

#endif
