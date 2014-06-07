/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_CLIENT_H
#define PROTOCOL_CLIENT_H

#include "Resolver.h"
#include "Connection.h"
#include "NetworkInterface.h"
#include "ClientServerPackets.h"

namespace protocol
{
    enum ClientState
    {
        CLIENT_STATE_Disconnected,                            // client is not connected. this is the initial client state.
        CLIENT_STATE_ResolvingHostname,                       // client is resolving hostname to address using the supplied resolver.
        CLIENT_STATE_SendingConnectionRequest,                // client is sending connection request packets to the server address.
        CLIENT_STATE_SendingChallengeResponse,                // client has received a connection challenge from server and is sending response packets.
        CLIENT_STATE_ReceivingServerData,                     // client is receiving a data block from the server.
        CLIENT_STATE_SendingClientData,                       // client is sending their own data block up to the server.
        CLIENT_STATE_ReadyForConnection,                      // client is ready for the server to start sending connection packets.
        CLIENT_STATE_Connected,                               // client is fully connected to the server. connection packets may now be processed.
        CLIENT_STATE_NumStates
    };

    const char * GetClientStateName( int clientState )
    {
        switch ( clientState )
        {
            case CLIENT_STATE_Disconnected:                     return "disconnected";
            case CLIENT_STATE_ResolvingHostname:                return "resolving hostname";
            case CLIENT_STATE_SendingConnectionRequest:         return "sending connection request";
            case CLIENT_STATE_SendingChallengeResponse:         return "sending challenge response";
            case CLIENT_STATE_ReceivingServerData:              return "receiving server data";
            case CLIENT_STATE_SendingClientData:                return "sending client data";
            case CLIENT_STATE_ReadyForConnection:               return "ready for connection";
            case CLIENT_STATE_Connected:                        return "connected";
            default: 
                assert( 0 );
                return "???";
        }
    }

    enum ClientError
    {
        CLIENT_ERROR_None,                                    // client is not in an error state.
        CLIENT_ERROR_ResolveHostnameFailed,                   // client failed to resolve hostname, eg. DNS said nope.
        CLIENT_ERROR_ResolveHostnameTimedOut,                 // client timed out while trying to resolve hostname.
        CLIENT_ERROR_ConnectionRequestDenied,                 // client received a connection denied response to their connection request.
        CLIENT_ERROR_ConnectionRequestTimedOut,               // client timed out while sending connection requests.
        CLIENT_ERROR_ChallengeResponseTimedOut,               // client timed out while sending connection challenge responses.
        CLIENT_ERROR_ReceiveServerDataTimedOut,               // client timed out while receiving server data
        CLIENT_ERROR_SendClientDataTimedOut,                  // client timed out while sending client data
        CLIENT_ERROR_ReadyForConnectionTimedOut,              // client timed out while in the ready for connection state
        CLIENT_ERROR_DisconnectedFromServer,                  // client was fully connected to the server, then received a disconnect packet.
        CLIENT_ERROR_ConnectionTimedOut,                      // client was fully connected to the server, then the connection timed out.
        CLIENT_ERROR_NumStates
    };

    const char * GetClientErrorString( int clientError )
    {
        switch ( clientError )
        {
            case CLIENT_ERROR_None:                             return "no error";
            case CLIENT_ERROR_ResolveHostnameFailed:            return "resolve hostname failed";
            case CLIENT_ERROR_ResolveHostnameTimedOut:          return "resolve hostname timed out";
            case CLIENT_ERROR_ConnectionRequestDenied:          return "connection request denied";
            case CLIENT_ERROR_ConnectionRequestTimedOut:        return "connection request timed out";
            case CLIENT_ERROR_ChallengeResponseTimedOut:        return "challenge response timed out";
            case CLIENT_ERROR_ReceiveServerDataTimedOut:        return "receive server data timed out";
            case CLIENT_ERROR_SendClientDataTimedOut:           return "send client data timed out";
            case CLIENT_ERROR_ReadyForConnectionTimedOut:       return "ready for connection timed out";
            case CLIENT_ERROR_DisconnectedFromServer:           return "disconnected from server";
            default:
                assert( false );
                return "???";
        }
    }

