// Client Server Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef CLIENT_SERVER_ENUMS_H
#define CLIENT_SERVER_ENUMS_H

#include "core/Core.h"
#include "protocol/ProtocolEnums.h"

namespace clientServer
{
    enum ClientState
    {
        CLIENT_STATE_DISCONNECTED,                            // client is not connected. this is the initial client state.
        CLIENT_STATE_RESOLVING_HOSTNAME,                      // client is resolving hostname to address using the supplied resolver.
        CLIENT_STATE_SENDING_CONNECTION_REQUEST,              // client is sending connection request packets to the server address.
        CLIENT_STATE_SENDING_CHALLENGE_RESPONSE,              // client has received a connection challenge from server and is sending response packets.
        CLIENT_STATE_SENDING_CLIENT_DATA,                     // client is sending their own data block up to the server.
        CLIENT_STATE_READY_FOR_CONNECTION,                    // client is ready for the server to start sending connection packets.
        CLIENT_STATE_CONNECTED,                               // client is fully connected to the server. connection packets may now be processed.
        CLIENT_STATE_COUNT
    };

    inline const char * GetClientStateName( int clientState )
    {
        switch ( clientState )
        {
            case CLIENT_STATE_DISCONNECTED:                     return "DISCONNECTED";
            case CLIENT_STATE_RESOLVING_HOSTNAME:               return "RESOLVING_HOSTNAME";
            case CLIENT_STATE_SENDING_CONNECTION_REQUEST:       return "SENDING_CONNECTION_REQUEST";
            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:       return "SENDING_CHALLENGE_RESPONSE";
            case CLIENT_STATE_SENDING_CLIENT_DATA:              return "SENDING_CLIENT_DATA";
            case CLIENT_STATE_READY_FOR_CONNECTION:             return "READY_FOR_CONNECTION";
            case CLIENT_STATE_CONNECTED:                        return "CONNECTED";
            default: 
                CORE_ASSERT( 0 );
                return "???";
        }
    }

    enum ClientError
    {
        CLIENT_ERROR_NONE,                                    // client is not in an error state.
        CLIENT_ERROR_RESOLVE_HOSTNAME_FAILED,                 // client failed to resolve hostname, eg. DNS said nope.
        CLIENT_ERROR_CONNECTION_REQUEST_DENIED,               // connection request was denied.
        CLIENT_ERROR_DISCONNECTED_FROM_SERVER,                // client was fully connected to the server, then received a disconnect packet.
        CLIENT_ERROR_CONNECTION_TIMED_OUT,                    // client connection timed out (eg. server stopped responding with packets)
        CLIENT_ERROR_CONNECTION_ERROR,                        // client connection is in error state. see CONNECTION_ERROR_*
        CLIENT_ERROR_INVALID_CONNECT_ADDRESS,                 // connect address string is not a valid address.
        CLIENT_ERROR_MISSING_RESOLVER,                        // client connect needs a resolver to resolve address but resolver was null.
        CLIENT_ERROR_DATA_BLOCK_ERROR,                        // data block receiver is in error state. error x is set to data block error #
        CLIENT_ERROR_COUNT
    };

    inline const char * GetClientErrorString( int clientError )
    {
        switch ( clientError )
        {
            case CLIENT_ERROR_NONE:                             return "no error";
            case CLIENT_ERROR_RESOLVE_HOSTNAME_FAILED:          return "resolve hostname failed";
            case CLIENT_ERROR_CONNECTION_REQUEST_DENIED:        return "connection request denied";
            case CLIENT_ERROR_DISCONNECTED_FROM_SERVER:         return "disconnected from server";
            case CLIENT_ERROR_CONNECTION_TIMED_OUT:             return "connection timed out";
            case CLIENT_ERROR_CONNECTION_ERROR:                 return "connection error";
            case CLIENT_ERROR_INVALID_CONNECT_ADDRESS:          return "invalid connect address";
            case CLIENT_ERROR_MISSING_RESOLVER:                 return "missing resolver";
            case CLIENT_ERROR_DATA_BLOCK_ERROR:                 return "data block error";
            default:
                CORE_ASSERT( false );
                return "???";
        }
    }

    enum ConnectionRequestDenyReason
    {
        CONNECTION_REQUEST_DENIED_SERVER_CLOSED,                // server is closed. all connection requests are denied.
        CONNECTION_REQUEST_DENIED_SERVER_FULL,                  // server is full. no free slots for a connecting client.
        CONNECTION_REQUEST_DENIED_ALREADY_CONNECTED             // client is already connected to the server by address but with different guids.
    };

    enum ServerClientState
    {
        SERVER_CLIENT_STATE_DISCONNECTED,                       // client is disconnected. default state.
        SERVER_CLIENT_STATE_SENDING_CHALLENGE,                  // responding with connection challenge waiting for challenge response
        SERVER_CLIENT_STATE_SENDING_SERVER_DATA,                // sending server data to client
        SERVER_CLIENT_STATE_READY_FOR_CONNECTION,               // server side is ready for connection. once client is also ready the connection is established.
        SERVER_CLIENT_STATE_CONNECTED                           // client is fully connected. connection packets are now exchanged.
    };

    inline const char * GetServerClientStateName( int clientState )
    {
        switch ( clientState )
        {
            case SERVER_CLIENT_STATE_DISCONNECTED:              return "DISCONNECTED";
            case SERVER_CLIENT_STATE_SENDING_CHALLENGE:         return "SENDING_CHALLENGE";
            case SERVER_CLIENT_STATE_SENDING_SERVER_DATA:       return "SENDING_SERVER_DATA";
            case SERVER_CLIENT_STATE_READY_FOR_CONNECTION:      return "READY_FOR_CONNECTION";
            case SERVER_CLIENT_STATE_CONNECTED:                 return "CONNECTION";
            default: 
                CORE_ASSERT( 0 );
                return "???";
        }
    }

    enum Contexts
    {
        CONTEXT_CONNECTION = protocol::CONTEXT_CONNECTION,
        CONTEXT_CLIENT_SERVER,
        CONTEXT_USER
    };
}

#endif
