// todo: get test_interface etc. workingr

#if 0

struct ConnectPacket : public Packet
{
    int a,b,c;

    ConnectPacket() : Packet( PACKET_Connect )
    {
        a = 1;
        b = 2;
        c = 3;        
    }

    void Serialize( Stream & stream )
    {
        serialize_int( stream, a, -10, 10 );
        serialize_int( stream, b, -10, 10 );
        serialize_int( stream, c, -10, 10 );
    }

    bool operator ==( const ConnectPacket & other ) const
    {
        return a == other.a && b == other.b && c == other.c;
    }

    bool operator !=( const ConnectPacket & other ) const
    {
        return !( *this == other );
    }
};

struct UpdatePacket : public Packet
{
    uint16_t timestamp;

    UpdatePacket() : Packet( PACKET_Update )
    {
        timestamp = 0;
    }

    void Serialize( Stream & stream )
    {
        serialize_int( stream, timestamp, 0, 65535 );
    }

    bool operator ==( const UpdatePacket & other ) const
    {
        return timestamp == other.timestamp;
    }

    bool operator !=( const UpdatePacket & other ) const
    {
        return !( *this == other );
    }
};

struct DisconnectPacket : public Packet
{
    int x;

    DisconnectPacket() : Packet( PACKET_Disconnect ) 
    {
        x = 2;
    }

    void Serialize( Stream & stream )
    {
        serialize_int( stream, x, -100, +100 );
    }

    bool operator ==( const DisconnectPacket & other ) const
    {
        return x == other.x;
    }

    bool operator !=( const DisconnectPacket & other ) const
    {
        return !( *this == other );
    }
};

class PacketFactory : public Factory<Packet>
{
public:
    PacketFactory()
    {
        Register( PACKET_Connect, [] { return make_shared<ConnectPacket>(); } );
        Register( PACKET_Update, [] { return make_shared<UpdatePacket>(); } );
        Register( PACKET_Disconnect, [] { return make_shared<DisconnectPacket>(); } );
    }
};

class TestInterface : public Interface
{
public:

    virtual void SendPacket( const Address & address, shared_ptr<Packet> packet )
    {
        packet->SetAddress( address );
        packet_queue.push( packet );
    }

    virtual void SendPacket( const string & address, uint16_t port, shared_ptr<Packet> packet )
    {
        assert( false );        // not supported
    }

    virtual shared_ptr<Packet> ReceivePacket()
    {
        if ( packet_queue.empty() )
            return nullptr;
        auto packet = packet_queue.front();
        packet_queue.pop();
        return packet;
    }

    virtual void Update()
    {
        // ...
    }

    PacketQueue packet_queue;
};

void test_interface()
{
    cout << "test_interface" << endl;

    TestInterface interface;

    Address address( 127,0,0,1, 1000 );

    interface.SendPacket( address, make_shared<ConnectPacket>() );
    interface.SendPacket( address, make_shared<UpdatePacket>() );
    interface.SendPacket( address, make_shared<DisconnectPacket>() );

    auto connectPacket = interface.ReceivePacket();
    auto updatePacket = interface.ReceivePacket();
    auto disconnectPacket = interface.ReceivePacket();

    assert( connectPacket->GetType() == PACKET_Connect );
    assert( connectPacket->GetAddress() == address );

    assert( updatePacket->GetType() == PACKET_Update );
    assert( updatePacket->GetAddress() == address );

    assert( disconnectPacket->GetType() == PACKET_Disconnect );
    assert( disconnectPacket->GetAddress() == address );

    assert( interface.ReceivePacket() == nullptr );
}

void test_network_interface_send_to_hostname()
{
    cout << "test_network_interface_send_to_hostname" << endl;

    NetworkInterface::Config config;

    auto factory = make_shared<PacketFactory>();

    config.port = 10000;
    config.maxPacketSize = 1024;
    config.factory = static_pointer_cast<Factory<Packet>>( factory );
    config.resolver = make_shared<DNSResolver>();

    NetworkInterface interface( config );

    int numPackets = 10;

    for ( int i = 0; i < numPackets; ++i )
    {
        auto packet = factory->Create( PACKET_Connect );

        interface.SendPacket( "google.com", config.port, packet );
    }

    auto start = chrono::steady_clock::now();

    double t = 0.0;
    double dt = 0.1f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    for ( int i = 0; i < 20; ++i )
    {
        interface.Update();

        if ( interface.GetCounter( NetworkInterface::PacketsSent ) == numPackets )
            break;

        this_thread::sleep_for( ms );

        t += dt;
    }

    auto finish = chrono::steady_clock::now();

    auto delta = finish - start;

    cout << chrono::duration<double,milli>( delta ).count() << " ms" << endl;
}

void test_network_interface_send_to_hostname_failure()
{
    cout << "test_network_interface_send_to_hostname_failure" << endl;

    NetworkInterface::Config config;

    auto factory = make_shared<PacketFactory>();

    config.port = 10000;
    config.maxPacketSize = 1024;
    config.factory = static_pointer_cast<Factory<Packet>>( factory );
    config.resolver = make_shared<DNSResolver>();

    NetworkInterface interface( config );

    int numPackets = 10;

    for ( int i = 0; i < numPackets; ++i )
    {
        auto packet = factory->Create( PACKET_Connect );

        interface.SendPacket( "aoesortuhantuehanthua", config.port, packet );
    }

    auto start = chrono::steady_clock::now();

    double t = 0.0;
    double dt = 0.1f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    for ( int i = 0; i < 20; ++i )
    {
        interface.Update();

        if ( interface.GetCounter( NetworkInterface::PacketsDiscarded ) == numPackets )
            break;

        this_thread::sleep_for( ms );

        t += dt;
    }

    auto finish = chrono::steady_clock::now();

    auto delta = finish - start;

    cout << chrono::duration<double,milli>( delta ).count() << " ms" << endl;
}

