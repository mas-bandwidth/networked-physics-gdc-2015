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

#ifndef PROTOCOL_CHANNEL_H
#define PROTOCOL_CHANNEL_H

#include "core/Core.h"
#include "protocol/Object.h"

namespace core { class Allocator; }

namespace protocol
{
    class ChannelData : public Object
    {
        // ...
    };

    class Channel
    {
    public:

        Channel()
        {
            m_context = NULL;
        }

        virtual ~Channel() {}

        virtual void Reset() = 0;

        virtual int GetError() const = 0;

        virtual ChannelData * GetData( uint16_t sequence ) = 0;

        virtual bool ProcessData( uint16_t sequence, ChannelData * data ) = 0;

        virtual void ProcessAck( uint16_t ack ) = 0;

        virtual void Update( const core::TimeBase & timeBase ) = 0;

        void SetContext( const void ** context )
        {
            m_context = context;
        }

    protected:

        const void ** GetContext() const { return m_context; }

    private:

        const void ** m_context;
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

        ChannelData * GetData( uint16_t /*sequence*/ ) { return NULL; }

        bool ProcessData( uint16_t /*sequence*/, ChannelData * /*data*/ ) { return true; }

        void ProcessAck( uint16_t /*ack*/ ) {}

        void Update( const core::TimeBase & /*timeBase*/ ) {}
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
        core::Allocator * m_channelAllocator;
        core::Allocator * m_channelDataAllocator;
        int m_numChannels;
 
    public:

        ChannelStructure( core::Allocator & channelAllocator, core::Allocator & channelDataAllocator, int numChannels );

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

        core::Allocator & GetChannelAllocator();

        core::Allocator & GetChannelDataAllocator();

    protected:

        virtual const char * GetChannelNameInternal( int channelIndex ) const = 0;
        virtual Channel * CreateChannelInternal( int channelIndex ) = 0;
        virtual ChannelData * CreateChannelDataInternal( int channelIndex ) = 0;
    };
}

#endif
