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

        PACKET_CONNECTION_REQUEST,                              // client is requesting a connection.
        PACKET_CHALLENGE_RESPONSE,                              // client response to server connection challenge.
        PACKET_READY_FOR_CONNECTION,                            // client is ready for connection packets

        // server -> client

        PACKET_CONNECTION_DENIED,                               // server denies request for connection. contains reason int, eg. full, closed etc.
        PACKET_CONNECTION_CHALLENGE,                            // server response to client connection request.
        PACKET_REQUEST_CLIENT_DATA,                             // server is ready for the client to start sending data to it.
        PACKET_DISCONNECTED,                                    // courtesy packet from server to tell the client they have been disconnected. sent for one second after server-side disconnect

        // bidirectional

        PACKET_CONNECTION
    };

    struct ConnectionRequestPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;

        ConnectionRequestPacket() : Packet( PACKET_CONNECTION_REQUEST ) {}

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
    
        void SerializeMeasure( MeasureStream & stream )
        {
            Serialize( stream );
        }
    };

    struct ChallengeResponsePacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ChallengeResponsePacket() : Packet( PACKET_CHALLENGE_RESPONSE ) {}

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
    
        void SerializeMeasure( MeasureStream & stream )
        {
            Serialize( stream );
        }
    };

    struct ConnectionDeniedPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint32_t reason = 0;

        ConnectionDeniedPacket() : Packet( PACKET_CONNECTION_DENIED ) {}

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
    
        void SerializeMeasure( MeasureStream & stream )
        {
            Serialize( stream );
        }
    };

    struct ConnectionChallengePacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ConnectionChallengePacket() : Packet( PACKET_CONNECTION_CHALLENGE ) {}

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
    
        void SerializeMeasure( MeasureStream & stream )
        {
            Serialize( stream );
        }
    };

    struct RequestClientDataPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        RequestClientDataPacket() : Packet( PACKET_REQUEST_CLIENT_DATA ) {}

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
    
        void SerializeMeasure( MeasureStream & stream )
        {
            Serialize( stream );
        }
    };

    struct ReadyForConnectionPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ReadyForConnectionPacket() : Packet( PACKET_READY_FOR_CONNECTION ) {}

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
    
        void SerializeMeasure( MeasureStream & stream )
        {
            Serialize( stream );
        }
    };

    struct DisconnectedPacket : public Packet
    {
        uint64_t protocolId = 0;
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        DisconnectedPacket() : Packet( PACKET_DISCONNECTED ) {}

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
    
        void SerializeMeasure( MeasureStream & stream )
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

            Register( PACKET_CONNECTION_REQUEST,   [] { return new ConnectionRequestPacket();  } );
            Register( PACKET_CHALLENGE_RESPONSE,   [] { return new ChallengeResponsePacket();  } );
            Register( PACKET_READY_FOR_CONNECTION, [] { return new ReadyForConnectionPacket(); } );

            Register( PACKET_CONNECTION_DENIED,    [] { return new ConnectionDeniedPacket(); } );
            Register( PACKET_CONNECTION_CHALLENGE, [] { return new ConnectionChallengePacket(); } );
            Register( PACKET_REQUEST_CLIENT_DATA,  [] { return new RequestClientDataPacket(); } );
            Register( PACKET_DISCONNECTED,         [] { return new DisconnectedPacket(); } );

            Register( PACKET_CONNECTION, [channelStructure] { return new ConnectionPacket( PACKET_CONNECTION, channelStructure ); } );
        }
    };
}

#endif
