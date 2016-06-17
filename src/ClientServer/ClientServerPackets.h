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

#ifndef CLIENT_SERVER_PACKETS_H
#define CLIENT_SERVER_PACKETS_H

#include "protocol/Packet.h"
#include "protocol/Stream.h"
#include "protocol/Channel.h"
#include "protocol/PacketFactory.h"
#include "protocol/ProtocolEnums.h"
#include "core/Memory.h"

namespace clientServer
{
    enum Packets
    { 
        CLIENT_SERVER_PACKET_CONNECTION = protocol::CONNECTION_PACKET,

        // client -> server

        CLIENT_SERVER_PACKET_CONNECTION_REQUEST,                // client is requesting a connection.
        CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE,                // client response to server connection challenge.

        // server -> client

        CLIENT_SERVER_PACKET_CONNECTION_DENIED,                 // server denies request for connection. contains reason int, eg. full, closed etc.
        CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE,              // server response to client connection request.

        // bidirectional

        CLIENT_SERVER_PACKET_READY_FOR_CONNECTION,              // client/server are ready for connection packets. when both are ready the connection is established.
        CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT,               // a fragment of a data block being sent down.
        CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK,           // ack for a received data block fragment.
        CLIENT_SERVER_PACKET_DISCONNECTED,                      // courtesy packet sent in both directions to indicate that the client slot has been disconnected

        NUM_CLIENT_SERVER_NUM_PACKETS
    };

    struct ConnectionRequestPacket : public protocol::Packet
    {
        uint16_t clientId;

        ConnectionRequestPacket() : Packet( CLIENT_SERVER_PACKET_CONNECTION_REQUEST ) 
        {
            clientId = 0;
        }

        PROTOCOL_SERIALIZE_OBJECT( stream )
        {
            serialize_uint16( stream, clientId );
        }
    };

    struct ChallengeResponsePacket : public protocol::Packet
    {
        uint16_t clientId;
        uint16_t serverId;

        ChallengeResponsePacket() : Packet( CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE ) 
        {
            clientId = 0;
            serverId = 0;
        }

        PROTOCOL_SERIALIZE_OBJECT( stream )
        {
            serialize_uint16( stream, clientId );
            serialize_uint16( stream, serverId );
        }
    };

    struct ConnectionDeniedPacket : public protocol::Packet
    {
        uint16_t clientId;
        uint32_t reason;

        ConnectionDeniedPacket() : Packet( CLIENT_SERVER_PACKET_CONNECTION_DENIED )
        {
            clientId = 0;
            reason = 0;
        }

        PROTOCOL_SERIALIZE_OBJECT( stream )
        {
            serialize_uint16( stream, clientId );
            serialize_uint32( stream, reason );
        }
    };

    struct ConnectionChallengePacket : public protocol::Packet
    {
        uint16_t clientId;
        uint16_t serverId;

        ConnectionChallengePacket() : Packet( CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE )
        {
            clientId = 0;
            serverId = 0;
        }

        PROTOCOL_SERIALIZE_OBJECT( stream )
        {
            serialize_uint16( stream, clientId );
            serialize_uint16( stream, serverId );
        }
    };

    struct ReadyForConnectionPacket : public protocol::Packet
    {
        uint16_t clientId;
        uint16_t serverId;

        ReadyForConnectionPacket() : Packet( CLIENT_SERVER_PACKET_READY_FOR_CONNECTION ) 
        {
            clientId = 0;
            serverId = 0;
        }

        PROTOCOL_SERIALIZE_OBJECT( stream )
        {
            serialize_uint16( stream, clientId );
            serialize_uint16( stream, serverId );
        }
    };

    struct DataBlockFragmentPacket : public protocol::Packet
    {
        uint16_t clientId;
        uint16_t serverId;
        uint32_t blockSize;
        uint32_t fragmentSize;
        uint32_t numFragments;
        uint32_t fragmentId;
        uint32_t fragmentBytes;
        uint8_t * fragmentData;

        DataBlockFragmentPacket() : Packet( CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT ) 
        {
            clientId = 0;
            serverId = 0;
            blockSize = 0;
            fragmentSize = 0;
            numFragments = 0;
            fragmentId = 0;
            fragmentBytes = 0;
            fragmentData = NULL;
        }

        ~DataBlockFragmentPacket()
        {
            if ( fragmentData )
            {
                core::memory::scratch_allocator().Free( fragmentData );
                fragmentData = nullptr;
            }
        }

        PROTOCOL_SERIALIZE_OBJECT( stream )
        {
            if ( Stream::IsWriting )
                CORE_ASSERT( fragmentSize <= protocol::MaxFragmentSize );

            serialize_uint16( stream, clientId );
            serialize_uint16( stream, serverId );
            serialize_uint32( stream, blockSize );
            serialize_bits( stream, numFragments, 16 );
            serialize_bits( stream, fragmentSize, 16 );
            serialize_bits( stream, fragmentId, 16 );
            serialize_bits( stream, fragmentBytes, 16 );        // actual fragment bytes included in this packed. may be *less* than fragment size!

            if ( Stream::IsReading )
            {
                CORE_ASSERT( fragmentSize <= protocol::MaxFragmentSize );
                if ( fragmentSize <= protocol::MaxFragmentSize )
                    fragmentData = (uint8_t*) core::memory::scratch_allocator().Allocate( fragmentBytes );
            }

            CORE_ASSERT( fragmentData );

            serialize_bytes( stream, fragmentData, fragmentBytes );
        }
    };

    struct DataBlockFragmentAckPacket : public protocol::Packet
    {
        uint16_t clientId;
        uint16_t serverId;
        uint32_t fragmentId;

        DataBlockFragmentAckPacket() : Packet( CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK ) 
        {
            clientId = 0;
            serverId = 0;
            fragmentId = 0;
        }

        PROTOCOL_SERIALIZE_OBJECT( stream )
        {
            serialize_uint16( stream, clientId );
            serialize_uint16( stream, serverId );
            serialize_bits( stream, fragmentId, 16 );
        }
    };

    struct DisconnectedPacket : public protocol::Packet
    {
        uint16_t clientId;
        uint16_t serverId;

        DisconnectedPacket() : Packet( CLIENT_SERVER_PACKET_DISCONNECTED )
        {
            clientId = 0;
            serverId = 0;
        }

        PROTOCOL_SERIALIZE_OBJECT( stream )
        {
            serialize_uint16( stream, clientId );
            serialize_uint16( stream, serverId );
        }
    };
}

#endif
