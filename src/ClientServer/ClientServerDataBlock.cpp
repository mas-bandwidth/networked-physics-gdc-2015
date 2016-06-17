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

#include "ClientServerDataBlock.h"
#include "ClientServerPackets.h"
#include "network/Interface.h"

namespace clientServer
{
    void DataBlockSender::SendFragment( int fragmentId, uint8_t * fragmentData, int fragmentBytes )
    {
        auto packet = (DataBlockFragmentPacket*) m_info.packetFactory->Create( CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT );

        packet->clientId = m_info.clientId;
        packet->serverId = m_info.serverId;
        packet->blockSize = GetBlockSize();
        packet->fragmentSize = GetFragmentSize();
        packet->numFragments = GetNumFragments();
        packet->fragmentId = fragmentId;
        packet->fragmentBytes = fragmentBytes;
        packet->fragmentData = (uint8_t*) core::memory::scratch_allocator().Allocate( packet->fragmentSize );
        memcpy( packet->fragmentData, fragmentData, fragmentBytes );

        m_info.networkInterface->SendPacket( m_info.address, packet );
    }

    void DataBlockReceiver::SendAck( int fragmentId )
    {
        auto packet = (DataBlockFragmentAckPacket*) m_info.packetFactory->Create( CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK );
        if ( !packet )
            return;

        packet->clientId = m_info.clientId;
        packet->serverId = m_info.serverId;
        packet->fragmentId = fragmentId;

        m_info.networkInterface->SendPacket( m_info.address, packet );
    }
}
