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
        CLIENT_ERROR_ConnectionRequestDenied,                 // connection request was denied.
        CLIENT_ERROR_DisconnectedFromServer,                  // client was fully connected to the server, then received a disconnect packet.
        CLIENT_ERROR_ConnectionTimedOut,                      // client connection timed out (eg. server stopped responding with packets)
        CLIENT_ERROR_NumStates
    };

    const char * GetClientErrorString( int clientError )
    {
        switch ( clientError )
        {
            case CLIENT_ERROR_None:                             return "no error";
            case CLIENT_ERROR_ResolveHostnameFailed:            return "resolve hostname failed";
            case CLIENT_ERROR_ConnectionRequestDenied:          return "connection request denied";
            case CLIENT_ERROR_DisconnectedFromServer:           return "disconnected from server";
            case CLIENT_ERROR_ConnectionTimedOut:               return "connection timed out";
            default:
                assert( false );
                return "???";
        }
    }

    struct ClientConfig
    {
        uint64_t protocolId = 42;                               // the protocol id. must be same on client or server or they cannot talk to each other.

        uint16_t defaultServerPort = 10000;                     // the default server port. used when resolving by hostname and address port is zero.

        float connectingTimeOut = 5.0f;                         // number of seconds before timeout for any situation *before* the client establishes connection
        float connectedTimeOut = 10.0f;                         // number of seconds in the connected state before timing out if no connection packet received

        float connectingSendRate = 10.0f;                       // client send rate while connecting
        float connectedSendRate = 30.0f;                        // client send rate *after* being connected, eg. connection packets

        Block * block = nullptr;                                // data block sent to server on connect. optional.
        Resolver * resolver = nullptr;                          // optional resolver used to to lookup server address by hostname.
        NetworkInterface * networkInterface = nullptr;          // network interface used to send and receive packets. required.
        ChannelStructure * channelStructure = nullptr;          // channel structure for connections. required.
    };

    class Client
    {
        const ClientConfig m_config;

        TimeBase m_timeBase;

        Connection * m_connection;
        ClientServerPacketFactory * m_packetFactory;

        string m_hostname;
        Address m_address;
        ClientState m_state = CLIENT_STATE_Disconnected;
        uint64_t m_clientGuid = 0;
        uint64_t m_serverGuid = 0;
        double m_accumulator = 0.0;
        double m_lastPacketReceiveTime = 0.0;
        ClientError m_error = CLIENT_ERROR_None;
        uint32_t m_extendedError = 0;

        // todo: need to disable copy constructor etc.

    public:

        Client( const ClientConfig & config )
            : m_config( config )
        {
            assert( m_config.networkInterface );
            assert( m_config.channelStructure );

            m_packetFactory = new ClientServerPacketFactory( m_config.channelStructure );

            ConnectionConfig connectionConfig;
            connectionConfig.packetType = PACKET_Connection;
            connectionConfig.maxPacketSize = m_config.networkInterface->GetMaxPacketSize();
            connectionConfig.channelStructure = m_config.channelStructure;
            connectionConfig.packetFactory = m_packetFactory;

            m_connection = new Connection( connectionConfig );
        }

        ~Client()
        {
            assert( m_connection );
            assert( m_packetFactory );

            delete m_connection;
            delete m_packetFactory;
            
            m_connection = nullptr;
            m_packetFactory = nullptr;
        }

        void Connect( const Address & address )
        {
            Disconnect();

            ClearError();

//            printf( "client connect by address: %s\n", address.ToString().c_str() );

            m_state = CLIENT_STATE_SendingConnectionRequest;
            m_address = address;
            m_clientGuid = GenerateGuid();
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

//            printf( "resolving hostname: \"%s\"\n", hostname.c_str() );

            assert( m_config.resolver );

            m_config.resolver->Resolve( hostname );
            
            m_state = CLIENT_STATE_ResolvingHostname;
            m_hostname = hostname;
            m_lastPacketReceiveTime = m_timeBase.time;
        }

        void Disconnect()
        {
            if ( IsDisconnected() )
                return;

//            printf( "client disconnect\n" );
            
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

        Resolver * GetResolver() const
        {
            return m_config.resolver;
        }

        NetworkInterface * GetNetworkInterface() const
        {
            return m_config.networkInterface;
        }

        Connection * GetConnection() const
        {
            return m_connection;
        }

        void Update( const TimeBase & timeBase )
        {
            m_timeBase = timeBase;

            UpdateResolver();
         
            UpdateConnection();

            UpdateSendPackets();

            UpdateNetworkInterface();
            
            UpdateReceivePackets();

            UpdateTimeout();
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

            if ( m_state != CLIENT_STATE_ResolvingHostname )
                return;

            auto entry = m_config.resolver->GetEntry( m_hostname );

//            printf( "update resolve hostname\n" );

            if ( !entry || entry->status == ResolveStatus::Failed )
            {
//                printf( "resolve hostname failed\n" );
                DisconnectAndSetError( CLIENT_ERROR_ResolveHostnameFailed );
                return;
            }

            if ( entry->status == ResolveStatus::Succeeded )
            {
//                printf( "resolve hostname succeeded: %s\n", entry->result->addresses[0].ToString().c_str() );

                auto address = entry->result->addresses[0];

                if ( address.GetPort() == 0 )
                    address.SetPort( m_config.defaultServerPort );

                Connect( address );
            }
        }

        void UpdateConnection()
        {
            if ( m_state == CLIENT_STATE_Connected )
                m_connection->Update( m_timeBase );
        }

        void UpdateSendPackets()
        {
            if ( m_state < CLIENT_STATE_SendingConnectionRequest )
                return;

            m_accumulator += m_timeBase.deltaTime;

            const float timeBetweenPackets = 1.0 / ( IsConnected() ? m_config.connectedSendRate : m_config.connectingSendRate );

            if ( m_accumulator >= timeBetweenPackets )
            {
                m_accumulator -= timeBetweenPackets;

                switch ( m_state )
                {
                    case CLIENT_STATE_SendingConnectionRequest:
                    {
//                        printf( "client sent connection request packet\n" );
                        auto packet = new ConnectionRequestPacket();
                        packet->protocolId = m_config.protocolId;
                        packet->clientGuid = m_clientGuid;
                        m_config.networkInterface->SendPacket( m_address, packet );
                    }
                    break;

                    case CLIENT_STATE_SendingChallengeResponse:
                    {
//                        printf( "client sent challenge response packet\n" );
                        auto packet = new ChallengeResponsePacket();
                        packet->protocolId = m_config.protocolId;
                        packet->clientGuid = m_clientGuid;
                        packet->serverGuid = m_serverGuid;
                        m_config.networkInterface->SendPacket( m_address, packet );
                    }
                    break;

                    case CLIENT_STATE_ReadyForConnection:
                    {
                        auto packet = new ReadyForConnectionPacket();
                        packet->protocolId = m_config.protocolId;
                        packet->clientGuid = m_clientGuid;
                        packet->serverGuid = m_serverGuid;
                        m_config.networkInterface->SendPacket( m_address, packet );
                    }
                    break;

                    case CLIENT_STATE_Connected:
                    {
//                        printf( "client sent connection packet\n" );
                        auto packet = m_connection->WritePacket();
                        m_config.networkInterface->SendPacket( m_address, packet );
                    }
                    break;

                    default:
                        break;
                }
            }
        }

        void UpdateReceivePackets()
        {
            while ( true )
            {
                auto packet = m_config.networkInterface->ReceivePacket();
                if ( !packet )
                    break;

                if ( packet->GetType() == PACKET_Disconnected )
                {
//                    printf( "client received disconnected packet\n" );
                    ProcessDisconnected( static_cast<DisconnectedPacket*>( packet ) );
                    continue;
                }

                switch ( m_state )
                {
                    case CLIENT_STATE_SendingConnectionRequest:
                    {
                        if ( packet->GetType() == PACKET_ConnectionChallenge )
                        {
                            auto connectionChallengePacket = static_cast<ConnectionChallengePacket*>( packet );

                            if ( connectionChallengePacket->GetAddress() == m_address &&
                                 connectionChallengePacket->clientGuid == m_clientGuid )
                            {
//                                printf( "received connection challenge packet from server\n" );

                                m_state = CLIENT_STATE_SendingChallengeResponse;
                                m_serverGuid = connectionChallengePacket->serverGuid;
                                m_lastPacketReceiveTime = m_timeBase.time;
                            }
                        }
                        else if ( packet->GetType() == PACKET_ConnectionDenied )
                        {
                            auto connectionDeniedPacket = static_cast<ConnectionDeniedPacket*>( packet );

                            if ( connectionDeniedPacket->GetAddress() == m_address &&
                                 connectionDeniedPacket->clientGuid == m_clientGuid )
                            {
//                                printf( "received connection denied packet from server\n" );

                                DisconnectAndSetError( CLIENT_ERROR_ConnectionRequestDenied, connectionDeniedPacket->reason );
                            }
                        }
                    }
                    break;

                    case CLIENT_STATE_SendingChallengeResponse:
                    {
                        if ( packet->GetType() == PACKET_RequestClientData )
                        {
                            auto requestClientDataPacket = static_cast<RequestClientDataPacket*>( packet );

                            if ( requestClientDataPacket->GetAddress() == m_address &&
                                 requestClientDataPacket->clientGuid == m_clientGuid &&
                                 requestClientDataPacket->serverGuid == m_serverGuid )
                            {
//                                printf( "received request client data packet from server\n" );

                                if ( !m_config.block )
                                {
//                                    printf( "client is ready for connection\n" );

                                    m_state = CLIENT_STATE_ReadyForConnection;
                                    m_lastPacketReceiveTime = m_timeBase.time;
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
//                            printf( "client received connection packet\n" );

                            if ( m_state == CLIENT_STATE_ReadyForConnection )
                            {
//                                printf( "client transitioned to connected state\n" );

                                m_state = CLIENT_STATE_Connected;
                            }

                            const bool result = m_connection->ReadPacket( static_cast<ConnectionPacket*>( packet ) );
                            if ( result )
                                m_lastPacketReceiveTime = m_timeBase.time;
                        }
                    }
                    break;

                    default:
                        break;
                }

                delete packet;
            }
        }

        void ProcessDisconnected( DisconnectedPacket * packet )
        {
            assert( packet );

            if ( packet->GetAddress() != m_address )
                return;

            if ( packet->clientGuid != m_clientGuid )
                return;

            if ( packet->serverGuid != m_serverGuid )
                return;

            DisconnectAndSetError( CLIENT_ERROR_DisconnectedFromServer );
        }

        void UpdateTimeout()
        {
            if ( IsDisconnected() )
                return;

            const double timeout = IsConnected() ? m_config.connectedTimeOut : m_config.connectingTimeOut;

            if ( m_lastPacketReceiveTime + timeout < m_timeBase.time )
            {
//                printf( "client timed out\n" );
                DisconnectAndSetError( CLIENT_ERROR_ConnectionTimedOut, m_state );
            }
        }

        void DisconnectAndSetError( ClientError error, uint32_t extendedError = 0 )
        {
//            printf( "client error: %s\n", GetClientErrorString( error ) );

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
            m_hostname = "";
            m_address = Address();
            m_clientGuid = 0;
            m_serverGuid = 0;
        }
    };
}

#endif