    struct ClientConfig
    {
        uint64_t protocolId = 42;                               // the protocol id. must be same on client or server or they cannot talk to each other.

        float connectingTimeOut = 5.0f;                         // number of seconds before timeout for any situation *before* the client establishes connection
        float connectedTimeOut = 10.0f;                         // number of seconds in the connected state before timing out if no connection packet received

        float connectingSendRate = 10.0f;                       // client send rate while connecting
        float connectedSendRate = 30.0f;                        // client send rate *after* being connected, eg. connection packets

        shared_ptr<Resolver> resolver;                          // optional resolver used to to lookup server address by hostname.
        shared_ptr<NetworkInterface> networkInterface;          // network interface used to send and receive packets.
        shared_ptr<ChannelStructure> channelStructure;          // channel structure for connections

        shared_ptr<Block> block;                                // data block sent to server on connect. optional.
    };

    class Client
    {
        const ClientConfig m_config;

        TimeBase m_timeBase;

        ClientState m_state = CLIENT_STATE_Disconnected;
        ClientError m_error = CLIENT_ERROR_None;
        uint32_t m_extendedError = 0;                           // extended error information, eg. if connection is denied will contain the reason why

        shared_ptr<Connection> m_connection;

        struct ResolveHostnameData
        {
            string hostname;
            double startTime = 0.0;
        };

        struct SendingConnectionRequestData
        {
            Address address;
            double startTime = 0.0;
            uint64_t clientGuid = 0;
            double accumulator = 0.0;
        };

        struct SendingChallengeResponseData
        {
            Address address;
            double startTime = 0.0;
            uint64_t clientGuid = 0;
            uint64_t serverGuid = 0;
            double accumulator = 0.0;
        };

        struct ReadyForConnectionData
        {
            Address address;
            double startTime = 0.0;
            uint64_t clientGuid = 0;
            uint64_t serverGuid = 0;
            double accumulator = 0.0;
        };

        struct ConnectedData
        {
            Address address;
            double startTime = 0.0;
            uint64_t clientGuid = 0;
            uint64_t serverGuid = 0;
            double accumulator = 0.0;
        };

        ResolveHostnameData m_resolveHostnameData;
        SendingConnectionRequestData m_sendingConnectionRequestData;
        SendingChallengeResponseData m_sendingChallengeResponseData;
        ReadyForConnectionData m_readyForConnectionData;
        ConnectedData m_connectedData;

    public:

        Client( const ClientConfig & config )
            : m_config( config )
        {
            assert( m_config.networkInterface );
            assert( m_config.channelStructure );

            ConnectionConfig connectionConfig;
            connectionConfig.packetType = PACKET_Connection;
            connectionConfig.maxPacketSize = m_config.networkInterface->GetMaxPacketSize();
            connectionConfig.channelStructure = m_config.channelStructure;
            connectionConfig.packetFactory = make_shared<ClientServerPacketFactory>( m_config.channelStructure );

            m_connection = make_shared<Connection>( connectionConfig );
        }

        void Connect( const Address & address )
        {
            Disconnect();

            ClearError();

//            cout << "client connect by address: " << address.ToString() << endl;

            m_state = CLIENT_STATE_SendingConnectionRequest;

            m_sendingConnectionRequestData.address = address;
            m_sendingConnectionRequestData.startTime = m_timeBase.time;
            m_sendingConnectionRequestData.clientGuid = GenerateGuid();
        }

        void Connect( const string & hostname )
        {
            Disconnect();

            ClearError();

            // is this hostname actually an address? If so connect by address instead.

            Address address( hostname );
            if ( address.IsValid() )
            {
                Connect( address );
                return;
            }

            // ok, it's really a hostname. go into the resolving hostname state

//            cout << "resolving hostname: \"" << hostname << "\"" << endl;

            assert( m_config.resolver );

            m_config.resolver->Resolve( hostname );

            m_resolveHostnameData.hostname = hostname;
            m_resolveHostnameData.startTime = m_timeBase.time;
            
            m_state = CLIENT_STATE_ResolvingHostname;
        }

