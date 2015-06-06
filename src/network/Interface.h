// Network Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

#include "core/Core.h"
#include "network/Address.h"
#include "protocol/Stream.h"
#include "protocol/Packet.h"

namespace network
{
    class Interface
    {
    public:

        virtual ~Interface() {}

        virtual void SendPacket( const Address & address, protocol::Packet * packet ) = 0;        // takes ownership of packet pointer

        virtual protocol::Packet * ReceivePacket() = 0;                                           // gives you ownership of packet pointer

        virtual void Update( const core::TimeBase & timeBase ) = 0;

        virtual uint32_t GetMaxPacketSize() const = 0;
        virtual protocol::PacketFactory & GetPacketFactory() const = 0;
        virtual void SetContext( const void ** context ) = 0;
    };
}

#endif
