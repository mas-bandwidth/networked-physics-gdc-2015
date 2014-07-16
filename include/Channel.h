/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_CHANNEL_H
#define PROTOCOL_CHANNEL_H

#include "Common.h"
#include "Allocator.h"
#include <functional>

namespace protocol
{
    /*
        Channel data is passed to the serializer per-packet
        and is how a channel specifies what data should be
        serialized in the connection packet.
    */

    class ChannelData : public Object
    {
    public:
        virtual ~ChannelData() {}
    };

    /*
        Implement this interface to integrate a custom channel
        inside a connection, see ReliableMessageChannel for a
        working example.
    */

    class Channel
    {
    public:

        virtual ~Channel()
        {
            // ...
        }

        virtual void Reset() = 0;

        virtual int GetError() const = 0;

        virtual ChannelData * GetData( uint16_t sequence ) = 0;

        virtual bool ProcessData( uint16_t sequence, ChannelData * data ) = 0;

        virtual void ProcessAck( uint16_t ack ) = 0;

        virtual void Update( const TimeBase & timeBase ) = 0;
    };

    /*  
        Many test cases don't need full functionality of a channel.
        Define a channel adapter here that stubs out all functionality
        so these tests are less verbose.
    */

    class ChannelAdapter : public Channel
    {
    public:

        void Reset() {}

        int GetError() const { return 0; }

        ChannelData * GetData( uint16_t sequence ) { return nullptr; }

        bool ProcessData( uint16_t sequence, ChannelData * data ) { return true; }

        void ProcessAck( uint16_t ack ) {}

        void Update( const TimeBase & timeBase ) {}
    };

    /*
        In order for two connections to be able to communicate with each
        other they must share the same channel structure precisely.

        This class describes that structure. Prior to being passed in
        to a connection, the channel structure must be locked, as the
        channel structure cannot change once a channel is using it.

        See ClientServerPackets.h or TestConnection.cpp for examples of use.
    */

    class ChannelStructure
    {
        Allocator * m_channelAllocator;
        Allocator * m_channelDataAllocator;
        int m_numChannels;
 
    public:

        ChannelStructure( Allocator & channelAllocator, Allocator & channelDataAllocator, int numChannels );

        ~ChannelStructure();

        int GetNumChannels() const
        {
            return m_numChannels;
        }

        const char * GetChannelName( int channelIndex ) const;

        Channel * CreateChannel( int channelIndex );

        ChannelData * CreateChannelData( int channelIndex );

        void DestroyChannel( Channel * channel );

        void DestroyChannelData( ChannelData * channelData );

        Allocator & GetChannelAllocator();

        Allocator & GetChannelDataAllocator();

    protected:

        virtual const char * GetChannelNameInternal( int channelIndex ) const = 0;
        virtual Channel * CreateChannelInternal( int channelIndex ) = 0;
        virtual ChannelData * CreateChannelDataInternal( int channelIndex ) = 0;
    };
}

#endif
