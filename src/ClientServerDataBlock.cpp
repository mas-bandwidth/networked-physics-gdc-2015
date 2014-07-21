/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#include "ClientServerDataBlock.h"
#include "NetworkInterface.h"
#include "Packets.h"

namespace protocol
{
    void ClientServerDataBlockSender::SendFragment( int fragmentId, uint8_t * fragmentData, int fragmentBytes )
    {
        auto packet = (DataBlockFragmentPacket*) m_info.packetFactory->Create( CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT );

        packet->clientGuid = m_info.clientGuid;
        packet->serverGuid = m_info.serverGuid;
        packet->totalSize = GetBlockSize();
        packet->fragmentSize = GetFragmentSize();
        packet->fragmentId = fragmentId;
        packet->fragmentBytes = fragmentBytes;
        packet->fragmentData = (uint8_t*) memory::scratch_allocator().Allocate( packet->fragmentSize );
        memcpy( packet->fragmentData, fragmentData, fragmentBytes );

        m_info.networkInterface->SendPacket( m_info.address, packet );
    }

    void ClientServerDataBlockReceiver::SendAck( int fragmentId )
    {
        auto packet = (DataBlockFragmentAckPacket*) m_info.packetFactory->Create( CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK );
        if ( !packet )
            return;

        packet->clientGuid = m_info.clientGuid;
        packet->serverGuid = m_info.serverGuid;
        packet->fragmentId = fragmentId;

        m_info.networkInterface->SendPacket( m_info.address, packet );
    }
}
