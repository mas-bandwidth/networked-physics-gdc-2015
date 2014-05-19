/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_UNRELIABLE_MESSAGE_CHANNEL_H
#define PROTOCOL_UNRELIABLE_MESSAGE_CHANNEL_H

#include "MessageChannel.h"    

namespace protocol
{
    // todo: once reliable channel is finished -- port over here and create an unreliable
    // message channel which can be placed at the end of the channels, and serializes as many
    // messages into the packet that can fit, and discards the rest.
}

#endif
