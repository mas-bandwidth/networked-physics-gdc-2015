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

#ifndef PROTOCOL_CLIENT_SERVER_CONTEXT_H
#define PROTOCOL_CLIENT_SERVER_CONTEXT_H

#include "core/Core.h"
#include "network/Address.h"

namespace clientServer
{
    struct ClientServerContext
    {
        enum Dummy { ClassId = 0x12345 };

        int classId;

        struct ClientInfo
        {
            network::Address address;
            uint16_t clientId;
            uint16_t serverId;
            bool connected;

            ClientInfo()
            {
                clientId = 0;
                serverId = 0;
                connected = false;
            }
        };

        int numClients;

        ClientInfo * clientInfo;

        ClientServerContext()
        {
            numClients = 0;
            clientInfo = NULL;
        }

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