void test_network_interface_send_and_receive_ipv4()
{
    cout << "test_network_interface_send_and_receive_ipv4" << endl;

    NetworkInterface::Config config;

    auto factory = make_shared<PacketFactory>();

    config.port = 10000;
    config.maxPacketSize = 1024;
    config.factory = static_pointer_cast<Factory<Packet>>( factory );
    config.resolver = make_shared<DNSResolver>( AF_INET );

    NetworkInterface interface( config );

    Address address( "127.0.0.1" );
    address.SetPort( config.port );

    double dt = 0.01f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    bool receivedConnectPacket = false;
    bool receivedUpdatePacket = false;
    bool receivedDisconnectPacket = false;

    int iterations = 0;

    while ( true )
    {
        assert ( iterations++ < 10 );

        auto connectPacket = make_shared<ConnectPacket>();
        auto updatePacket = make_shared<UpdatePacket>();
        auto disconnectPacket = make_shared<DisconnectPacket>();

        connectPacket->a = 2;
        connectPacket->b = 6;
        connectPacket->c = -1;

        updatePacket->timestamp = 500;

        disconnectPacket->x = -100;

        interface.SendPacket( address, connectPacket );
        interface.SendPacket( address, updatePacket );
        interface.SendPacket( address, disconnectPacket );

        interface.Update();

        this_thread::sleep_for( ms );

        while ( true )
        {
            auto packet = interface.ReceivePacket();
            if ( !packet )
                break;

            assert( packet->GetAddress() == address );

            switch ( packet->GetType() )
            {
                case PACKET_Connect:
                {
                    cout << "received connect packet" << endl;
                    auto recv_connectPacket = static_pointer_cast<ConnectPacket>( packet );
                    assert( *recv_connectPacket == *connectPacket );
                    receivedConnectPacket = true;
                }
                break;

                case PACKET_Update:
                {
                    cout << "received update packet" << endl;
                    auto recv_updatePacket = static_pointer_cast<UpdatePacket>( packet );
                    assert( *recv_updatePacket == *updatePacket );
                    receivedUpdatePacket = true;
                }
                break;

                case PACKET_Disconnect:
                {
                    cout << "received disconnect packet" << endl;
                    auto recv_disconnectPacket = static_pointer_cast<DisconnectPacket>( packet );
                    assert( *recv_disconnectPacket == *disconnectPacket );
                    receivedDisconnectPacket = true;
                }
                break;
            }
        }

        if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
            break;
    }
}

void test_network_interface_send_and_receive_ipv6()
{
    cout << "test_network_interface_send_and_receive_ipv6" << endl;

    NetworkInterface::Config config;

    auto factory = make_shared<PacketFactory>();

    config.port = 10000;
    config.family = AF_INET6;
    config.maxPacketSize = 1024;
    config.factory = static_pointer_cast<Factory<Packet>>( factory );
    config.resolver = make_shared<DNSResolver>( AF_INET6 );

    NetworkInterface interface( config );

    Address address( "::1" );
    address.SetPort( config.port );

    double dt = 0.01f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    bool receivedConnectPacket = false;
    bool receivedUpdatePacket = false;
    bool receivedDisconnectPacket = false;

    int iterations = 0;

    while ( true )
    {
        assert ( iterations++ < 10 );

        auto connectPacket = make_shared<ConnectPacket>();
        auto updatePacket = make_shared<UpdatePacket>();
        auto disconnectPacket = make_shared<DisconnectPacket>();

        connectPacket->a = 2;
        connectPacket->b = 6;
        connectPacket->c = -1;

        updatePacket->timestamp = 500;

        disconnectPacket->x = -100;

        interface.SendPacket( address, connectPacket );
        interface.SendPacket( address, updatePacket );
        interface.SendPacket( address, disconnectPacket );

        interface.Update();

        this_thread::sleep_for( ms );

        while ( true )
        {
            auto packet = interface.ReceivePacket();
            if ( !packet )
                break;

            cout << "receive packet from address " << packet->GetAddress().ToString() << endl;

            assert( packet->GetAddress() == address );

            switch ( packet->GetType() )
            {
                case PACKET_Connect:
                {
                    cout << "received connect packet" << endl;
                    auto recv_connectPacket = static_pointer_cast<ConnectPacket>( packet );
                    assert( *recv_connectPacket == *connectPacket );
                    receivedConnectPacket = true;
                }
                break;

                case PACKET_Update:
                {
                    cout << "received update packet" << endl;
                    auto recv_updatePacket = static_pointer_cast<UpdatePacket>( packet );
                    assert( *recv_updatePacket == *updatePacket );
                    receivedUpdatePacket = true;
                }
                break;

                case PACKET_Disconnect:
                {
                    cout << "received disconnect packet" << endl;
                    auto recv_disconnectPacket = static_pointer_cast<DisconnectPacket>( packet );
                    assert( *recv_disconnectPacket == *disconnectPacket );
                    receivedDisconnectPacket = true;
                }
                break;
            }
        }

        if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
            break;
    }
}

#endif
