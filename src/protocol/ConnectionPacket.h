// Protocol Library - Copyright (c) 2008-2015, The Network Protocol Company, Inc.

#ifndef PROTOCOL_CONNECTION_PACKET_H
#define PROTOCOL_CONNECTION_PACKET_H

#include "Packet.h"
#include "Stream.h"
#include "Channel.h"
#include "Memory.h"
#include "PacketFactory.h"

// todo: protocol should not depend on client/server
#include "clientServer/ClientServerContext.h"
#include "clientServer/ClientServerEnums.h"

namespace protocol
{
    struct ConnectionPacket : public Packet
    {
        uint16_t clientId = 0;
        uint16_t serverId = 0;
        uint16_t sequence = 0;
        uint16_t ack = 0;
        uint32_t ack_bits = 0;
        ChannelData * channelData[MaxChannels];

        ConnectionPacket() : Packet( CONNECTION_PACKET )
        {
            memset( channelData, 0, sizeof( ChannelData* ) * MaxChannels );
        }

    private:

        ~ConnectionPacket()
        {
            for ( int i = 0; i < MaxChannels; ++i )
            {
                if ( channelData[i] )
                {
                    CORE_DELETE( core::memory::scratch_allocator(), ChannelData, channelData[i] );
                    channelData[i] = nullptr;
                }
            }
        }

    public:

        PROTOCOL_SERIALIZE_OBJECT( stream )
        {
            // IMPORTANT: Channel structure must be supplied as context
            // so we know what channels are to be serialized for this packet

            ChannelStructure * channelStructure = (ChannelStructure*) stream.GetContext( CONTEXT_CONNECTION );
            
            CORE_ASSERT( channelStructure );

            const int numChannels = channelStructure->GetNumChannels();

            CORE_ASSERT( numChannels > 0 );
            CORE_ASSERT( numChannels <= MaxChannels );

            // todo: protocol should not depend on client/server

            // IMPORTANT: Context here is used when running under client/server
            // so we can filter out connection packets that do not match any connected client.

            auto clientServerContext = (const clientServer::ClientServerContext*) stream.GetContext( clientServer::CONTEXT_CLIENT_SERVER );

            if ( clientServerContext )
            {
                serialize_uint16( stream, clientId );
                serialize_uint16( stream, serverId );

                if ( Stream::IsReading && !clientServerContext->ClientPotentiallyExists( clientId, serverId ) )
                {
                    clientId = 0;
                    serverId = 0;
                    stream.Abort();
                    return;                        
                }
            }

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
                        CORE_ASSERT( channelData[i] );
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

                CORE_ASSERT( ack_delta > 0 );
                
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
