/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#include "Client.h"
#include "Memory.h"

namespace protocol
{
    Client::Client( const ClientConfig & config )
        : m_config( config )
    {
        PROTOCOL_ASSERT( m_config.networkInterface );
        PROTOCOL_ASSERT( m_config.channelStructure );

        m_hostname[0] = '\0';

        m_allocator = m_config.allocator ? m_config.allocator : &memory::default_allocator();

        m_packetFactory = &m_config.networkInterface->GetPacketFactory();

        ConnectionConfig connectionConfig;
        connectionConfig.packetType = CLIENT_SERVER_PACKET_CONNECTION;
        connectionConfig.maxPacketSize = m_config.networkInterface->GetMaxPacketSize();
        connectionConfig.channelStructure = m_config.channelStructure;
        connectionConfig.packetFactory = m_packetFactory;

        m_connection = PROTOCOL_NEW( *m_allocator, Connection, connectionConfig );

        // todo
        /*
        if ( m_config.maxServerDataSize > 0 )
            m_serverData = (uint8_t*) m_allocator->Allocate( m_config.maxServerDataSize );

        if ( m_config.clientData )
        {
            const int bytes = m_config.clientData->GetSize();
            m_numClientDataFragments = bytes / m_config.fragmentSize + ( ( bytes % m_config.fragmentSize ) ? 1 : 0 );
            m_ackedFragment = (uint8_t*) m_allocator->Allocate( m_numClientDataFragments );
            memset( m_ackedFragment, 0, m_numClientDataFragments );
        }
        */

        ClearStateData();
    }

    Client::~Client()
    {
        PROTOCOL_ASSERT( m_allocator );

        Disconnect();

        // todo
        /*
        m_serverDataBlock.Disconnect();
        */

        PROTOCOL_ASSERT( m_connection );
        PROTOCOL_ASSERT( m_packetFactory );
        PROTOCOL_ASSERT( m_config.fragmentSize >= 0 );
        PROTOCOL_ASSERT( m_config.fragmentSize <= MaxFragmentSize );

        PROTOCOL_DELETE( *m_allocator, Connection, m_connection );

        m_connection = nullptr;
        m_packetFactory = nullptr;          // IMPORTANT: packet factory pointer is not owned by us
    }

