/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_PACKETS_H
#define PROTOCOL_PACKETS_H

#include "Packet.h"
#include "Stream.h"
#include "Channel.h"
#include "Factory.h"

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
        uint64_t clientGuid = 0;

        ConnectionRequestPacket() : Packet( PACKET_CONNECTION_REQUEST ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
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
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ChallengeResponsePacket() : Packet( PACKET_CHALLENGE_RESPONSE ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
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
        uint64_t clientGuid = 0;
        uint32_t reason = 0;

        ConnectionDeniedPacket() : Packet( PACKET_CONNECTION_DENIED ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
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
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ConnectionChallengePacket() : Packet( PACKET_CONNECTION_CHALLENGE ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
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
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        RequestClientDataPacket() : Packet( PACKET_REQUEST_CLIENT_DATA ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
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
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        ReadyForConnectionPacket() : Packet( PACKET_READY_FOR_CONNECTION ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
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
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;

        DisconnectedPacket() : Packet( PACKET_DISCONNECTED ) {}

        template <typename Stream> void Serialize( Stream & stream )
        {
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

    struct ConnectionPacket : public Packet
    {
        uint16_t sequence = 0;
        uint16_t ack = 0;
        uint32_t ack_bits = 0;
        int numChannels = 0;
        ChannelData * channelData[MaxChannels];
        ChannelStructure * channelStructure = nullptr;

        ConnectionPacket( int type, ChannelStructure * _channelStructure ) : Packet( type )
        {
//            printf( "ConnectionPacket\n" );
            assert( _channelStructure );
            channelStructure = _channelStructure;
            numChannels = channelStructure->GetNumChannels();
            memset( channelData, 0, sizeof( ChannelData* ) * numChannels );
        }

        ~ConnectionPacket()
        {
//            printf( "~ConnectionPacket\n" );

            assert( channelStructure );

            for ( int i = 0; i < numChannels; ++i )
            {
                if ( channelData[i] )
                {
//                    printf( "connection packet delete channel data\n" );
                    delete channelData[i];
                    channelData[i] = nullptr;
                }
            }

            channelStructure = nullptr;
        }

        template <typename Stream> void Serialize( Stream & stream )
        {
            assert( channelStructure );

            // IMPORTANT: Insert non-frequently changing values here
            // This helps LZ dictionary based compressors do a good job!

            bool perfect;
            if ( Stream::IsWriting )
                 perfect = ack_bits == 0xFFFFFFFF;

            serialize_bool( stream, perfect );

            if ( !perfect )
                serialize_bits( stream, ack_bits, 32 );
            else
                ack_bits = 0xFFFFFFFF;

            stream.Align();

            if ( Stream::IsWriting )
            {
                for ( int i = 0; i < numChannels; ++i )
                {
                    bool has_data = channelData[i] != nullptr;
                    serialize_bool( stream, has_data );
                }
            }
            else                
            {
                for ( int i = 0; i < numChannels; ++i )
                {
                    bool has_data;
                    serialize_bool( stream, has_data );
                    if ( has_data )
                    {
                        channelData[i] = channelStructure->CreateChannelData( i );
                        assert( channelData[i] );
                    }
                }
            }

            // IMPORTANT: Insert frequently changing values below

            serialize_bits( stream, sequence, 16 );

            int ack_delta = 0;
            bool ack_in_range = false;

            if ( Stream::IsWriting )
            {
                if ( ack < sequence )
                    ack_delta = sequence - ack;
                else
                    ack_delta = (int)sequence + 65536 - ack;

                assert( ack_delta > 0 );
                
                ack_in_range = ack_delta <= 128;
            }

            serialize_bool( stream, ack_in_range );
    
            if ( ack_in_range )
            {
                serialize_int( stream, ack_delta, 1, 128 );
                if ( Stream::IsReading )
                    ack = sequence - ack_delta;
            }
            else
                serialize_bits( stream, ack, 16 );

            // now serialize per-channel data

            for ( int i = 0; i < numChannels; ++i )
            {
                if ( channelData[i] )
                {
                    stream.Align();

                    serialize_object( stream, *channelData[i] );
                }
            }
        }

        void SerializeRead( ReadStream & stream )
        {
            return Serialize( stream );
        }

        void SerializeWrite( WriteStream & stream )
        {
            return Serialize( stream );
        }

        void SerializeMeasure( MeasureStream & stream )
        {
            return Serialize( stream );
        }

        bool operator ==( const ConnectionPacket & other ) const
        {
            return sequence == other.sequence &&
                        ack == other.ack &&
                   ack_bits == other.ack_bits;
        }

        bool operator !=( const ConnectionPacket & other ) const
        {
            return !( *this == other );
        }

    private:

        ConnectionPacket( const ConnectionPacket & other );
        const ConnectionPacket & operator = ( const ConnectionPacket & other );

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
