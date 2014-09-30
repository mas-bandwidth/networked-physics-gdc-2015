#include "network/Network.h"
#include "network/BSDSocket.h"
#include "TestPackets.h"

void test_bsd_socket_send_and_receive_ipv4()
{
    printf( "test_bsd_socket_send_and_receive_ipv4\n" );

    core::memory::initialize();
    {
        network::BSDSocketConfig config;

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        config.port = 10000;
        config.ipv6 = false;
        config.maxPacketSize = 1024;
        config.packetFactory = &packetFactory;

        network::BSDSocket interface( config );

        network::Address address( "127.0.0.1" );
        address.SetPort( config.port );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        bool receivedConnectPacket = false;
        bool receivedUpdatePacket = false;
        bool receivedDisconnectPacket = false;

        ConnectPacket connectPacketTemplate;
        UpdatePacket updatePacketTemplate;
        DisconnectPacket disconnectPacketTemplate;

        connectPacketTemplate.a = 2;
        connectPacketTemplate.b = 6;
        connectPacketTemplate.c = -1;

        updatePacketTemplate.timestamp = 500;

        disconnectPacketTemplate.x = -100;

        while ( true )
        {
            auto connectPacket = (ConnectPacket*) packetFactory.Create( PACKET_CONNECT );
            auto updatePacket = (UpdatePacket*) packetFactory.Create( PACKET_UPDATE );
            auto disconnectPacket = (DisconnectPacket*) packetFactory.Create( PACKET_DISCONNECT );

            *connectPacket = connectPacketTemplate;
            *updatePacket = updatePacketTemplate;
            *disconnectPacket = disconnectPacketTemplate;

            CORE_CHECK( *connectPacket == connectPacketTemplate );
            CORE_CHECK( *updatePacket == updatePacketTemplate );
            CORE_CHECK( *disconnectPacket == disconnectPacketTemplate );

            interface.SendPacket( address, connectPacket );
            interface.SendPacket( address, updatePacket );
            interface.SendPacket( address, disconnectPacket );

            interface.Update( timeBase );

            while ( true )
            {
                auto packet = interface.ReceivePacket();
                if ( !packet )
                    break;

                CORE_CHECK( packet->GetAddress() == address );

                switch ( packet->GetType() )
                {
                    case PACKET_CONNECT:
                    {
                        auto recv_connectPacket = static_cast<ConnectPacket*>( packet );
                        CORE_CHECK( *recv_connectPacket == connectPacketTemplate );
                        receivedConnectPacket = true;
                    }
                    break;

                    case PACKET_UPDATE:
                    {
                        auto recv_updatePacket = static_cast<UpdatePacket*>( packet );
                        CORE_CHECK( *recv_updatePacket == updatePacketTemplate );
                        receivedUpdatePacket = true;
                    }
                    break;

                    case PACKET_DISCONNECT:
                    {
                        auto recv_disconnectPacket = static_cast<DisconnectPacket*>( packet );
                        CORE_CHECK( *recv_disconnectPacket == disconnectPacketTemplate );
                        receivedDisconnectPacket = true;
                    }
                    break;
                }

                packetFactory.Destroy( packet );
            }

            if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
                break;

            timeBase.time += timeBase.deltaTime;
        }
    }
    core::memory::shutdown();
}

