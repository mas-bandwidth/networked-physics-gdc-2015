// Client Server Library - Copyright (c) 2014, The Network Protocol Company, Inc.

#include "clientServer/Client.h"
#include "protocol/Connection.h"
#include "network/Simulator.h"
#include "network/Interface.h"
#include "core/Memory.h"

namespace clientServer
{
    Client::Client( const ClientConfig & config )
        : m_config( config )
    {
        CORE_ASSERT( m_config.networkInterface );
        CORE_ASSERT( m_config.channelStructure );

        m_hostname[0] = '\0';

        m_allocator = m_config.allocator ? m_config.allocator : &core::memory::default_allocator();

        m_packetFactory = &m_config.networkInterface->GetPacketFactory();

        if ( m_config.maxServerDataSize > 0 )
            m_dataBlockReceiver = CORE_NEW( *m_allocator, DataBlockReceiver, *m_allocator, m_config.fragmentSize, m_config.maxServerDataSize );

        if ( m_config.clientData )
            m_dataBlockSender = CORE_NEW( *m_allocator, DataBlockSender, *m_allocator, *m_config.clientData, m_config.fragmentSize, m_config.fragmentsPerSecond );

        memset( m_context, 0, sizeof(m_context) );

        m_clientServerContext.Initialize( *m_allocator, 1 );

        m_context[CONTEXT_CONNECTION] = m_config.channelStructure;
        m_context[CONTEXT_CLIENT_SERVER] = &m_clientServerContext;

        m_config.networkInterface->SetContext( m_context );

        protocol::ConnectionConfig connectionConfig;
        connectionConfig.packetType = CLIENT_SERVER_PACKET_CONNECTION;
        connectionConfig.maxPacketSize = m_config.networkInterface->GetMaxPacketSize();
        connectionConfig.channelStructure = m_config.channelStructure;
        connectionConfig.packetFactory = m_packetFactory;
        connectionConfig.context = m_context;

        m_connection = CORE_NEW( *m_allocator, protocol::Connection, connectionConfig );

        ClearStateData();
    }

    Client::~Client()
    {
        CORE_ASSERT( m_allocator );

        Disconnect();

        if ( m_dataBlockSender )
        {
            CORE_DELETE( *m_allocator, DataBlockSender, m_dataBlockSender );
            m_dataBlockSender = nullptr;
        }

        if ( m_dataBlockReceiver )
        {
            CORE_DELETE( *m_allocator, DataBlockReceiver, m_dataBlockReceiver );
            m_dataBlockReceiver = nullptr;
        }

        CORE_ASSERT( m_connection );
        CORE_ASSERT( m_packetFactory );
        CORE_ASSERT( m_config.fragmentSize >= 0 );
        CORE_ASSERT( m_config.fragmentSize <= protocol::MaxFragmentSize );

        CORE_DELETE( *m_allocator, Connection, m_connection );

        m_clientServerContext.Free( *m_allocator );

        m_connection = nullptr;
        m_packetFactory = nullptr;          // IMPORTANT: packet factory pointer is not owned by us
    }

    void Client::Connect( const network::Address & address )
    {
        Disconnect();

        ClearError();

        OnConnect( address );

//            printf( "client connect by address: %s\n", address.ToString().c_str() );

        SetClientState( CLIENT_STATE_SENDING_CONNECTION_REQUEST );
        m_address = address;
        m_clientId = core::generate_id();

//            printf( "connect: set client id = %x\n", m_clientId );

        m_lastPacketReceiveTime = m_timeBase.time;          // IMPORTANT: otherwise times out immediately after connect once time value gets large
    }

    void Client::Connect( const char * hostname )
    {
        Disconnect();

        ClearError();

        // is this hostname actually an address? If so connect by address instead.

        network::Address address( hostname );
        if ( address.IsValid() )
        {
            Connect( address );
            return;
        }

#if PROTOCOL_USE_RESOLVER

        // if we don't have a resolver, we can't resolve the string to an address...

        if ( !m_config.resolver )
            DisconnectAndSetError( CLIENT_ERROR_MISSING_RESOLVER );

        // ok, it's probably a hostname. go into the resolving hostname state

        m_config.resolver->Resolve( hostname );
        
        SetClientState( CLIENT_STATE_RESOLVING_HOSTNAME );
        m_lastPacketReceiveTime = m_timeBase.time;
        strncpy( m_hostname, hostname, MaxHostName - 1 );
        m_hostname[MaxHostName-1] = '\0';

        OnConnect( hostname );

#else

        // we are configured to never use a resolver. caller must pass in a valid address

        DisconnectAndSetError( CLIENT_ERROR_INVALID_CONNECT_ADDRESS );

#endif
    }

