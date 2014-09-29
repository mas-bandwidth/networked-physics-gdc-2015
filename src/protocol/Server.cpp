// Protocol Library - Copyright (c) 2014, The Network Protocol Company, Inc.

#include "protocol/Server.h"
#include "network/Simulator.h"
#include "core/Memory.h"

namespace protocol
{
    Server::Server( const ServerConfig & config )
        : m_config( config )
    {
        CORE_ASSERT( m_config.networkInterface );
        CORE_ASSERT( m_config.channelStructure );
        CORE_ASSERT( m_config.maxClients >= 1 );
        CORE_ASSERT( m_config.fragmentSize >= 0 );
        CORE_ASSERT( m_config.fragmentSize <= MaxFragmentSize );

        m_allocator = m_config.allocator ? m_config.allocator : &core::memory::default_allocator();

//            printf( "creating server with %d client slots\n", m_config.maxClients );

        m_packetFactory = &m_config.networkInterface->GetPacketFactory();

        m_numClients = m_config.maxClients;

        m_clientServerContext.Initialize( *m_allocator, m_numClients );

        memset( m_context, 0, sizeof( m_context ) );

        m_context[CONTEXT_CHANNEL_STRUCTURE] = m_config.channelStructure;
        m_context[CONTEXT_CLIENT_SERVER] = &m_clientServerContext;

        m_config.networkInterface->SetContext( m_context );

        ConnectionConfig connectionConfig;
        connectionConfig.packetType = CLIENT_SERVER_PACKET_CONNECTION;
        connectionConfig.maxPacketSize = m_config.networkInterface->GetMaxPacketSize();
        connectionConfig.channelStructure = m_config.channelStructure;
        connectionConfig.packetFactory = m_packetFactory;
        connectionConfig.context = m_context;

        m_clients = CORE_NEW_ARRAY( *m_allocator, ClientData, m_numClients );

        for ( int i = 0; i < m_numClients; ++i )
        {
            m_clients[i].connection = CORE_NEW( *m_allocator, Connection, connectionConfig );

            if ( m_config.serverData )
                m_clients[i].dataBlockSender = CORE_NEW( *m_allocator, ClientServerDataBlockSender, *m_allocator, *m_config.serverData, m_config.fragmentSize, m_config.fragmentsPerSecond );

            if ( m_config.maxClientDataSize > 0 )
                m_clients[i].dataBlockReceiver = CORE_NEW( *m_allocator, ClientServerDataBlockReceiver, *m_allocator, m_config.fragmentSize, m_config.maxClientDataSize );
        }
    }

    Server::~Server()
    {
        CORE_ASSERT( m_allocator );
        CORE_ASSERT( m_clients );
        CORE_ASSERT( m_packetFactory );

        m_clientServerContext.Free( *m_allocator );

        for ( int i = 0; i < m_numClients; ++i )
        {
            CORE_ASSERT( m_clients[i].connection );
            CORE_DELETE( *m_allocator, Connection, m_clients[i].connection );
            m_clients[i].connection = nullptr;

            if ( m_clients[i].dataBlockSender )
            {
                CORE_DELETE( *m_allocator, ClientServerDataBlockSender, m_clients[i].dataBlockSender );
                m_clients[i].dataBlockSender = nullptr;
            }

            if ( m_clients[i].dataBlockReceiver )
            {
                CORE_DELETE( *m_allocator, ClientServerDataBlockReceiver, m_clients[i].dataBlockReceiver );
                m_clients[i].dataBlockReceiver = nullptr;
            }
        }

        CORE_DELETE_ARRAY( *m_allocator, m_clients, m_numClients );

        m_clients = nullptr;
        m_packetFactory = nullptr;
    }

    void Server::Open()
    {
        m_open = true;
    }

    void Server::Close()
    {
        m_open = false;
    }

    bool Server::IsOpen() const
    {
        return m_open;
    }

    void Server::Update( const core::TimeBase & timeBase )
    {
        m_timeBase = timeBase;

        UpdateClients();

        UpdateNetworkSimulator();

        UpdateNetworkInterface();

        UpdateReceivePackets();
    }

