// Client Server Library - Copyright (c) 2008-2015, Glenn Fiedler

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