    void Client::Disconnect()
    {
        if ( IsDisconnected() )
            return;

//            printf( "client disconnect\n" );

        auto packet = (DisconnectedPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_DISCONNECTED );

        packet->clientId = m_clientId;
        packet->serverId = m_serverId;

        SendPacket( packet );

        m_connection->Reset();

        ClearStateData();
        
        SetClientState( CLIENT_STATE_DISCONNECTED );
        m_address = network::Address();
        m_hostname[0] = '\0';

        if ( m_dataBlockSender )
            m_dataBlockSender->Clear();

        if ( m_dataBlockReceiver )
            m_dataBlockReceiver->Clear();

        OnDisconnect();
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

#if NETWORK_USE_RESOLVER

    Resolver * Client::GetResolver() const
    {
        return m_config.resolver;
    }

#endif

    network::Interface * Client::GetNetworkInterface() const
    {
        return m_config.networkInterface;
    }

    protocol::Connection * Client::GetConnection() const
    {
        return m_connection;
    }

    const protocol::Block * Client::GetServerData() const
    {
        return m_dataBlockReceiver ? m_dataBlockReceiver->GetBlock() : nullptr;
    }

    void Client::SetContext( int index, const void * ptr )
    {
        CORE_ASSERT( index >= CONTEXT_USER );
        CORE_ASSERT( index < protocol::MaxContexts );
        m_context[index] = ptr;
    }

    void Client::Update( const core::TimeBase & timeBase )
    {
        m_timeBase = timeBase;

#if PROTOCOL_USE_RESOLVER
        UpdateResolver();
#endif
     
        UpdateConnection();

        UpdateSendPackets();

        UpdateNetworkSimulator();

        UpdateNetworkInterface();
        
        UpdateReceivePackets();

        UpdateSendClientData();

        UpdateTimeout();
    }

    void Client::UpdateNetworkSimulator()
    {
        if ( !m_config.networkSimulator )
            return;

        m_config.networkSimulator->Update( m_timeBase );

        while ( true )
        {
            auto packet = m_config.networkSimulator->ReceivePacket();
            if ( !packet )
                break;

            m_config.networkInterface->SendPacket( packet->GetAddress(), packet );
        }
    }

    void Client::UpdateNetworkInterface()
    {
        m_config.networkInterface->Update( m_timeBase );
    }

#if PROTOCOL_USE_RESOLVER

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

#endif

    void Client::UpdateConnection()
    {
        if ( m_state == CLIENT_STATE_CONNECTED )
        {
            m_connection->Update( m_timeBase );

            if ( m_connection->GetError() != protocol::CONNECTION_ERROR_NONE )
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
                    auto packet = (ConnectionRequestPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_CONNECTION_REQUEST );
                    packet->clientId = m_clientId;
                    m_config.networkInterface->SendPacket( m_address, packet );
                }
                break;

                case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
                {
                    auto packet = (ChallengeResponsePacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE );
                    packet->clientId = m_clientId;
                    packet->serverId = m_serverId;
                    m_config.networkInterface->SendPacket( m_address, packet );
                }
                break;

                case CLIENT_STATE_READY_FOR_CONNECTION:
                {
                    auto packet = (ReadyForConnectionPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_READY_FOR_CONNECTION );
                    packet->clientId = m_clientId;
                    packet->serverId = m_serverId;
                    m_config.networkInterface->SendPacket( m_address, packet );
                }
                break;

                case CLIENT_STATE_CONNECTED:
                {
                    auto packet = m_connection->WritePacket();
                    packet->clientId = m_clientId;
                    packet->serverId = m_serverId;
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
                m_packetFactory->Destroy( packet );
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
                             connectionChallengePacket->clientId == m_clientId )
                        {
//                                printf( "received connection challenge packet from server\n" );

                            SetClientState( CLIENT_STATE_SENDING_CHALLENGE_RESPONSE );

                            m_lastPacketReceiveTime = m_timeBase.time;

                            m_serverId = connectionChallengePacket->serverId;

                            ClientServerInfo info;
                            info.address = m_address;
                            info.clientId = m_clientId;
                            info.serverId = m_serverId;
                            info.packetFactory = m_packetFactory;
                            info.networkInterface = m_config.networkInterface;

                            if ( m_dataBlockSender )
                                m_dataBlockSender->SetInfo( info );

                            if ( m_dataBlockReceiver )
                                m_dataBlockReceiver->SetInfo( info );

                            m_clientServerContext.AddClient( 0, m_address, m_clientId, m_serverId );
                        }
                    }
                    else if ( type == CLIENT_SERVER_PACKET_CONNECTION_DENIED )
                    {
                        auto connectionDeniedPacket = static_cast<ConnectionDeniedPacket*>( packet );

                        if ( connectionDeniedPacket->GetAddress() == m_address &&
                             connectionDeniedPacket->clientId == m_clientId )
                        {
//                                printf( "received connection denied packet from server\n" );

                            DisconnectAndSetError( CLIENT_ERROR_CONNECTION_REQUEST_DENIED, connectionDeniedPacket->reason );
                        }
                    }
                }
                break;

