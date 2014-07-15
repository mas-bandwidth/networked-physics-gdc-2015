/*
    Network Protocol Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_NETWORK_H
#define PROTOCOL_NETWORK_H

#include "Common.h"

namespace protocol 
{     
    bool InitializeNetwork();

    void ShutdownNetwork();

    bool IsNetworkInitialized();
}

#endif
