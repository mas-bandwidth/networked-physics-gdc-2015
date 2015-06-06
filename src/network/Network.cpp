// Network Library - Copyright (c) 2008-2015, Glenn Fiedler

#include "network/Network.h"
#include "core/Core.h"

namespace network 
{     
    static bool s_networkInitialized = false;

    bool InitializeNetwork()     
    {         
        CORE_ASSERT( !s_networkInitialized );

        bool result = true;

        #if CORE_PLATFORM == CORE_PLATFORM_WINDOWS
        WSADATA WsaData;         
        result = WSAStartup( MAKEWORD(2,2), &WsaData ) == NO_ERROR;
        #endif

        if ( result )
            s_networkInitialized = result;   

        return result;
    }

    void ShutdownNetwork()
    {
        CORE_ASSERT( s_networkInitialized );

        #if CORE_PLATFORM == CORE_PLATFORM_WINDOWS
        WSACleanup();
        #endif

        s_networkInitialized = false;
    }

    bool IsNetworkInitialized()
    {
        return s_networkInitialized;
    }
}