                case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
                {
                    if ( type == CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT )
                    {
                        ProcessDataBlockFragment( (DataBlockFragmentPacket*) packet );
                    }
                    else if ( type == CLIENT_SERVER_PACKET_READY_FOR_CONNECTION )
                    {
                        auto readyForConnectionPacket = static_cast<ReadyForConnectionPacket*>( packet );

                        if ( readyForConnectionPacket->GetAddress() == m_address &&
                             readyForConnectionPacket->clientId == m_clientId &&
                             readyForConnectionPacket->serverId == m_serverId )
                        {
                            if ( !m_config.clientData )
                            {
                                SetClientState( CLIENT_STATE_READY_FOR_CONNECTION );
                                m_lastPacketReceiveTime = m_timeBase.time;
                            }
                            else
                            {
                                SetClientState( CLIENT_STATE_SENDING_CLIENT_DATA );
                                m_lastPacketReceiveTime = m_timeBase.time;
                            }
                        }
                    }
                }
                break;

                case CLIENT_STATE_SENDING_CLIENT_DATA:
                {
                    if ( type == CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT )
                    {
                        ProcessDataBlockFragment( (DataBlockFragmentPacket*) packet );
                    }
                    else if ( type == CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK )
                    {
                        ProcessDataBlockFragmentAck( (DataBlockFragmentAckPacket*) packet );
                    }
                }
                break;

                case CLIENT_STATE_READY_FOR_CONNECTION:
                case CLIENT_STATE_CONNECTED:
                {
                    if ( type == CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT )
                    {
                        ProcessDataBlockFragment( (DataBlockFragmentPacket*) packet );
                    }
                    else if ( type == CLIENT_SERVER_PACKET_CONNECTION )
                    {
                        auto connectionPacket = (protocol::ConnectionPacket*) packet;
                        if ( connectionPacket->clientId == m_clientId && connectionPacket->serverId == m_serverId )
                        {
                            if ( m_state == CLIENT_STATE_READY_FOR_CONNECTION )
                                SetClientState( CLIENT_STATE_CONNECTED );

                            const bool result = m_connection->ReadPacket( connectionPacket );
                            if ( result )
                                m_lastPacketReceiveTime = m_timeBase.time;
                        }
                    }
                }
                break;

                default:
                    break;
            }

            m_packetFactory->Destroy( packet );
        }
    }

    void Client::UpdateSendClientData()
    {
        if ( m_state != CLIENT_STATE_SENDING_CLIENT_DATA )
            return;

        CORE_ASSERT( m_dataBlockSender );

//        printf( "update send client data\n" );

        if ( m_dataBlockSender->SendCompleted() )
        {
//            printf( "ready for connection\n" );
            SetClientState( CLIENT_STATE_READY_FOR_CONNECTION );
            return;
        }
        
        m_dataBlockSender->Update( m_timeBase );
    }

    void Client::ProcessDisconnected( DisconnectedPacket * packet )
    {
        CORE_ASSERT( packet );

        if ( packet->GetAddress() != m_address )
            return;

        if ( packet->clientId != m_clientId )
            return;

        if ( packet->serverId != m_serverId )
            return;

        DisconnectAndSetError( CLIENT_ERROR_DISCONNECTED_FROM_SERVER );
    }

    void Client::ProcessDataBlockFragment( DataBlockFragmentPacket * packet )
    {
        CORE_ASSERT( packet );

        if ( packet->clientId != m_clientId )
            return;

        if ( packet->serverId != m_serverId )
            return;

        if ( !m_dataBlockReceiver )
            return;

        bool receiveAlreadyCompleted = m_dataBlockReceiver->ReceiveCompleted();

        m_dataBlockReceiver->ProcessFragment( packet->blockSize,
                                              packet->numFragments,
                                              packet->fragmentId, 
                                              packet->fragmentBytes,
                                              packet->fragmentData );

        if ( m_dataBlockReceiver->IsError() )
            DisconnectAndSetError( CLIENT_ERROR_DATA_BLOCK_ERROR, m_dataBlockReceiver->GetError() );

        if ( m_dataBlockReceiver->ReceiveCompleted() && !receiveAlreadyCompleted )
            OnServerDataReceived( *m_dataBlockReceiver->GetBlock() );
    }

    void Client::ProcessDataBlockFragmentAck( DataBlockFragmentAckPacket * packet )
    {
        CORE_ASSERT( packet );

//        printf( "process fragment ack %d\n", packet->fragmentId );

        if ( packet->clientId != m_clientId )
            return;

        if ( packet->serverId != m_serverId )
            return;

        if ( !m_dataBlockSender )
            return;

        m_dataBlockSender->ProcessAck( packet->fragmentId );
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
        CORE_ASSERT( error != CLIENT_ERROR_NONE );

        OnError( error, extendedError );

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
        m_address = network::Address();
        m_clientId = 0;
        m_serverId = 0;
        m_clientServerContext.RemoveClient( 0 );
    }

    void Client::SendPacket( protocol::Packet * packet )
    {
        auto interface = m_config.networkSimulator ? m_config.networkSimulator : m_config.networkInterface;
        interface->SendPacket( m_address, packet );
    }

    void Client::SetClientState( ClientState state )
    {
        ClientState previous = m_state;

        if ( state != previous )
        {
            m_state = state;
            OnStateChange( previous, state );
        }
    }
}
