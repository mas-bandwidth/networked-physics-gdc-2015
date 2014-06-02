/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_CLIENT_SERVER_PACKETS_H
#define PROTOCOL_CLIENT_SERVER_PACKETS_H

#include "Packet.h"
#include "Stream.h"
#include "Connection.h"
#include "Channel.h"

namespace protocol
{
    enum ClientServerPackets 
    { 
        // client -> server

        PACKET_ConnectionRequest,                               // client is requesting a connection.
        PACKET_ChallengeResponse,                               // client response to server connection challenge.
        PACKET_AckServerBlockFragment,                          // client received a data block fragment from server. ack it.
        PACKET_SendClientBlockFragment,                         // client sending data block fragment to server.

        // server -> client

        PACKET_ConnectionDenied,                                // server denies request for connection. contains reason int, eg. full, closed etc.
        PACKET_ConnectionChallenge,                             // server response to client connection request.
        PACKET_SendServerBlockFragment,                         // server sending a data block fragment down to client.
        PACKET_AckClientBlockFragment,                          // server received a data block fragment from client. ack it.
        PACKET_Disconnected,                                    // server is telling the client they have been disconnected. sent for one second on disconnect

        // bidirectional

        PACKET_Connection
    };

    struct ConnectionRequestPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;

        ConnectionRequestPacket() : Packet( PACKET_ConnectionRequest ) {}

        void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
        }
    };

    struct ChallengeResponsePacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ChallengeResponsePacket() : Packet( PACKET_ChallengeResponse ) {}

        void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
            serialize_uint64( stream, serverGuid );
        }
    };

    struct ConnectionDeniedPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint32_t reason = 0;

        ConnectionDeniedPacket() : Packet( PACKET_ConnectionDenied ) {}

        void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
            serialize_uint32( stream, reason );
        }
    };

    struct ConnectionChallengePacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ConnectionChallengePacket() : Packet( PACKET_ConnectionChallenge ) {}

        void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
            serialize_uint64( stream, serverGuid );
        }
    };

    class ClientServerPacketFactory : public Factory<Packet>
    {
    public:

        ClientServerPacketFactory( shared_ptr<ChannelStructure> channelStructure )
        {
            // client -> server messages
            Register( PACKET_ConnectionRequest, [] { return make_shared<ConnectionRequestPacket>(); } );
            Register( PACKET_ChallengeResponse, [] { return make_shared<ChallengeResponsePacket>(); } );

            // server -> client messages
            Register( PACKET_ConnectionDenied, [] { return make_shared<ConnectionDeniedPacket>(); } );
            Register( PACKET_ConnectionChallenge, [] { return make_shared<ConnectionChallengePacket>(); } );

            // bidirectional messages
            Register( PACKET_Connection, [channelStructure] { return make_shared<ConnectionPacket>( PACKET_Connection, channelStructure ); } );
        }
    };
}

#endif
