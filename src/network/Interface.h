// Network Library - Copyright (c) 2014, The Network Protocol Company, Inc.

#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

// todo: maybe rename this to "Interface.h"?

#include "core/Common.h"
#include "network/Address.h"
#include "Stream.h"     // todo: can't depend on protocol stuff from network!
#include "Packet.h"

namespace network
{
    class Interface
    {
    public:

        virtual ~Interface() {}

        virtual void SendPacket( const Address & address, Packet * packet ) = 0;        // takes ownership of packet pointer

        virtual Packet * ReceivePacket() = 0;                                           // gives you ownership of packet pointer

        virtual void Update( const TimeBase & timeBase ) = 0;

        // todo: really can't be doing this stuff at the network interface level!
        virtual uint32_t GetMaxPacketSize() const = 0;
        virtual PacketFactory & GetPacketFactory() const = 0;
        virtual void SetContext( const void ** context ) = 0;
    };
}

#endif
