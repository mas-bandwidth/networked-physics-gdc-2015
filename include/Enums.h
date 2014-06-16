#ifndef PROTOCOL_ENUMS_H
#define PROTOCOL_ENUMS_H

#include <assert.h>

namespace protocol
{
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
        BSD_SOCKET_COUNTER_NUM_COUNTERS
    };

    enum ClientState
    {
        CLIENT_STATE_DISCONNECTED,                            // client is not connected. this is the initial client state.
        CLIENT_STATE_RESOLVING_HOSTNAME,                      // client is resolving hostname to address using the supplied resolver.
        CLIENT_STATE_SENDING_CONNECTION_REQUEST,              // client is sending connection request packets to the server address.
        CLIENT_STATE_SENDING_CHALLENGE_RESPONSE,              // client has received a connection challenge from server and is sending response packets.
        CLIENT_STATE_RECEIVING_SERVER_DATA,                   // client is receiving a data block from the server.
        CLIENT_STATE_SENDING_CLIENT_DATA,                     // client is sending their own data block up to the server.
        CLIENT_STATE_READY_FOR_CONNECTION,                    // client is ready for the server to start sending connection packets.
        CLIENT_STATE_CONNECTED,                               // client is fully connected to the server. connection packets may now be processed.
        CLIENT_STATE_COUNT
    };

    const char * GetClientStateName( int clientState )
    {
        switch ( clientState )
        {
            case CLIENT_STATE_DISCONNECTED:                     return "disconnected";
            case CLIENT_STATE_RESOLVING_HOSTNAME:               return "resolving hostname";
            case CLIENT_STATE_SENDING_CONNECTION_REQUEST:       return "sending connection request";
            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:       return "sending challenge response";
            case CLIENT_STATE_RECEIVING_SERVER_DATA:            return "receiving server data";
            case CLIENT_STATE_SENDING_CLIENT_DATA:              return "sending client data";
            case CLIENT_STATE_READY_FOR_CONNECTION:             return "ready for connection";
            case CLIENT_STATE_CONNECTED:                        return "connected";
            default: 
                assert( 0 );
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
        CLIENT_ERROR_COUNT
    };

    const char * GetClientErrorString( int clientError )
    {
        switch ( clientError )
        {
            case CLIENT_ERROR_NONE:                             return "no error";
            case CLIENT_ERROR_RESOLVE_HOSTNAME_FAILED:          return "resolve hostname failed";
            case CLIENT_ERROR_CONNECTION_REQUEST_DENIED:        return "connection request denied";
            case CLIENT_ERROR_DISCONNECTED_FROM_SERVER:         return "disconnected from server";
            case CLIENT_ERROR_CONNECTION_TIMED_OUT:             return "connection timed out";
            case CLIENT_ERROR_CONNECTION_ERROR:                 return "connection error";
            default:
                assert( false );
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
        SERVER_CLIENT_STATE_REQUESTING_CLIENT_DATA,             // requesting client data
        SERVER_CLIENT_STATE_RECEIVING_CLIENT_DATA,              // receiving client data
        SERVER_CLIENT_STATE_CONNECTED                           // client is fully connected. connection packets are now exchanged.
    };

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
}

#endif
