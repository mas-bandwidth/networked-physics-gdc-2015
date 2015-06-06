// Client Server Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef PROTOCOL_CLIENT_SERVER_CONTEXT_H
#define PROTOCOL_CLIENT_SERVER_CONTEXT_H

#include "core/Core.h"
#include "network/Address.h"

namespace clientServer
{
    struct ClientServerContext
    {
        const int ClassId = 0x12345;

        uint32_t classId = 0;

        struct ClientInfo
        {
            network::Address address;
            uint16_t clientId = 0;
            uint16_t serverId = 0;
            bool connected = false;
        };

        int numClients = 0;

        ClientInfo * clientInfo = nullptr;

        void Initialize( core::Allocator & allocator, int numClients );

        void Free( core::Allocator & allocator );

        void AddClient( int clientIndex, const network::Address & address, uint16_t clientId, uint16_t serverId );

        void RemoveClient( int clientIndex );

        int FindClient( const network::Address & address ) const;

        int FindClient( const network::Address & address, uint16_t clientId ) const;

        int FindClient( const network::Address & address, uint16_t clientId, uint16_t serverId ) const;

        bool ClientPotentiallyExists( uint16_t clientId, uint16_t serverId ) const;

        int FindFreeSlot() const;
    };
}

#endif
