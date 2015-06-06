// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef PROTOCOL_MESSAGE_CHANNEL_H
#define PROTOCOL_MESSAGE_CHANNEL_H

#include "protocol/Channel.h"
#include "protocol/Message.h"
#include "core/Core.h"

namespace protocol
{
    class MessageChannel : public Channel
    {
    public:

        virtual bool CanSendMessage() const = 0;

        virtual void SendMessage( Message * message ) = 0;          // takes ownership of message pointer

        virtual void SendBlock( Block & block ) = 0;                // takes ownership of block data

        virtual Message * ReceiveMessage() = 0;                     // gives ownership of message pointer to *you*
    };
}

#endif
