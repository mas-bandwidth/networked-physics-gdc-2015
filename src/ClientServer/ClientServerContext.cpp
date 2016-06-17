/*
    Networked Physics Demo

    Copyright © 2008 - 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ClientServerContext.h"
#include "core/Memory.h"

namespace clientServer
{
    void ClientServerContext::Initialize( core::Allocator & allocator, int _numClients )
    {
        CORE_ASSERT( _numClients > 0 );
        classId = ClientServerContext::ClassId;
        numClients = _numClients;
        clientInfo = (ClientInfo*) CORE_NEW_ARRAY( allocator, ClientInfo, numClients );
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
        CORE_ASSERT( (int) classId == ClientServerContext::ClassId );
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
        CORE_ASSERT( (int) classId == ClientServerContext::ClassId );
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
        CORE_ASSERT( (int) classId == ClientServerContext::ClassId );
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
        CORE_ASSERT( (int) classId == ClientServerContext::ClassId );
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
