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

#ifndef CLIENT_SERVER_DATA_BLOCK_H
#define CLIENT_SERVER_DATA_BLOCK_H

#include "network/Address.h"
#include "protocol/DataBlockSender.h"
#include "protocol/DataBlockReceiver.h"

namespace core { class Allocator; }
namespace network { class Interface; }
namespace protocol { class Block; class PacketFactory; }

namespace clientServer
{
    struct ClientServerInfo
    {
        network::Address address;
        uint16_t clientId;
        uint16_t serverId;
        protocol::PacketFactory * packetFactory;
        network::Interface * networkInterface;
    };

    class DataBlockSender : public protocol::DataBlockSender
    {
        ClientServerInfo m_info;

    public:

        DataBlockSender( core::Allocator & allocator, protocol::Block & dataBlock, int fragmentSize, int fragmentsPerSecond )
            : protocol::DataBlockSender( allocator, dataBlock, fragmentSize, fragmentsPerSecond ) {}

        void SetInfo( const ClientServerInfo & info )
        {
            m_info = info;
        }

    protected:

        void SendFragment( int fragmentId, uint8_t * fragmentData, int fragmentBytes );
    };

    class DataBlockReceiver : public protocol::DataBlockReceiver
    {
        ClientServerInfo m_info;

    public:

        DataBlockReceiver( core::Allocator & allocator, int fragmentSize, int maxBlockSize )
            : protocol::DataBlockReceiver( allocator, fragmentSize, maxBlockSize ) {}

        void SetInfo( const ClientServerInfo & info )
        {
            m_info = info;
        }

    protected:

        void SendAck( int fragmentId );
    };
}

#endif