void test_bsd_socket_send_and_receive_ipv6()
{
    printf( "test_bsd_socket_send_and_receive_ipv6\n" );

    core::memory::initialize();
    {
        network::BSDSocketConfig config;

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        config.port = 10000;
        config.maxPacketSize = 1024;
        config.packetFactory = &packetFactory;

        network::BSDSocket interface( config );

        network::Address address( "::1" );
        address.SetPort( config.port );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        bool receivedConnectPacket = false;
        bool receivedUpdatePacket = false;
        bool receivedDisconnectPacket = false;

        ConnectPacket connectPacketTemplate;
        UpdatePacket updatePacketTemplate;
        DisconnectPacket disconnectPacketTemplate;

        connectPacketTemplate.a = 2;
        connectPacketTemplate.b = 6;
        connectPacketTemplate.c = -1;

        updatePacketTemplate.timestamp = 500;

        disconnectPacketTemplate.x = -100;

        while ( true )
        {
            auto connectPacket = (ConnectPacket*) packetFactory.Create( PACKET_CONNECT );
            auto updatePacket = (UpdatePacket*) packetFactory.Create( PACKET_UPDATE );
            auto disconnectPacket = (DisconnectPacket*) packetFactory.Create( PACKET_DISCONNECT );

            *connectPacket = connectPacketTemplate;
            *updatePacket = updatePacketTemplate;
            *disconnectPacket = disconnectPacketTemplate;

            CORE_CHECK( *connectPacket == connectPacketTemplate );
            CORE_CHECK( *updatePacket == updatePacketTemplate );
            CORE_CHECK( *disconnectPacket == disconnectPacketTemplate );

            interface.SendPacket( address, connectPacket );
            interface.SendPacket( address, updatePacket );
            interface.SendPacket( address, disconnectPacket );

            interface.Update( timeBase );

            while ( true )
            {
                auto packet = interface.ReceivePacket();
                if ( !packet )
                    break;

                CORE_CHECK( packet->GetAddress() == address );

                switch ( packet->GetType() )
                {
                    case PACKET_CONNECT:
                    {
                        auto recv_connectPacket = static_cast<ConnectPacket*>( packet );
                        CORE_CHECK( *recv_connectPacket == connectPacketTemplate );
                        receivedConnectPacket = true;
                    }
                    break;

                    case PACKET_UPDATE:
                    {
                        auto recv_updatePacket = static_cast<UpdatePacket*>( packet );
                        CORE_CHECK( *recv_updatePacket == updatePacketTemplate );
                        receivedUpdatePacket = true;
                    }
                    break;

                    case PACKET_DISCONNECT:
                    {
                        auto recv_disconnectPacket = static_cast<DisconnectPacket*>( packet );
                        CORE_CHECK( *recv_disconnectPacket == disconnectPacketTemplate );
                        receivedDisconnectPacket = true;
                    }
                    break;
                }

                packetFactory.Destroy( packet );
            }

            if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
                break;

            timeBase.time += timeBase.deltaTime;
        }
    }
    core::memory::shutdown();
}

void test_bsd_socket_send_and_receive_multiple_ipv4()
{
    printf( "test_bsd_socket_send_and_receive_multiple_ipv4\n" );

    core::memory::initialize();
    {
        TestPacketFactory packetFactory( core::memory::default_allocator() );

        network::BSDSocketConfig sender_config;
        sender_config.port = 10000;
        sender_config.ipv6 = false;
        sender_config.maxPacketSize = 1024;
        sender_config.packetFactory = &packetFactory;

        network::BSDSocket interface_sender( sender_config );
        
        network::BSDSocketConfig receiver_config;
        receiver_config.port = 10001;
        receiver_config.ipv6 = false;
        receiver_config.maxPacketSize = 1024;
        receiver_config.packetFactory = &packetFactory;

        network::BSDSocket interface_receiver( receiver_config );

        network::Address sender_address( "[127.0.0.1]:10000" );
        network::Address receiver_address( "[127.0.0.1]:10001" );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        bool receivedConnectPacket = false;
        bool receivedUpdatePacket = false;
        bool receivedDisconnectPacket = false;

        ConnectPacket connectPacketTemplate;
        UpdatePacket updatePacketTemplate;
        DisconnectPacket disconnectPacketTemplate;

        connectPacketTemplate.a = 2;
        connectPacketTemplate.b = 6;
        connectPacketTemplate.c = -1;

        updatePacketTemplate.timestamp = 500;

        disconnectPacketTemplate.x = -100;

        while ( true )
        {
            auto connectPacket = (ConnectPacket*) packetFactory.Create( PACKET_CONNECT );
            auto updatePacket = (UpdatePacket*) packetFactory.Create( PACKET_UPDATE );
            auto disconnectPacket = (DisconnectPacket*) packetFactory.Create( PACKET_DISCONNECT );

            *connectPacket = connectPacketTemplate;
            *updatePacket = updatePacketTemplate;
            *disconnectPacket = disconnectPacketTemplate;

            CORE_CHECK( *connectPacket == connectPacketTemplate );
            CORE_CHECK( *updatePacket == updatePacketTemplate );
            CORE_CHECK( *disconnectPacket == disconnectPacketTemplate );

            interface_sender.SendPacket( receiver_address, connectPacket );
            interface_sender.SendPacket( receiver_address, updatePacket );
            interface_sender.SendPacket( receiver_address, disconnectPacket );

            interface_sender.Update( timeBase );
            interface_receiver.Update( timeBase );

            while ( true )
            {
                auto packet = interface_receiver.ReceivePacket();
                if ( !packet )
                    break;

                CORE_CHECK( packet->GetAddress() == sender_address );

                switch ( packet->GetType() )
                {
                    case PACKET_CONNECT:
                    {
                        auto recv_connectPacket = static_cast<ConnectPacket*>( packet );
                        CORE_CHECK( *recv_connectPacket == connectPacketTemplate );
                        receivedConnectPacket = true;
                    }
                    break;

                    case PACKET_UPDATE:
                    {
                        auto recv_updatePacket = static_cast<UpdatePacket*>( packet );
                        CORE_CHECK( *recv_updatePacket == updatePacketTemplate );
                        receivedUpdatePacket = true;
                    }
                    break;

                    case PACKET_DISCONNECT:
                    {
                        auto recv_disconnectPacket = static_cast<DisconnectPacket*>( packet );
                        CORE_CHECK( *recv_disconnectPacket == disconnectPacketTemplate );
                        receivedDisconnectPacket = true;
                    }
                    break;
                }

                packetFactory.Destroy( packet );
            }

            if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
                break;

            timeBase.time += timeBase.deltaTime;
        }
    }
    core::memory::shutdown();
}

