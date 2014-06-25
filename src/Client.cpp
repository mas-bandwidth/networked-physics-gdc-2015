/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#include "Client.h"

namespace protocol
{
    Client::Client( const ClientConfig & config )
        : m_config( config )
    {
        assert( m_config.networkInterface );
        assert( m_config.channelStructure );

        m_packetFactory = new ClientServerPacketFactory( m_config.channelStructure );

        ConnectionConfig connectionConfig;
        connectionConfig.packetType = CLIENT_SERVER_PACKET_CONNECTION;
        connectionConfig.maxPacketSize = m_config.networkInterface->GetMaxPacketSize();
        connectionConfig.channelStructure = m_config.channelStructure;
        connectionConfig.packetFactory = m_packetFactory;

        m_connection = new Connection( connectionConfig );

        ClearStateData();
    }

    Client::~Client()
    {
        Disconnect();

        assert( m_connection );
        assert( m_packetFactory );

        delete m_connection;
        delete m_packetFactory;
        
        m_connection = nullptr;
        m_packetFactory = nullptr;
    }

    void Client::Connect( const Address & address )
    {
        Disconnect();

        ClearError();

//            printf( "client connect by address: %s\n", address.ToString().c_str() );

        m_state = CLIENT_STATE_SENDING_CONNECTION_REQUEST;
        m_address = address;
        m_clientGuid = GenerateGuid();

//            printf( "connect: set client guid = %llx\n", m_clientGuid );
    }

    void Client::Connect( const char * hostname )
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

//            printf( "resolving hostname: \"%s\"\n", hostname );

        assert( m_config.resolver );

        m_config.resolver->Resolve( hostname );
        
        m_state = CLIENT_STATE_RESOLVING_HOSTNAME;
        m_lastPacketReceiveTime = m_timeBase.time;
        strncpy( m_hostname, hostname, MaxHostName - 1 );
        m_hostname[MaxHostName-1] = '\0';
    }

    void Client::Disconnect()
    {
        if ( IsDisconnected() )
            return;

//            printf( "client disconnect\n" );

        // todo: send off disconnect packet to server. in the common case this packet
        // gets through and tells the server this client has disconnected, freeing up
        // the client slot much faster than timing it out over 5-10 seconds.

        m_connection->Reset();

        ClearStateData();
        
        m_state = CLIENT_STATE_DISCONNECTED;
    }

    bool Client::IsDisconnected() const
    {
        return m_state == CLIENT_STATE_DISCONNECTED;
    }

    bool Client::IsConnected() const
    {
        return m_state == CLIENT_STATE_CONNECTED;
    }

    bool Client::IsConnecting() const
    {
        return m_state > CLIENT_STATE_DISCONNECTED && m_state < CLIENT_STATE_CONNECTED;
    }

    ClientState Client::GetState() const
    {
        return m_state;
    }

    bool Client::HasError() const
    {
        return m_error != CLIENT_ERROR_NONE;
    }

    ClientError Client::GetError() const
    {
        return m_error;
    }

    uint32_t Client::GetExtendedError() const
    {
        return m_extendedError;
    }

    Resolver * Client::GetResolver() const
    {
        return m_config.resolver;
    }

    NetworkInterface * Client::GetNetworkInterface() const
    {
        return m_config.networkInterface;
    }

    Connection * Client::GetConnection() const
    {
        return m_connection;
    }

    void Client::Update( const TimeBase & timeBase )
    {
        m_timeBase = timeBase;

        UpdateResolver();
     
        UpdateConnection();

        UpdateSendPackets();

        UpdateNetworkInterface();
        
        UpdateReceivePackets();

        UpdateTimeout();
    }

    void Client::UpdateNetworkInterface()
    {
        m_config.networkInterface->Update( m_timeBase );
    }

    void Client::UpdateResolver()
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

            auto address = entry->result.address[0];

            if ( address.GetPort() == 0 )
                address.SetPort( m_config.defaultServerPort );

            Connect( address );
        }
    }

    void Client::UpdateConnection()
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

    void Client::UpdateSendPackets()
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

    void Client::UpdateReceivePackets()
    {
        while ( true )
        {
            auto packet = m_config.networkInterface->ReceivePacket();
            if ( !packet )
                break;

            const int type = packet->GetType();

            if ( type == CLIENT_SERVER_PACKET_DISCONNECTED )
            {
//                    printf( "client received disconnected packet\n" );
                ProcessDisconnected( static_cast<DisconnectedPacket*>( packet ) );
                continue;
            }

            switch ( m_state )
            {
                case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
                {
                    if ( type == CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE )
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
                    else if ( type == CLIENT_SERVER_PACKET_CONNECTION_DENIED )
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
                    if ( type == CLIENT_SERVER_PACKET_REQUEST_CLIENT_DATA )
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
                    if ( type == CLIENT_SERVER_PACKET_CONNECTION )
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

    void Client::ProcessDisconnected( DisconnectedPacket * packet )
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

    void Client::UpdateTimeout()
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

    void Client::DisconnectAndSetError( ClientError error, uint32_t extendedError )
    {
//            printf( "client error: %s\n", GetClientErrorString( error ) );

        Disconnect();
        
        m_error = error;
        m_extendedError = extendedError;
    }

    void Client::ClearError()
    {
        m_error = CLIENT_ERROR_NONE;
        m_extendedError = 0;
    }

    void Client::ClearStateData()
    {
        m_hostname[0] = '\0';
        m_address = Address();
        m_clientGuid = 0;
        m_serverGuid = 0;
    }
}
