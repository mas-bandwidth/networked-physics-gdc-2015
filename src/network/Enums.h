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

#ifndef NETWORK_ENUMS_H
#define NETWORK_ENUMS_H

#include "core/Core.h"

namespace network
{
    enum AddressType
    {
        ADDRESS_UNDEFINED,
        ADDRESS_IPV4,
        ADDRESS_IPV6
    };

    enum BSDSocketError
    {
        BSD_SOCKET_ERROR_NONE = 0,
        BSD_SOCKET_ERROR_CREATE_FAILED,
        BSD_SOCKET_ERROR_SOCKOPT_IPV6_ONLY_FAILED,
        BSD_SOCKET_ERROR_BIND_IPV6_FAILED,
        BSD_SOCKET_ERROR_BIND_IPV4_FAILED,
        BSD_SOCKET_ERROR_SET_NON_BLOCKING_FAILED
    };

    enum BSDSocketCounter
    {
        BSD_SOCKET_COUNTER_PACKETS_SENT,
        BSD_SOCKET_COUNTER_PACKETS_RECEIVED,
        BSD_SOCKET_COUNTER_SEND_FAILURES,
        BSD_SOCKET_COUNTER_SERIALIZE_WRITE_OVERFLOW,
        BSD_SOCKET_COUNTER_SERIALIZE_READ_OVERFLOW,
        BSD_SOCKET_COUNTER_PACKET_TOO_LARGE_TO_SEND,
        BSD_SOCKET_COUNTER_CREATE_PACKET_FAILURES,
        BSD_SOCKET_COUNTER_PROTOCOL_ID_MISMATCH,
        BSD_SOCKET_COUNTER_ABORTED_PACKET_READS,
        BSD_SOCKET_COUNTER_NUM_COUNTERS
    };
}

#endif