        void Disconnect()
        {
            if ( IsDisconnected() )
                return;

//            cout << "client disconnect" << endl;
            
            m_connection->Reset();

            ClearStateData();
            
            m_state = CLIENT_STATE_Disconnected;
        }

        bool IsDisconnected() const
        {
            return m_state == CLIENT_STATE_Disconnected;
        }

        bool IsConnected() const
        {
            return m_state == CLIENT_STATE_Connected;
        }

        bool IsConnecting() const
        {
            return m_state > CLIENT_STATE_Disconnected && m_state < CLIENT_STATE_Connected;
        }

        ClientState GetState() const
        {
            return m_state;
        }

        bool HasError() const
        {
            return m_error != CLIENT_ERROR_None;
        }

        ClientError GetError() const
        {
            return m_error;
        }

        uint32_t GetExtendedError() const
        {
            return m_extendedError;
        }

        shared_ptr<Resolver> GetResolver() const
        {
            return m_config.resolver;
        }

        shared_ptr<NetworkInterface> GetNetworkInterface() const
        {
            return m_config.networkInterface;
        }

        shared_ptr<Connection> GetConnection() const
        {
            return m_connection;
        }

        void Update( const TimeBase & timeBase )
        {
            m_timeBase = timeBase;

            UpdateResolver();
            
            UpdateCurrentState();
            
            UpdateNetworkInterface();
            
            UpdateReceivePackets();
        }

    protected:

        void UpdateNetworkInterface()
        {
            m_config.networkInterface->Update( m_timeBase );
        }

        void UpdateResolver()
        {
            if ( m_config.resolver )
                m_config.resolver->Update( m_timeBase );
        }

        void UpdateReceivePackets()
        {
            while ( true )
            {
                auto packet = m_config.networkInterface->ReceivePacket();
                if ( !packet )
                    break;

                switch ( m_state )
                {
                    // todo: clean this up. hard to read.

                    case CLIENT_STATE_SendingConnectionRequest:
                    {
                        if ( packet->GetType() == PACKET_ConnectionChallenge )
                        {
                            auto connectionChallengePacket = static_pointer_cast<ConnectionChallengePacket>( packet );

                            if ( connectionChallengePacket->GetAddress() == m_sendingConnectionRequestData.address &&
                                 connectionChallengePacket->clientGuid == m_sendingConnectionRequestData.clientGuid )
                            {
//                                cout << "received connection challenge packet from server" << endl;

                                m_state = CLIENT_STATE_SendingChallengeResponse;

                                m_sendingChallengeResponseData.startTime = m_timeBase.time;
                                m_sendingChallengeResponseData.address = m_sendingConnectionRequestData.address;
                                m_sendingChallengeResponseData.clientGuid = connectionChallengePacket->clientGuid;
                                m_sendingChallengeResponseData.serverGuid = connectionChallengePacket->serverGuid;
                            }
                        }
                        else if ( packet->GetType() == PACKET_ConnectionDenied )
                        {
                            auto connectionDeniedPacket = static_pointer_cast<ConnectionDeniedPacket>( packet );

                            if ( connectionDeniedPacket->GetAddress() == m_sendingConnectionRequestData.address &&
                                 connectionDeniedPacket->clientGuid == m_sendingConnectionRequestData.clientGuid )
                            {
//                                cout << "received connection denied packet from server" << endl;

                                DisconnectAndSetError( CLIENT_ERROR_ConnectionRequestDenied, connectionDeniedPacket->reason );
                            }
                        }
                    }
                    break;

                    case CLIENT_STATE_SendingChallengeResponse:
                    {
                        if ( packet->GetType() == PACKET_RequestClientData )
                        {
                            auto requestClientDataPacket = static_pointer_cast<RequestClientDataPacket>( packet );

                            if ( requestClientDataPacket->GetAddress() == m_sendingChallengeResponseData.address &&
                                 requestClientDataPacket->clientGuid == m_sendingChallengeResponseData.clientGuid &&
                                 requestClientDataPacket->serverGuid == m_sendingChallengeResponseData.serverGuid )
                            {
//                                cout << "received request client data packet from server" << endl;

                                if ( !m_config.block )
                                {
//                                    cout << "client is ready for connection" << endl;

                                    m_state = CLIENT_STATE_ReadyForConnection;

                                    m_readyForConnectionData.startTime = m_timeBase.time;
                                    m_readyForConnectionData.address = m_sendingChallengeResponseData.address;
                                    m_readyForConnectionData.clientGuid = requestClientDataPacket->clientGuid;
                                    m_readyForConnectionData.serverGuid = requestClientDataPacket->serverGuid;
                                }
                                else
                                {
                                    // todo: implement client block send to server
                                    assert( false );
                                }
                            }
                        }
                    }
                    break;

                    case CLIENT_STATE_ReadyForConnection:
                    case CLIENT_STATE_Connected:
                    {
                        if ( packet->GetType() == PACKET_Connection )
                        {
//                            cout << "client received connection packet" << endl;

                            if ( m_state == CLIENT_STATE_ReadyForConnection )
                            {
//                                cout << "client transitioned to connected state" << endl;

                                m_state = CLIENT_STATE_Connected;
                                m_connectedData.startTime = m_timeBase.time;
                                m_connectedData.accumulator = 0.0;
                                m_connectedData.address = m_readyForConnectionData.address;
                                m_connectedData.clientGuid = m_readyForConnectionData.clientGuid;
                                m_connectedData.serverGuid = m_readyForConnectionData.serverGuid;
                            }

                            m_connection->ReadPacket( static_pointer_cast<ConnectionPacket>( packet ) );

                            // todo: update last packet received time
                        }
                    }
                    break;

                    default:
                        break;
                }
            }
        }

