/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_CLIENT_SERVER_CONTEXT_H
#define PROTOCOL_CLIENT_SERVER_CONTEXT_H

#include "Common.h"
#include "Address.h"

namespace protocol
{
    struct ClientServerContext
    {
        const int ClassId = 0x12345;

        uint32_t classId = 0;

        struct ClientInfo
        {
            Address address;
            uint16_t clientId = 0;
            uint16_t serverId = 0;
            bool connected = false;
        };

        int numClients = 0;

        ClientInfo * clientInfo = nullptr;

        void Initialize( Allocator & allocator, int numClients );

        void Free( Allocator & allocator );

        void AddClient( int clientIndex, const Address & address, uint16_t clientId, uint16_t serverId );

        void RemoveClient( int clientIndex );

        int FindClient( const Address & address ) const;

        int FindClient( const Address & address, uint16_t clientId ) const;

        int FindClient( const Address & address, uint16_t clientId, uint16_t serverId ) const;

        bool ClientPotentiallyExists( uint16_t clientId, uint16_t serverId ) const;

        int FindFreeSlot() const;
    };
}

#endif
