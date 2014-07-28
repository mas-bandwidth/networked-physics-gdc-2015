/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#include "ClientServerContext.h"

#include "Memory.h"

namespace protocol
{
    void ClientServerContext::Initialize( Allocator & allocator, int numClients )
    {
        PROTOCOL_ASSERT( numClients > 0 );
        this->classId = ClientServerContext::ClassId;
        this->numClients = numClients;
        this->clientInfo = (ClientInfo*) PROTOCOL_NEW_ARRAY( allocator, ClientInfo, numClients );
    }

    void ClientServerContext::Free( Allocator & allocator )
    {
        PROTOCOL_ASSERT( clientInfo );
        PROTOCOL_DELETE_ARRAY( allocator, clientInfo, numClients );
        clientInfo = nullptr;
        numClients = 0;
    }

    void ClientServerContext::AddClient( int clientIndex, const Address & address, uint16_t clientId, uint16_t serverId )
    {
        PROTOCOL_ASSERT( clientIndex >= 0 );
        PROTOCOL_ASSERT( clientIndex < numClients );
        ClientInfo & client = clientInfo[clientIndex];
        client.connected = true;
        client.address = address;
        client.clientId = clientId;
        client.serverId = serverId;
    }

    void ClientServerContext::RemoveClient( int clientIndex )
    {
        PROTOCOL_ASSERT( clientIndex >= 0 );
        PROTOCOL_ASSERT( clientIndex < numClients );
        clientInfo[clientIndex] = ClientInfo();
    }

    int ClientServerContext::FindClient( const Address & address ) const
    {
        PROTOCOL_ASSERT( classId == ClientServerContext::ClassId );
        for ( int i = 0; i < numClients; ++i )
        {
            if ( clientInfo[i].connected && 
                 clientInfo[i].address == address )
                return i;
        }
        return -1;
    }

    int ClientServerContext::FindClient( const Address & address, uint16_t clientId ) const
    {
        PROTOCOL_ASSERT( classId == ClientServerContext::ClassId );
        for ( int i = 0; i < numClients; ++i )
        {
            if ( clientInfo[i].connected && 
                 clientInfo[i].address == address &&
                 clientInfo[i].clientId == clientId )
                return i;
        }
        return -1;
    }

    int ClientServerContext::FindClient( const Address & address, uint16_t clientId, uint16_t serverId ) const
    {
        PROTOCOL_ASSERT( classId == ClientServerContext::ClassId );
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
        PROTOCOL_ASSERT( classId == ClientServerContext::ClassId );
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
