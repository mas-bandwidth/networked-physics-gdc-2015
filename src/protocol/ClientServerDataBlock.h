// Protocol Library - Copyright (c) 2014, The Network Protocol Company, Inc.

#ifndef PROTOCOL_CLIENT_SERVER_DATA_BLOCK_H
#define PROTOCOL_CLIENT_SERVER_DATA_BLOCK_H

#include "network/Address.h"
#include "protocol/DataBlockSender.h"
#include "protocol/DataBlockReceiver.h"

namespace core { class Allocator; }
namespace network { class Interface; }

namespace protocol
{
    class Block;
    class PacketFactory;

    struct ClientServerInfo
    {
        network::Address address;
        uint16_t clientId;
        uint16_t serverId;
        PacketFactory * packetFactory;
        network::Interface * networkInterface;
    };

    class ClientServerDataBlockSender : public DataBlockSender
    {
        ClientServerInfo m_info;

    public:

        ClientServerDataBlockSender( core::Allocator & allocator, Block & dataBlock, int fragmentSize, int fragmentsPerSecond )
            : DataBlockSender( allocator, dataBlock, fragmentSize, fragmentsPerSecond ) {}

        void SetInfo( const ClientServerInfo & info )
        {
            m_info = info;
        }

    protected:

        void SendFragment( int fragmentId, uint8_t * fragmentData, int fragmentBytes );
    };

    class ClientServerDataBlockReceiver : public DataBlockReceiver
    {
        ClientServerInfo m_info;

    public:

        ClientServerDataBlockReceiver( core::Allocator & allocator, int fragmentSize, int maxBlockSize )
            : DataBlockReceiver( allocator, fragmentSize, maxBlockSize ) {}

        void SetInfo( const ClientServerInfo & info )
        {
            m_info = info;
        }

    protected:

        void SendAck( int fragmentId );
    };
}

#endif