    void Server::DisconnectClient( int clientIndex )
    {
        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < m_numClients );

        auto & client = m_clients[clientIndex];

        if ( client.state == SERVER_CLIENT_STATE_DISCONNECTED )
            return;

//            printf( "sent disconnected packet to client\n" );

        auto packet = (DisconnectedPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_DISCONNECTED );

        packet->clientId = client.clientId;
        packet->serverId = client.serverId;

        SendPacket( client.address, packet );

        ResetClientSlot( clientIndex );
    }

    ServerClientState Server::GetClientState( int clientIndex ) const
    {
        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < m_numClients );
        return m_clients[clientIndex].state;
    }

    Connection * Server::GetClientConnection( int clientIndex )
    {
        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < m_numClients );
        return m_clients[clientIndex].connection;
    }

    const Block * Server::GetClientData( int clientIndex ) const
    {
        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < m_numClients );

        ClientData & client = m_clients[clientIndex];

        return client.dataBlockReceiver ? client.dataBlockReceiver->GetBlock() : nullptr;
    }

    void Server::SetContext( int index, const void * ptr )
    {
        CORE_ASSERT( index >= CONTEXT_USER );
        CORE_ASSERT( index < MaxContexts );
        m_context[index] = ptr;
    }

    void Server::UpdateClients()
    {
        for ( int i = 0; i < m_numClients; ++i )
        {
            switch ( m_clients[i].state )
            {
                case SERVER_CLIENT_STATE_SENDING_CHALLENGE:
                    UpdateSendingChallenge( i );
                    break;

                case SERVER_CLIENT_STATE_SENDING_SERVER_DATA:
                    UpdateSendingServerData( i );
                    break;

                case SERVER_CLIENT_STATE_READY_FOR_CONNECTION:
                    UpdateReadyForConnection( i );
                    break;

                case SERVER_CLIENT_STATE_CONNECTED:
                    UpdateConnected( i );
                    break;

                default:
                    break;
            }

            UpdateTimeouts( i );

            if ( m_clients[i].state == SERVER_CLIENT_STATE_READY_FOR_CONNECTION && m_clients[i].readyForConnection )
            {
                SetClientState( i, SERVER_CLIENT_STATE_CONNECTED );
                m_clients[i].accumulator = 0.0f;
            }
        }
    }

    void Server::UpdateSendingChallenge( int clientIndex )
    {
        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < m_numClients );

        ClientData & client = m_clients[clientIndex];

        CORE_ASSERT( client.state == SERVER_CLIENT_STATE_SENDING_CHALLENGE );

        if ( client.accumulator > 1.0 / m_config.connectingSendRate )
        {
            auto packet = (ConnectionChallengePacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE );

            packet->clientId = client.clientId;
            packet->serverId = client.serverId;

            SendPacket( client.address, packet );

            client.accumulator = 0.0;
        }
    }

    void Server::UpdateSendingServerData( int clientIndex )
    {
        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < m_numClients );

        CORE_ASSERT( m_config.serverData );
        CORE_ASSERT( m_config.serverData->GetSize() );

        ClientData & client = m_clients[clientIndex];

        CORE_ASSERT( client.state == SERVER_CLIENT_STATE_SENDING_SERVER_DATA );

        client.dataBlockSender->Update( m_timeBase );
    }

    void Server::UpdateReadyForConnection( int clientIndex )
    {
        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < m_numClients );

        #ifndef NDEBUG
        ClientData & client = m_clients[clientIndex];
        #endif

        CORE_ASSERT( client.state == SERVER_CLIENT_STATE_READY_FOR_CONNECTION );

        if ( client.accumulator > 1.0 / m_config.connectingSendRate )
        {
            auto packet = (ReadyForConnectionPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_READY_FOR_CONNECTION );

            packet->clientId = client.clientId;
            packet->serverId = client.serverId;

            SendPacket( client.address, packet );

            client.accumulator = 0.0;
        }
    }

    void Server::UpdateConnected( int clientIndex )
    {
        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < m_numClients );

        ClientData & client = m_clients[clientIndex];

        CORE_ASSERT( client.state == SERVER_CLIENT_STATE_CONNECTED );

        CORE_ASSERT( client.connection );

        client.connection->Update( m_timeBase );

        if ( client.connection->GetError() != CONNECTION_ERROR_NONE )
        {
//            printf( "client connection is in error state\n" );
            ResetClientSlot( clientIndex );
            return;
        }

        if ( client.accumulator > 1.0 / m_config.connectedSendRate )
        {
            auto packet = client.connection->WritePacket();

//                printf( "server sent connection packet\n" );

            packet->clientId = client.clientId;
            packet->serverId = client.serverId;

            SendPacket( client.address, packet );

            client.accumulator = 0.0;
        }
    }

    void Server::UpdateTimeouts( int clientIndex )
    {
        ClientData & client = m_clients[clientIndex];

        if ( client.state == SERVER_CLIENT_STATE_DISCONNECTED )
            return;

        client.accumulator += m_timeBase.deltaTime;

        const float timeout = client.state == SERVER_CLIENT_STATE_CONNECTED ? m_config.connectedTimeOut : m_config.connectingTimeOut;

        if ( client.lastPacketTime + timeout < m_timeBase.time )
        {
            OnClientTimedOut( clientIndex );

            ResetClientSlot( clientIndex );
        }
    }

    void Server::UpdateNetworkSimulator()
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

    void Server::UpdateNetworkInterface()
    {
        CORE_ASSERT( m_config.networkInterface );

        m_config.networkInterface->Update( m_timeBase );
    }

    void Server::UpdateReceivePackets()
    {
        while ( true )
        {
            auto packet = m_config.networkInterface->ReceivePacket();
            if ( !packet )
                break;

//                printf( "server ;packet\n" );

            switch ( packet->GetType() )
            {
                case CLIENT_SERVER_PACKET_CONNECTION_REQUEST:
                    ProcessConnectionRequestPacket( static_cast<ConnectionRequestPacket*>( packet ) );
                    break;

                case CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE:
                    ProcessChallengeResponsePacket( static_cast<ChallengeResponsePacket*>( packet ) );
                    break;

                case CLIENT_SERVER_PACKET_READY_FOR_CONNECTION:
                    ProcessReadyForConnectionPacket( static_cast<ReadyForConnectionPacket*>( packet ) );
                    break;

                case CLIENT_SERVER_PACKET_DISCONNECTED:
                    ProcessDisconnectedPacket( static_cast<DisconnectedPacket*>( packet ) );
                    break;

                case CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT:
                    ProcessDataBlockFragmentPacket( static_cast<DataBlockFragmentPacket*>( packet ) );
                    break;

                case CLIENT_SERVER_PACKET_DATA_BLOCK_FRAGMENT_ACK:
                    ProcessDataBlockFragmentAckPacket( static_cast<DataBlockFragmentAckPacket*>( packet ) );
                    break;

                case CLIENT_SERVER_PACKET_CONNECTION:
                    ProcessConnectionPacket( static_cast<ConnectionPacket*>( packet ) );
                    break;

                default:
                    break;
            }

            m_packetFactory->Destroy( packet );
        }
    }

    void Server::ProcessConnectionRequestPacket( ConnectionRequestPacket * packet )
    {
        CORE_ASSERT( packet );

        // printf( "server received connection request packet: clientGuid = %llx\n", packet->clientGuid );

        auto address = packet->GetAddress();

        if ( !m_open )
        {
            // printf( "server is closed. denying connection request\n" );

            auto connectionDeniedPacket = (ConnectionDeniedPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_CONNECTION_DENIED );
            connectionDeniedPacket->clientId = packet->clientId;
            connectionDeniedPacket->reason = CONNECTION_REQUEST_DENIED_SERVER_CLOSED;

            SendPacket( address, connectionDeniedPacket );

            return;
        }

        int clientIndex = FindClientSlot( address );
        if ( clientIndex != -1 && m_clients[clientIndex].clientId != packet->clientId )
        {
            // printf( "client is already connected. denying connection request\n" );
            auto connectionDeniedPacket = (ConnectionDeniedPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_CONNECTION_DENIED );
            connectionDeniedPacket->clientId = packet->clientId;
            connectionDeniedPacket->reason = CONNECTION_REQUEST_DENIED_ALREADY_CONNECTED;
            SendPacket( address, connectionDeniedPacket );
            return;
        }

        if ( FindClientSlot( address, packet->clientId ) != -1 )
        {
            // printf( "ignoring connection request. client already has a slot\n" );
            return;
        }

        clientIndex = FindFreeClientSlot();
        if ( clientIndex == -1 )
        {
            // printf( "server is full. denying connection request\n" );
            auto connectionDeniedPacket = (ConnectionDeniedPacket*) m_packetFactory->Create( CLIENT_SERVER_PACKET_CONNECTION_DENIED );
            connectionDeniedPacket->clientId = packet->clientId;
            connectionDeniedPacket->reason = CONNECTION_REQUEST_DENIED_SERVER_FULL;
            SendPacket( address, connectionDeniedPacket );
            return;
        }

        // printf( "incoming client connection at index %d\n", clientIndex );

        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < m_numClients );

        ClientData & client = m_clients[clientIndex];

        client.address = address;
        client.clientId = packet->clientId;
        client.serverId = core::generate_id();
        client.lastPacketTime = m_timeBase.time;

        SetClientState( clientIndex, SERVER_CLIENT_STATE_SENDING_CHALLENGE );

        ClientServerInfo info;
        info.address = address;
        info.clientId = client.clientId;
        info.serverId = client.serverId;
        info.packetFactory = m_packetFactory;
        info.networkInterface = m_config.networkInterface;

        if ( client.dataBlockSender )
            client.dataBlockSender->SetInfo( info );

        if ( client.dataBlockReceiver )
            client.dataBlockReceiver->SetInfo( info );

        m_clientServerContext.AddClient( clientIndex, client.address, client.clientId, client.serverId );
    }

    void Server::ProcessChallengeResponsePacket( ChallengeResponsePacket * packet )
    {
        CORE_ASSERT( packet );

        const int clientIndex = FindClientSlot( packet->GetAddress(), packet->clientId, packet->serverId );
        if ( clientIndex == -1 )
            return;

        ClientData & client = m_clients[clientIndex];

        if ( client.state != SERVER_CLIENT_STATE_SENDING_CHALLENGE )
            return;

        client.accumulator = 0.0;
        client.lastPacketTime = m_timeBase.time;
        SetClientState( clientIndex, m_config.serverData ? SERVER_CLIENT_STATE_SENDING_SERVER_DATA : SERVER_CLIENT_STATE_READY_FOR_CONNECTION );
    }

    void Server::ProcessReadyForConnectionPacket( ReadyForConnectionPacket * packet )
    {
        CORE_ASSERT( packet );

        const int clientIndex = FindClientSlot( packet->GetAddress(), packet->clientId );
        if ( clientIndex == -1 )
            return;

        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < m_numClients );

        ClientData & client = m_clients[clientIndex];

        if ( client.serverId != packet->serverId )
            return;

        if ( client.state != SERVER_CLIENT_STATE_SENDING_SERVER_DATA &&
             client.state != SERVER_CLIENT_STATE_READY_FOR_CONNECTION )
            return;

        client.accumulator = 0.0;
        client.lastPacketTime = m_timeBase.time;
        client.readyForConnection = true;
    }

    void Server::ProcessDataBlockFragmentPacket( DataBlockFragmentPacket * packet )
    {
        CORE_ASSERT( packet );

        const int clientIndex = FindClientSlot( packet->GetAddress(), packet->clientId );
        if ( clientIndex == -1 )
            return;
        
        ClientData & client = m_clients[clientIndex];
        if ( client.serverId != packet->serverId )
            return;
        
        if ( !client.dataBlockReceiver )
            return;

//        printf( "process data block fragment %d\n", packet->fragmentId );

        bool receiveAlreadyCompleted = client.dataBlockReceiver->ReceiveCompleted();

        client.dataBlockReceiver->ProcessFragment( packet->blockSize,
                                                   packet->numFragments,
                                                   packet->fragmentId, 
                                                   packet->fragmentBytes,
                                                   packet->fragmentData );

        if ( client.dataBlockReceiver->IsError() )
        {
            ResetClientSlot( clientIndex );
            return;
        }

        if ( client.dataBlockReceiver->ReceiveCompleted() && !receiveAlreadyCompleted )
            OnClientDataReceived( clientIndex, *client.dataBlockReceiver->GetBlock() );
    }

    void Server::ProcessDataBlockFragmentAckPacket( DataBlockFragmentAckPacket * packet )
    {
        CORE_ASSERT( packet );

        const int clientIndex = FindClientSlot( packet->GetAddress(), packet->clientId );
        if ( clientIndex == -1 )
            return;
        
        ClientData & client = m_clients[clientIndex];
        if ( client.serverId != packet->serverId )
            return;
        
        if ( !client.dataBlockSender )
            return;

        if ( client.dataBlockSender->SendCompleted() )
            return;

        client.dataBlockSender->ProcessAck( packet->fragmentId );

        if ( client.dataBlockSender->SendCompleted() )
        {
            client.accumulator = 0.0;
            client.lastPacketTime = m_timeBase.time;
            SetClientState( clientIndex, SERVER_CLIENT_STATE_READY_FOR_CONNECTION );
        }
    }

    void Server::ProcessDisconnectedPacket( DisconnectedPacket * packet )
    {
        CORE_ASSERT( packet );

        const int clientIndex = FindClientSlot( packet->GetAddress(), packet->clientId );
        if ( clientIndex == -1 )
            return;

        ClientData & client = m_clients[clientIndex];
        if ( client.serverId != packet->serverId )
            return;

        ResetClientSlot( clientIndex );
    }

    void Server::ProcessConnectionPacket( ConnectionPacket * packet )
    {
        CORE_ASSERT( packet );

        if ( packet->clientId == 0 && packet->serverId == 0 )
            return;

        const int clientIndex = FindClientSlot( packet->GetAddress(), packet->clientId, packet->serverId );
        if ( clientIndex == -1 )
            return;

        ClientData & client = m_clients[clientIndex];
        if ( client.state != SERVER_CLIENT_STATE_CONNECTED )
            return;

        client.connection->ReadPacket( packet );

        client.lastPacketTime = m_timeBase.time;
    }

    int Server::FindClientSlot( const network::Address & address ) const
    {
        return m_clientServerContext.FindClient( address );
    }

    int Server::FindClientSlot( const network::Address & address, uint64_t clientGuid ) const
    {
        return m_clientServerContext.FindClient( address, clientGuid );
    }

    int Server::FindClientSlot( const network::Address & address, uint64_t clientGuid, uint64_t serverGuid ) const
    {
        return m_clientServerContext.FindClient( address, clientGuid, serverGuid );
    }

    int Server::FindFreeClientSlot() const
    {
        for ( int i = 0; i < m_numClients; ++i )
        {
            if ( m_clients[i].state == SERVER_CLIENT_STATE_DISCONNECTED )
                return i;
        }
        return -1;
    }

    void Server::ResetClientSlot( int clientIndex )
    {
//          printf( "reset client slot %d\n", clientIndex );

        ClientData & client = m_clients[clientIndex];

        SetClientState( clientIndex, SERVER_CLIENT_STATE_DISCONNECTED );

        client.Clear();

        CORE_ASSERT( client.connection );

        client.connection->Reset();

        m_clientServerContext.RemoveClient( clientIndex );
    }

    void Server::SendPacket( const network::Address & address, Packet * packet )
    {
        auto interface = m_config.networkSimulator ? m_config.networkSimulator : m_config.networkInterface;
        interface->SendPacket( address, packet );
    }

    void Server::SetClientState( int clientIndex, ServerClientState state )
    {
        CORE_ASSERT( clientIndex >= 0 );
        CORE_ASSERT( clientIndex < m_numClients );

        if ( state != m_clients[clientIndex].state )
        {
            OnClientStateChange( clientIndex, m_clients[clientIndex].state, state );
            m_clients[clientIndex].state = state;
        }
    }
}
