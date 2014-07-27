/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_CLIENT_SERVER_CONTEXT_H
#define PROTOCOL_CLIENT_SERVER_CONTEXT_H

#include "Common.h"

namespace protocol
{
    struct ClientServerContext
    {
        bool connectionEstablished;                 // true if a connection is established. if false then all "ConnectionPackets" must not serialize.
        uint64_t clientGuid;                        // the client guid. if a connection packet comes in with a different client guid it must not serialize.
        uint64_t serverGuid;                        // the server guid. if a connection packet comes in with a different server guid it must not serialize.
    };
}

#endif
