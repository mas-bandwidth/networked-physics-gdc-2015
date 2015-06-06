// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

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
