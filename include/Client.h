/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_CLIENT_H
#define PROTOCOL_CLIENT_H

#include "Packets.h"
#include "Resolver.h"
#include "Connection.h"
#include "NetworkInterface.h"

namespace protocol
{
    struct ClientConfig
    {
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

        std::string m_hostname;
        Address m_address;
        ClientState m_state = CLIENT_STATE_DISCONNECTED;
        uint64_t m_clientGuid = 0;
        uint64_t m_serverGuid = 0;
        double m_accumulator = 0.0;
        double m_lastPacketReceiveTime = 0.0;
        ClientError m_error = CLIENT_ERROR_NONE;
        uint32_t m_extendedError = 0;

        Client( const Client & other );
        Client & operator = ( const Client & other );

    public:

        Client( const ClientConfig & config )
            : m_config( config )
        {
            assert( m_config.networkInterface );
            assert( m_config.channelStructure );

            m_packetFactory = new ClientServerPacketFactory( m_config.channelStructure );

            ConnectionConfig connectionConfig;
            connectionConfig.packetType = PACKET_CONNECTION;
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

            m_state = CLIENT_STATE_SENDING_CONNECTION_REQUEST;
            m_address = address;
            m_clientGuid = GenerateGuid();

//            printf( "connect: set client guid = %llx\n", m_clientGuid );
        }

        void Connect( const std::string & hostname )
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
            
            m_state = CLIENT_STATE_RESOLVING_HOSTNAME;
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
            
            m_state = CLIENT_STATE_DISCONNECTED;
        }

        bool IsDisconnected() const
        {
            return m_state == CLIENT_STATE_DISCONNECTED;
        }

        bool IsConnected() const
        {
            return m_state == CLIENT_STATE_CONNECTED;
        }

        bool IsConnecting() const
        {
            return m_state > CLIENT_STATE_DISCONNECTED && m_state < CLIENT_STATE_CONNECTED;
        }

        ClientState GetState() const
        {
            return m_state;
        }

        bool HasError() const
        {
            return m_error != CLIENT_ERROR_NONE;
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

            if ( m_state != CLIENT_STATE_RESOLVING_HOSTNAME )
                return;

            auto entry = m_config.resolver->GetEntry( m_hostname );

//            printf( "update resolve hostname\n" );

            if ( !entry || entry->status == RESOLVE_FAILED )
            {
//                printf( "resolve hostname failed\n" );
                DisconnectAndSetError( CLIENT_ERROR_RESOLVE_HOSTNAME_FAILED );
                return;
            }

            if ( entry->status == RESOLVE_SUCCEEDED )
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
            if ( m_state == CLIENT_STATE_CONNECTED )
            {
                m_connection->Update( m_timeBase );

                if ( m_connection->GetError() != CONNECTION_ERROR_NONE )
                {
                    DisconnectAndSetError( CLIENT_ERROR_CONNECTION_ERROR );
                    return;
                }
            }
        }

        void UpdateSendPackets()
        {
            if ( m_state < CLIENT_STATE_SENDING_CONNECTION_REQUEST )
                return;

            m_accumulator += m_timeBase.deltaTime;

            const float timeBetweenPackets = 1.0 / ( IsConnected() ? m_config.connectedSendRate : m_config.connectingSendRate );

            if ( m_accumulator >= timeBetweenPackets )
            {
                m_accumulator -= timeBetweenPackets;

                switch ( m_state )
                {
                    case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
                    {
//                        printf( "client sent connection request packet\n" );
                        auto packet = new ConnectionRequestPacket();
                        packet->clientGuid = m_clientGuid;
//                        printf( "send connection request packet: m_clientGuid = %llx\n", m_clientGuid );
                        m_config.networkInterface->SendPacket( m_address, packet );
                    }
                    break;

                    case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
                    {
//                        printf( "client sent challenge response packet\n" );
                        auto packet = new ChallengeResponsePacket();
                        packet->clientGuid = m_clientGuid;
                        packet->serverGuid = m_serverGuid;
                        m_config.networkInterface->SendPacket( m_address, packet );
                    }
                    break;

                    case CLIENT_STATE_READY_FOR_CONNECTION:
                    {
                        auto packet = new ReadyForConnectionPacket();
                        packet->clientGuid = m_clientGuid;
                        packet->serverGuid = m_serverGuid;
                        m_config.networkInterface->SendPacket( m_address, packet );
                    }
                    break;

                    case CLIENT_STATE_CONNECTED:
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

                const int type = packet->GetType();

                if ( type == PACKET_DISCONNECTED )
                {
//                    printf( "client received disconnected packet\n" );
                    ProcessDisconnected( static_cast<DisconnectedPacket*>( packet ) );
                    continue;
                }

                switch ( m_state )
                {
                    case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
                    {
                        if ( type == PACKET_CONNECTION_CHALLENGE )
                        {
                            auto connectionChallengePacket = static_cast<ConnectionChallengePacket*>( packet );

                            if ( connectionChallengePacket->GetAddress() == m_address &&
                                 connectionChallengePacket->clientGuid == m_clientGuid )
                            {
//                                printf( "received connection challenge packet from server\n" );

                                m_state = CLIENT_STATE_SENDING_CHALLENGE_RESPONSE;
                                m_serverGuid = connectionChallengePacket->serverGuid;
                                m_lastPacketReceiveTime = m_timeBase.time;
                            }
                        }
                        else if ( type == PACKET_CONNECTION_DENIED )
                        {
                            auto connectionDeniedPacket = static_cast<ConnectionDeniedPacket*>( packet );

                            if ( connectionDeniedPacket->GetAddress() == m_address &&
                                 connectionDeniedPacket->clientGuid == m_clientGuid )
                            {
//                                printf( "received connection denied packet from server\n" );

                                DisconnectAndSetError( CLIENT_ERROR_CONNECTION_REQUEST_DENIED, connectionDeniedPacket->reason );
                            }
                        }
                    }
                    break;

                    case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
                    {
                        if ( type == PACKET_REQUEST_CLIENT_DATA )
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

                                    m_state = CLIENT_STATE_READY_FOR_CONNECTION;
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

                    case CLIENT_STATE_READY_FOR_CONNECTION:
                    case CLIENT_STATE_CONNECTED:
                    {
                        if ( type == PACKET_CONNECTION )
                        {
//                            printf( "client received connection packet\n" );

                            if ( m_state == CLIENT_STATE_READY_FOR_CONNECTION )
                            {
//                                printf( "client transitioned to connected state\n" );

                                m_state = CLIENT_STATE_CONNECTED;
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

            DisconnectAndSetError( CLIENT_ERROR_DISCONNECTED_FROM_SERVER );
        }

        void UpdateTimeout()
        {
            if ( IsDisconnected() )
                return;

            const double timeout = IsConnected() ? m_config.connectedTimeOut : m_config.connectingTimeOut;

            if ( m_lastPacketReceiveTime + timeout < m_timeBase.time )
            {
//                printf( "client timed out\n" );
                DisconnectAndSetError( CLIENT_ERROR_CONNECTION_TIMED_OUT, m_state );
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
            m_error = CLIENT_ERROR_NONE;
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
