// Client Server Library - Copyright (c) 2008-2015, Glenn Fiedler

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
