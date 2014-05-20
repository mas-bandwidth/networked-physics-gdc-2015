/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_MESSAGE_CHANNEL_H
#define PROTOCOL_MESSAGE_CHANNEL_H

#include "Common.h"
#include "Channel.h"
#include "Message.h"

namespace protocol
{
    class MessageChannel : public Channel
    {
    public:

        virtual bool CanSendMessage() const = 0;

        virtual void SendMessage( shared_ptr<Message> message ) = 0;

        virtual void SendBlock( shared_ptr<Block> block ) = 0;

        virtual shared_ptr<Message> ReceiveMessage() = 0;
    };
}

#endif