        void UpdateCurrentState()
        {
            switch ( m_state )
            {
                case CLIENT_STATE_ResolvingHostname:
                    UpdateResolveHostname();
                    break;

                case CLIENT_STATE_SendingConnectionRequest:
                    UpdateSendingConnectionRequest();
                    break;

                case CLIENT_STATE_SendingChallengeResponse:
                    UpdateSendingChallengeResponse();
                    break;

                case CLIENT_STATE_ReadyForConnection:
                    UpdateReadyForConnection();
                    break;

                case CLIENT_STATE_Connected:
                    UpdateConnected();
                    break;

                default:
                    break;
            }
        }

        void UpdateResolveHostname()
        {
            assert( m_state == CLIENT_STATE_ResolvingHostname );

            if ( m_timeBase.time - m_resolveHostnameData.startTime > m_config.connectingTimeOut )
            {
                DisconnectAndSetError( CLIENT_ERROR_ResolveHostnameTimedOut );
                return;
            }

            auto entry = m_config.resolver->GetEntry( m_resolveHostnameData.hostname );

            if ( !entry || entry->status == ResolveStatus::Failed )
            {
                DisconnectAndSetError( CLIENT_ERROR_ResolveHostnameFailed );
                return;
            }

            if ( entry->status == ResolveStatus::Succeeded )
            {
//                cout << "resolve succeeded" << endl;

                assert( entry->result );

                const vector<Address> & addresses = entry->result->addresses;

                assert( addresses.size() );

                Connect( addresses[0] );
            }
        }

        void UpdateSendingConnectionRequest()
        {
            assert( m_state == CLIENT_STATE_SendingConnectionRequest );

            if ( m_timeBase.time - m_sendingConnectionRequestData.startTime > m_config.connectingTimeOut )
            {
//                cout << "connection request timed out" << endl;
                DisconnectAndSetError( CLIENT_ERROR_ConnectionRequestTimedOut );
                return;
            }

            m_sendingConnectionRequestData.accumulator += m_timeBase.deltaTime;

            const float timeBetweenPackets = 1.0f / m_config.connectingSendRate;

            if ( m_sendingConnectionRequestData.accumulator >= timeBetweenPackets )
            {
                m_sendingConnectionRequestData.accumulator -= timeBetweenPackets;

                auto packet = make_shared<ConnectionRequestPacket>();
                packet->protocolId = m_config.protocolId;
                packet->clientGuid = m_sendingConnectionRequestData.clientGuid;

//                cout << "client sent connection request packet" << endl;

                m_config.networkInterface->SendPacket( m_sendingConnectionRequestData.address, packet );
            }
        }

