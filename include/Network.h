/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
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