void test_bsd_socket_send_and_receive_multiple_ipv6()
{
    printf( "test_bsd_socket_send_and_receive_multiple_ipv6\n" );

    core::memory::initialize();
    {
        TestPacketFactory packetFactory( core::memory::default_allocator() );

        network::BSDSocketConfig sender_config;
        sender_config.port = 10000;
        sender_config.maxPacketSize = 1024;
        sender_config.packetFactory = &packetFactory;

        network::BSDSocket interface_sender( sender_config );
        
        network::BSDSocketConfig receiver_config;
        receiver_config.port = 10001;
        receiver_config.maxPacketSize = 1024;
        receiver_config.packetFactory = &packetFactory;

        network::BSDSocket interface_receiver( receiver_config );

        network::Address sender_address( "[::1]:10000" );
        network::Address receiver_address( "[::1]:10001" );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        bool receivedConnectPacket = false;
        bool receivedUpdatePacket = false;
        bool receivedDisconnectPacket = false;

        ConnectPacket connectPacketTemplate;
        UpdatePacket updatePacketTemplate;
        DisconnectPacket disconnectPacketTemplate;

        connectPacketTemplate.a = 2;
        connectPacketTemplate.b = 6;
        connectPacketTemplate.c = -1;

        updatePacketTemplate.timestamp = 500;

        disconnectPacketTemplate.x = -100;

        while ( true )
        {
            auto connectPacket = (ConnectPacket*) packetFactory.Create( PACKET_CONNECT );
            auto updatePacket = (UpdatePacket*) packetFactory.Create( PACKET_UPDATE );
            auto disconnectPacket = (DisconnectPacket*) packetFactory.Create( PACKET_DISCONNECT );

            *connectPacket = connectPacketTemplate;
            *updatePacket = updatePacketTemplate;
            *disconnectPacket = disconnectPacketTemplate;

            CORE_CHECK( *connectPacket == connectPacketTemplate );
            CORE_CHECK( *updatePacket == updatePacketTemplate );
            CORE_CHECK( *disconnectPacket == disconnectPacketTemplate );

            interface_sender.SendPacket( receiver_address, connectPacket );
            interface_sender.SendPacket( receiver_address, updatePacket );
            interface_sender.SendPacket( receiver_address, disconnectPacket );

            interface_sender.Update( timeBase );
            interface_receiver.Update( timeBase );

            while ( true )
            {
                auto packet = interface_receiver.ReceivePacket();
                if ( !packet )
                    break;

                CORE_CHECK( packet->GetAddress() == sender_address );

                switch ( packet->GetType() )
                {
                    case PACKET_CONNECT:
                    {
                        auto recv_connectPacket = static_cast<ConnectPacket*>( packet );
                        CORE_CHECK( *recv_connectPacket == connectPacketTemplate );
                        receivedConnectPacket = true;
                    }
                    break;

                    case PACKET_UPDATE:
                    {
                        auto recv_updatePacket = static_cast<UpdatePacket*>( packet );
                        CORE_CHECK( *recv_updatePacket == updatePacketTemplate );
                        receivedUpdatePacket = true;
                    }
                    break;

                    case PACKET_DISCONNECT:
                    {
                        auto recv_disconnectPacket = static_cast<DisconnectPacket*>( packet );
                        CORE_CHECK( *recv_disconnectPacket == disconnectPacketTemplate );
                        receivedDisconnectPacket = true;
                    }
                    break;
                }

                packetFactory.Destroy( packet );
            }

            if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
                break;

            timeBase.time += timeBase.deltaTime;
        }
    }
    core::memory::shutdown();
}
