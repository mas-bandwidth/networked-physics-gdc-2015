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

        template <typename Stream> void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
        }

        void SerializeRead( ReadStream & stream )
        {
            Serialize( stream );
        }

        void SerializeWrite( WriteStream & stream )
        {
            Serialize( stream );
        }
    };

    struct ChallengeResponsePacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ChallengeResponsePacket() : Packet( PACKET_ChallengeResponse ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
            serialize_uint64( stream, serverGuid );
        }

        void SerializeRead( ReadStream & stream )
        {
            Serialize( stream );
        }

        void SerializeWrite( WriteStream & stream )
        {
            Serialize( stream );
        }
    };

    struct ConnectionDeniedPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint32_t reason = 0;

        ConnectionDeniedPacket() : Packet( PACKET_ConnectionDenied ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
            serialize_uint32( stream, reason );
        }

        void SerializeRead( ReadStream & stream )
        {
            Serialize( stream );
        }

        void SerializeWrite( WriteStream & stream )
        {
            Serialize( stream );
        }
    };

    struct ConnectionChallengePacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ConnectionChallengePacket() : Packet( PACKET_ConnectionChallenge ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
            serialize_uint64( stream, serverGuid );
        }

        void SerializeRead( ReadStream & stream )
        {
            Serialize( stream );
        }

        void SerializeWrite( WriteStream & stream )
        {
            Serialize( stream );
        }
    };

    struct RequestClientDataPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        RequestClientDataPacket() : Packet( PACKET_RequestClientData ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
            serialize_uint64( stream, serverGuid );
        }

        void SerializeRead( ReadStream & stream )
        {
            Serialize( stream );
        }

        void SerializeWrite( WriteStream & stream )
        {
            Serialize( stream );
        }
    };

    struct ReadyForConnectionPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ReadyForConnectionPacket() : Packet( PACKET_ReadyForConnection ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
            serialize_uint64( stream, serverGuid );
        }

        void SerializeRead( ReadStream & stream )
        {
            Serialize( stream );
        }

        void SerializeWrite( WriteStream & stream )
        {
            Serialize( stream );
        }
    };

    struct DisconnectedPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        DisconnectedPacket() : Packet( PACKET_Disconnected ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
            serialize_uint64( stream, protocolId );
            serialize_uint64( stream, clientGuid );
            serialize_uint64( stream, serverGuid );
        }

        void SerializeRead( ReadStream & stream )
        {
            Serialize( stream );
        }

        void SerializeWrite( WriteStream & stream )
        {
            Serialize( stream );
        }
    };

    class ClientServerPacketFactory : public Factory<Packet>
    {
    public:

        ClientServerPacketFactory( ChannelStructure * channelStructure )
        {
            assert( channelStructure );

            // client -> server packets
            Register( PACKET_ConnectionRequest,  [] { return new ConnectionRequestPacket();  } );
            Register( PACKET_ChallengeResponse,  [] { return new ChallengeResponsePacket();  } );
            Register( PACKET_ReadyForConnection, [] { return new ReadyForConnectionPacket(); } );

            // server -> client packets
            Register( PACKET_ConnectionDenied, [] { return new ConnectionDeniedPacket(); } );
            Register( PACKET_ConnectionChallenge, [] { return new ConnectionChallengePacket(); } );
            Register( PACKET_RequestClientData, [] { return new RequestClientDataPacket(); } );
            Register( PACKET_Disconnected, [] { return new DisconnectedPacket(); } );

            // bidirectional packets
            Register( PACKET_Connection, [channelStructure] { return new ConnectionPacket( PACKET_Connection, channelStructure ); } );
        }
    };
}

#endif
