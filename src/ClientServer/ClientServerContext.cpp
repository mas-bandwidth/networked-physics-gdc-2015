// Client Server Library - Copyright (c) 2008-2015, Glenn Fiedler

#include "ClientServerContext.h"
#include "core/Memory.h"

namespace clientServer
{
    void ClientServerContext::Initialize( core::Allocator & allocator, int numClients )
    {
        CORE_ASSERT( numClients > 0 );
        this->classId = ClientServerContext::ClassId;
        this->numClients = numClients;
        this->clientInfo = (ClientInfo*) CORE_NEW_ARRAY( allocator, ClientInfo, numClients );
    }

    void ClientServerContext::Free( core::Allocator & allocator )
    {
        CORE_ASSERT( clientInfo );
        CORE_DELETE_ARRAY( allocator, clientInfo, numClients );
        clientInfo = nullptr;
        numClients = 0;
    }

    void ClientServerContext::AddClient( int clientIndex, const network::Address & address, uint16_t clientId, uint16_t serverId )
    {
        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < numClients );
        ClientInfo & client = clientInfo[clientIndex];
        client.connected = true;
        client.address = address;
        client.clientId = clientId;
        client.serverId = serverId;
    }

    void ClientServerContext::RemoveClient( int clientIndex )
    {
        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < numClients );
        clientInfo[clientIndex] = ClientInfo();
    }

    int ClientServerContext::FindClient( const network::Address & address ) const
    {
        CORE_ASSERT( classId == ClientServerContext::ClassId );
        for ( int i = 0; i < numClients; ++i )
        {
            if ( clientInfo[i].connected && 
                 clientInfo[i].address == address )
                return i;
        }
        return -1;
    }

    int ClientServerContext::FindClient( const network::Address & address, uint16_t clientId ) const
    {
        CORE_ASSERT( classId == ClientServerContext::ClassId );
        for ( int i = 0; i < numClients; ++i )
        {
            if ( clientInfo[i].connected && 
                 clientInfo[i].address == address &&
                 clientInfo[i].clientId == clientId )
                return i;
        }
        return -1;
    }

    int ClientServerContext::FindClient( const network::Address & address, uint16_t clientId, uint16_t serverId ) const
    {
        CORE_ASSERT( classId == ClientServerContext::ClassId );
        for ( int i = 0; i < numClients; ++i )
        {
            if ( clientInfo[i].connected && 
                 clientInfo[i].address == address &&
                 clientInfo[i].clientId == clientId && 
                 clientInfo[i].serverId == serverId )
                return i;
        }
        return -1;
    }

    bool ClientServerContext::ClientPotentiallyExists( uint16_t clientId, uint16_t serverId ) const
    {
        CORE_ASSERT( classId == ClientServerContext::ClassId );
        for ( int i = 0; i < numClients; ++i )
        {
            if ( clientInfo[i].connected &&
                 clientInfo[i].clientId == clientId && 
                 clientInfo[i].serverId == serverId )
                return true;
        }
        return false;
    }
}
