/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
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

        virtual void SendPacket( const Address & address, shared_ptr<Packet> packet ) = 0;

        virtual void SendPacket( const string & hostname, uint16_t port, shared_ptr<Packet> packet ) = 0;

        virtual shared_ptr<Packet> ReceivePacket() = 0;

        virtual void Update( const TimeBase & timeBase ) = 0;
    };
}

#endif
