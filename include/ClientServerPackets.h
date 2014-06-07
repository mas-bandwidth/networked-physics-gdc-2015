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
        PACKET_ReadyForConnection,                              // client is ready for connection packets

        // server -> client

        PACKET_ConnectionDenied,                                // server denies request for connection. contains reason int, eg. full, closed etc.
        PACKET_ConnectionChallenge,                             // server response to client connection request.
        PACKET_RequestClientData,                               // server is ready for the client to start sending data to it.
        PACKET_Disconnected,                                    // courtesy packet from server to tell the client they have been disconnected. sent for one second after server-side disconnect

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

    struct RequestClientDataPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        RequestClientDataPacket() : Packet( PACKET_RequestClientData ) {}

        void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
            serialize_uint64( stream, serverGuid );
        }
    };

    struct ReadyForConnectionPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ReadyForConnectionPacket() : Packet( PACKET_ReadyForConnection ) {}

        void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
            serialize_uint64( stream, serverGuid );
        }
    };

    struct DisconnectedPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        DisconnectedPacket() : Packet( PACKET_Disconnected ) {}

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
            Register( PACKET_ReadyForConnection, [] { return make_shared<ReadyForConnectionPacket>(); } );

            // server -> client messages
            Register( PACKET_ConnectionDenied, [] { return make_shared<ConnectionDeniedPacket>(); } );
            Register( PACKET_ConnectionChallenge, [] { return make_shared<ConnectionChallengePacket>(); } );
            Register( PACKET_RequestClientData, [] { return make_shared<RequestClientDataPacket>(); } );
            Register( PACKET_Disconnected, [] { return make_shared<DisconnectedPacket>(); } );

            // bidirectional messages
            Register( PACKET_Connection, [channelStructure] { return make_shared<ConnectionPacket>( PACKET_Connection, channelStructure ); } );
        }
    };
}

#endif
