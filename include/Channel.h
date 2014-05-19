/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_CHANNEL_H
#define PROTOCOL_CHANNEL_H

#include "Common.h"

namespace protocol
{
    class ChannelData : public Object
    {
        // ...
    };

    class Channel
    {
    public:

        virtual ~Channel()
        {
            // ...
        }

        virtual shared_ptr<ChannelData> CreateData() = 0;

        virtual shared_ptr<ChannelData> GetData( uint16_t sequence ) = 0;

        virtual void ProcessData( uint16_t sequence, shared_ptr<ChannelData> data ) = 0;

        virtual void ProcessAck( uint16_t ack ) = 0;

        virtual void Update( const TimeBase & timeBase ) = 0;
    };
}

#endif