    void Client::Connect( const Address & address )
    {
        Disconnect();

        ClearError();

//            printf( "client connect by address: %s\n", address.ToString().c_str() );

        m_state = CLIENT_STATE_SENDING_CONNECTION_REQUEST;
        m_address = address;
        m_clientGuid = generate_guid();

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

#if PROTOCOL_USE_RESOLVER

        // if we don't have a resolver, we can't resolve the string to an address...

        if ( !m_config.resolver )
            DisconnectAndSetError( CLIENT_ERROR_MISSING_RESOLVER );

        // ok, it's probably a hostname. go into the resolving hostname state

        m_config.resolver->Resolve( hostname );
        
        m_state = CLIENT_STATE_RESOLVING_HOSTNAME;
        m_lastPacketReceiveTime = m_timeBase.time;
        strncpy( m_hostname, hostname, MaxHostName - 1 );
        m_hostname[MaxHostName-1] = '\0';

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

        packet->clientGuid = m_clientGuid;
        packet->serverGuid = m_serverGuid;

        m_config.networkInterface->SendPacket( m_address, packet );

        m_connection->Reset();

        ClearStateData();
        
        m_state = CLIENT_STATE_DISCONNECTED;
        m_address = Address();
        m_hostname[0] = '\0';

        // todo: disconnect clear sender and receiver
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

#if PROTOCOL_USE_RESOLVER

    Resolver * Client::GetResolver() const
    {
        return m_config.resolver;
    }

#endif

    NetworkInterface * Client::GetNetworkInterface() const
    {
        return m_config.networkInterface;
    }

    Connection * Client::GetConnection() const
    {
        return m_connection;
    }

    const Block * Client::GetServerData() const
    {
        // todo
        /*
        PROTOCOL_ASSERT( m_serverData );
        if ( m_serverDataSize > 0 && m_state == CLIENT_STATE_CONNECTED )
        {
            m_serverDataBlock.Connect( *m_allocator, m_serverData, m_serverDataSize );
            return &m_serverDataBlock;
        }
        else
        */
            return nullptr;
    }

    void Client::Update( const TimeBase & timeBase )
    {
        m_timeBase = timeBase;

#if PROTOCOL_USE_RESOLVER
        UpdateResolver();
#endif
     
        UpdateConnection();

        UpdateSendPackets();

        UpdateNetworkInterface();
        
        UpdateReceivePackets();

        UpdateSendClientData();

        UpdateTimeout();
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
                    auto packet = (ConnectionRequestPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_CONNECTION_REQUEST );
                    packet->clientGuid = m_clientGuid;
//                        printf( "send connection request packet: m_clientGuid = %llx\n", m_clientGuid );
                    m_config.networkInterface->SendPacket( m_address, packet );
                }
                break;

                case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
                {
//                        printf( "client sent challenge response packet\n" );
                    auto packet = (ChallengeResponsePacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE );
                    packet->clientGuid = m_clientGuid;
                    packet->serverGuid = m_serverGuid;
                    m_config.networkInterface->SendPacket( m_address, packet );
                }
                break;

                case CLIENT_STATE_READY_FOR_CONNECTION:
                {
                    auto packet = (ReadyForConnectionPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_READY_FOR_CONNECTION );
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
                    if ( type == CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT )
                    {
                        ProcessDataBlockFragment( (DataBlockFragmentPacket*) packet );
                    }
                    else if ( type == CLIENT_SERVER_PACKET_READY_FOR_CONNECTION )
                    {
                        auto readyForConnectionPacket = static_cast<ReadyForConnectionPacket*>( packet );

                        if ( readyForConnectionPacket->GetAddress() == m_address &&
                             readyForConnectionPacket->clientGuid == m_clientGuid &&
                             readyForConnectionPacket->serverGuid == m_serverGuid )
                        {
                            if ( !m_config.clientData )
                            {
                                m_state = CLIENT_STATE_READY_FOR_CONNECTION;
                                m_lastPacketReceiveTime = m_timeBase.time;
                            }
                            else
                            {
                                m_state = CLIENT_STATE_SENDING_CLIENT_DATA;
                                m_lastPacketReceiveTime = m_timeBase.time;
                            }
                        }
                    }
                }
                break;

                case CLIENT_STATE_SENDING_CLIENT_DATA:
//                case CLIENT_STATE_READY_FOR_CONNECTION:
                {
                    if ( type == CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT )
                    {
                        ProcessDataBlockFragment( (DataBlockFragmentPacket*) packet );
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

            m_packetFactory->Destroy( packet );
        }
    }

    void Client::UpdateSendClientData()
    {
        if ( m_state != CLIENT_STATE_SENDING_CLIENT_DATA )
            return;

        PROTOCOL_ASSERT( m_config.clientData );
        PROTOCOL_ASSERT( m_config.clientData->GetSize() );

        // todo
        /*
        const double timeBetweenFragments = 1.0 / m_config.fragmentsPerSecond;

        if ( m_lastFragmentSendTime + timeBetweenFragments >= m_timeBase.time )
            return;

        m_lastFragmentSendTime = m_timeBase.time;

        PROTOCOL_ASSERT( m_numAckedFragments < m_numClientDataFragments );

        for ( int i = 0; i < m_numClientDataFragments; ++i )
        {
            if ( !m_ackedFragment[m_fragmentIndex] )
                break;
            m_fragmentIndex++;
            m_fragmentIndex %= m_numClientDataFragments;
        }

        printf( "client send fragment %d\n", m_fragmentIndex );

        PROTOCOL_ASSERT( m_fragmentIndex >= 0 );
        PROTOCOL_ASSERT( m_fragmentIndex < m_numClientDataFragments );
        PROTOCOL_ASSERT( !m_ackedFragment[m_fragmentIndex] );

        auto packet = (DataBlockFragmentPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT );

        int fragmentBytes = m_config.fragmentSize;
        if ( m_fragmentIndex == m_numClientDataFragments - 1 )
            fragmentBytes = ( m_numClientDataFragments * m_config.fragmentSize ) - m_config.clientData->GetSize();

        PROTOCOL_ASSERT( fragmentBytes > 0 );
        PROTOCOL_ASSERT( fragmentBytes <= MaxFragmentSize );

        packet->clientGuid = m_clientGuid;
        packet->serverGuid = m_serverGuid;
        packet->totalSize = m_config.clientData->GetSize();
        packet->fragmentSize = m_config.fragmentSize;
        packet->fragmentId = m_fragmentIndex;
        packet->fragmentBytes = fragmentBytes;
        packet->fragmentData = (uint8_t*) memory::scratch_allocator().Allocate( packet->fragmentSize );

        const uint8_t * clientData = m_config.clientData->GetData();

        memcpy( packet->fragmentData, clientData + m_fragmentIndex * m_config.fragmentSize, fragmentBytes );

        m_config.networkInterface->SendPacket( m_address, packet );

        m_fragmentIndex = ( m_fragmentIndex + 1 ) % m_numClientDataFragments;
        */
    }

    void Client::ProcessDisconnected( DisconnectedPacket * packet )
    {
        PROTOCOL_ASSERT( packet );

        if ( packet->GetAddress() != m_address )
            return;

        if ( packet->clientGuid != m_clientGuid )
            return;

        if ( packet->serverGuid != m_serverGuid )
            return;

        DisconnectAndSetError( CLIENT_ERROR_DISCONNECTED_FROM_SERVER );
    }

    void Client::ProcessDataBlockFragment( DataBlockFragmentPacket * packet )
    {
        PROTOCOL_ASSERT( packet );

        if ( packet->clientGuid != m_clientGuid )
            return;

        if ( packet->serverGuid != m_serverGuid )
            return;

        // todo
        /*
        if ( packet->totalSize > m_config.maxServerDataSize )
        {
            DisconnectAndSetError( CLIENT_ERROR_SERVER_DATA_TOO_LARGE );
            return;
        }

//        printf( "received fragment: %d\n", packet->fragmentId );

        m_serverDataSize = packet->totalSize;

        auto replyPacket = (DataBlockFragmentAckPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK );
        if ( !replyPacket )
            return;

        replyPacket->clientGuid = m_clientGuid;
        replyPacket->serverGuid = m_serverGuid;
        replyPacket->fragmentId = packet->fragmentId;

        m_config.networkInterface->SendPacket( m_address, replyPacket );

        const int start = packet->fragmentId * packet->fragmentSize;
        const int finish = start + packet->fragmentBytes;

        PROTOCOL_ASSERT( finish <= m_config.maxServerDataSize );
        if ( finish > m_config.maxServerDataSize )
            return;

        memcpy( m_serverData + start, packet->fragmentData, packet->fragmentBytes );

        m_lastPacketReceiveTime = m_timeBase.time;
        */
    }

    void Client::UpdateTimeout()
    {
        if ( IsDisconnected() )
            return;

        // todo: change this so the timeout before connection is applied since connect start time

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
