/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#include "Network.h"

namespace protocol 
{     
    static bool s_networkInitialized = false;

    bool InitializeNetwork()     
    {         
        assert( !s_networkInitialized );

        bool result = true;

        #if PROTOCOL_PLATFORM == PROTOCOL_PLATFORM_WINDOWS
        WSADATA WsaData;         
        result = WSAStartup( MAKEWORD(2,2), &WsaData ) == NO_ERROR;
        #endif

        if ( result )
            s_networkInitialized = result;   

        return result;
    }

    void ShutdownNetwork()
    {
        assert( s_networkInitialized );

        #if PROTOCOL_PLATFORM == PROTOCOL_PLATFORM_WINDOWS
        WSACleanup();
        #endif

        s_networkInitialized = false;
    }

    bool IsNetworkInitialized()
    {
        return s_networkInitialized;
    }
}
