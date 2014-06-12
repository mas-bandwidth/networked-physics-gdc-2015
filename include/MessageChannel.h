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

        virtual void SendMessage( Message * message ) = 0;          // takes ownership of message pointer

        virtual void SendBlock( Block * block ) = 0;                // takes ownership of block pointer

        virtual Message * ReceiveMessage() = 0;                     // gives ownership of message pointer to *you*
    };
}

#endif