        void UpdateSendingChallengeResponse()
        {
            assert( m_state == CLIENT_STATE_SendingChallengeResponse );

            if ( m_timeBase.time - m_sendingChallengeResponseData.startTime > m_config.connectingTimeOut )
            {
                DisconnectAndSetError( CLIENT_ERROR_ChallengeResponseTimedOut );
                return;
            }

            m_sendingChallengeResponseData.accumulator += m_timeBase.deltaTime;

            const float timeBetweenPackets = 1.0f / m_config.connectingSendRate;

            if ( m_sendingChallengeResponseData.accumulator >= timeBetweenPackets )
            {
//                cout << "client sent challenge response" << endl;

                m_sendingChallengeResponseData.accumulator -= timeBetweenPackets;

                auto packet = make_shared<ChallengeResponsePacket>();

                packet->protocolId = m_config.protocolId;
                packet->clientGuid = m_sendingChallengeResponseData.clientGuid;
                packet->serverGuid = m_sendingChallengeResponseData.serverGuid;

                m_config.networkInterface->SendPacket( m_sendingChallengeResponseData.address, packet );
            }
        }

        void UpdateReadyForConnection()
        {
            assert( m_state == CLIENT_STATE_ReadyForConnection );

            if ( m_timeBase.time - m_readyForConnectionData.startTime > m_config.connectingTimeOut )
            {
                // todo: add unit test for this case
                DisconnectAndSetError( CLIENT_ERROR_ReadyForConnectionTimedOut );
                return;
            }

            m_readyForConnectionData.accumulator += m_timeBase.deltaTime;

            const float timeBetweenPackets = 1.0f / m_config.connectingSendRate;

            if ( m_readyForConnectionData.accumulator >= timeBetweenPackets )
            {
//                cout << "client sent ready for connection packet" << endl;

                m_readyForConnectionData.accumulator -= timeBetweenPackets;

                auto packet = make_shared<ReadyForConnectionPacket>();

                packet->protocolId = m_config.protocolId;
                packet->clientGuid = m_readyForConnectionData.clientGuid;
                packet->serverGuid = m_readyForConnectionData.serverGuid;

                m_config.networkInterface->SendPacket( m_readyForConnectionData.address, packet );
            }
        }

        void UpdateConnected()
        {
            assert( m_state == CLIENT_STATE_Connected );

            // todo: this needs to be based off last packet receive time, not start time!
            /*
            if ( m_timeBase.time - m_connectedData.startTime > m_config.connectedTimeOut )
            {
                // todo: add unit test for this case
                cout << "client timed out while connected" << endl;
                DisconnectAndSetError( CLIENT_ERROR_ConnectionTimedOut );
                return;
            }
            */

            m_connection->Update( m_timeBase );

            m_connectedData.accumulator += m_timeBase.deltaTime;

            const float timeBetweenPackets = 1.0f / m_config.connectedSendRate;

            if ( m_connectedData.accumulator >= timeBetweenPackets )
            {
//                cout << "client sent connection packet" << endl;

                m_connectedData.accumulator -= timeBetweenPackets;

                auto packet = m_connection->WritePacket();

                m_config.networkInterface->SendPacket( m_connectedData.address, packet );
            }
        }

        void DisconnectAndSetError( ClientError error, uint32_t extendedError = 0 )
        {
//            cout << "client error: " << GetClientErrorString( error ) << endl;

            Disconnect();
            
            m_error = error;
            m_extendedError = extendedError;
        }

        void ClearError()
        {
            m_error = CLIENT_ERROR_None;
            m_extendedError = 0;
        }

        void ClearStateData()
        {
            m_resolveHostnameData = ResolveHostnameData();
            m_sendingConnectionRequestData = SendingConnectionRequestData();
            m_sendingChallengeResponseData = SendingChallengeResponseData();
            m_readyForConnectionData = ReadyForConnectionData();
            m_connectedData = ConnectedData();
        }
    };
}

#endif
