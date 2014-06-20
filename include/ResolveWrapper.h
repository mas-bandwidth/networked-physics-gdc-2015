/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef RESOLVE_WRAPPER_H
#define RESOLVE_WRAPPER_H

#include "NetworkInterface.h"
#include "Resolver.h"

namespace protocol
{
    struct ResolveWrapperConfig
    {
        Resolver * resolver = nullptr;                      // resolver eg: DNS
        NetworkInterface * networkInterface = nullptr;      // the network interface we are wrapping
    };

    class ResolveWrapper : public NetworkInterface
    {
        const ResolveWrapperConfig m_config;

        std::map<std::string,PacketQueue*> m_resolve_send_queues;

    public:

        ResolveWrapper( const ResolveWrapperConfig & config );

        ~ResolveWrapper();

        void SendPacket( const Address & address, Packet * packet );

        void SendPacket( const std::string & hostname, Packet * packet );

        void SendPacket( const std::string & hostname, uint16_t port, Packet * packet );

        Packet * ReceivePacket();

        void Update( const TimeBase & timeBase );

        uint32_t GetMaxPacketSize() const;
    };
}

#endif
