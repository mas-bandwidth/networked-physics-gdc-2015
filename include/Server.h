/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_SERVER_H
#define PROTOCOL_SERVER_H

#include "Connection.h"
#include "NetworkInterface.h"
#include "ClientServerPackets.h"

namespace protocol
{
    struct ServerConfig
    {
        uint64_t protocolId = 0;                                // the protocol id.

        shared_ptr<NetworkInterface> networkInterface;
    };

    class Server
    {
        ServerConfig m_config;

    public:

        Server( const ServerConfig & config )
            : m_config( config )
        {
            assert( m_config.networkInterface );               // you must supply a network interface!
            
            // ...
        }

        ~Server()
        {
            // ...
        }
    };
}

#endif
