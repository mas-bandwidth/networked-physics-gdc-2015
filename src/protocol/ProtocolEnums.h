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

#ifndef PROTOCOL_ENUMS_H
#define PROTOCOL_ENUMS_H

#include "core/Core.h"

namespace protocol
{
    enum ConnectionCounters
    {
        CONNECTION_COUNTER_PACKETS_READ,                        // number of packets read
        CONNECTION_COUNTER_PACKETS_WRITTEN,                     // number of packets written
        CONNECTION_COUNTER_PACKETS_ACKED,                       // number of packets acked
        CONNECTION_COUNTER_PACKETS_DISCARDED,                   // number of read packets that we discarded (eg. not acked)
        CONNECTION_COUNTER_NUM_COUNTERS
    };

    enum ConnectionError
    {
        CONNECTION_ERROR_NONE = 0,
        CONNECTION_ERROR_CHANNEL
    };

    enum ReliableMessageChannelError
    {
        RELIABLE_MESSAGE_CHANNEL_ERROR_NONE = 0,
        RELIABLE_MESSAGE_CHANNEL_ERROR_SEND_QUEUE_FULL
    };

    enum ReliableMessageChannelCounters
    {
        RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT,
        RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_WRITTEN,
        RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_READ,
        RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED,
        RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_LATE,
        RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY,
        RELIABLE_MESSAGE_CHANNEL_COUNTER_NUM_COUNTERS
    };

    enum DataBlockReceiverError
    {
        DATA_BLOCK_RECEIVER_ERROR_NONE = 0,
        DATA_BLOCK_RECEIVER_ERROR_BLOCK_TOO_LARGE
    };

    enum Contexts
    {
        CONTEXT_CONNECTION = 0,
    };

    enum Packets
    {
        CONNECTION_PACKET = 0
    };
}

#endif
