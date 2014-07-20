/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_PACKETS_H
#define PROTOCOL_PACKETS_H

#include "Packet.h"
#include "Stream.h"
#include "Channel.h"
#include "PacketFactory.h"
#include "Memory.h"

namespace protocol
{
    enum ClientServerPackets
    { 
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

        CLIENT_SERVER_PACKET_CONNECTION,                        // connection packet send both directions once connection established (Connection.cpp)

        NUM_CLIENT_SERVER_PACKETS
    };

    struct ConnectionRequestPacket : public Packet
    {
        uint64_t clientGuid = 0;

        ConnectionRequestPacket() : Packet( CLIENT_SERVER_PACKET_CONNECTION_REQUEST ) {}

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

        ChallengeResponsePacket() : Packet( CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE ) {}

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

        ConnectionDeniedPacket() : Packet( CLIENT_SERVER_PACKET_CONNECTION_DENIED ) {}

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

        ConnectionChallengePacket() : Packet( CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE ) {}

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

        ReadyForConnectionPacket() : Packet( CLIENT_SERVER_PACKET_READY_FOR_CONNECTION ) {}

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

    struct DataBlockFragmentPacket : public Packet
    {
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;
        uint32_t totalSize = 0;
        uint32_t fragmentSize : 16;
        uint32_t fragmentId : 16;
        uint32_t fragmentBytes : 16;
        uint8_t * fragmentData = nullptr;

        DataBlockFragmentPacket() : Packet( CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT ) 
        {
            fragmentId = 0;
            fragmentSize = 0;
        }

        ~DataBlockFragmentPacket()
        {
            if ( fragmentData )
            {
                memory::scratch_allocator().Free( fragmentData );
                fragmentData = nullptr;
            }
        }

        template <typename Stream> void Serialize( Stream & stream )
        {
            if ( Stream::IsWriting )
                PROTOCOL_ASSERT( fragmentSize <= MaxFragmentSize );

            serialize_uint64( stream, clientGuid );
            serialize_uint64( stream, serverGuid );
            serialize_uint32( stream, totalSize );
            serialize_bits( stream, fragmentSize, 16 );         // fragment size, as per-config
            serialize_bits( stream, fragmentId, 16 );
            serialize_bits( stream, fragmentBytes, 16 );        // fragment bytes included in this packed. may be *less* than fragment size!

            if ( Stream::IsReading )
            {
                PROTOCOL_ASSERT( fragmentSize <= MaxFragmentSize );
                if ( fragmentSize <= MaxFragmentSize )
                    fragmentData = (uint8_t*) memory::scratch_allocator().Allocate( fragmentBytes );
            }

            PROTOCOL_ASSERT( fragmentData );

            serialize_bytes( stream, fragmentData, fragmentBytes );
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

    struct DataBlockFragmentAckPacket : public Packet
    {
        uint64_t clientGuid = 0;
        uint64_t serverGuid = 0;
        uint32_t fragmentId : 16;

        DataBlockFragmentAckPacket() : Packet( CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK ) 
        {
            fragmentId = 0;
        }

        template <typename Stream> void Serialize( Stream & stream )
        {
            serialize_uint64( stream, clientGuid );
            serialize_uint64( stream, serverGuid );
            serialize_bits( stream, fragmentId, 16 );
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

        DisconnectedPacket() : Packet( CLIENT_SERVER_PACKET_DISCONNECTED ) {}

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
            PROTOCOL_ASSERT( _channelStructure );
            channelStructure = _channelStructure;
            numChannels = channelStructure->GetNumChannels();
            memset( channelData, 0, sizeof( ChannelData* ) * numChannels );
        }

    private:

        ~ConnectionPacket()
        {
            PROTOCOL_ASSERT( channelStructure );

            for ( int i = 0; i < numChannels; ++i )
            {
                if ( channelData[i] )
                {
                    PROTOCOL_DELETE( memory::scratch_allocator(), ChannelData, channelData[i] );
                    channelData[i] = nullptr;
                }
            }

            channelStructure = nullptr;
        }

    public:

        template <typename Stream> void Serialize( Stream & stream )
        {
            PROTOCOL_ASSERT( channelStructure );

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
                        PROTOCOL_ASSERT( channelData[i] );
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

                PROTOCOL_ASSERT( ack_delta > 0 );
                
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
}

#endif
