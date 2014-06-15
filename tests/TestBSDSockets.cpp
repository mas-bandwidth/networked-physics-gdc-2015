#include "BSDSockets.h"

using namespace protocol;

enum PacketType
{
    PACKET_CONNECT = 1,
    PACKET_UPDATE,
    PACKET_DISCONNECT
};

struct ConnectPacket : public Packet
{
    int a,b,c;

    ConnectPacket() : Packet( PACKET_CONNECT )
    {
        a = 1;
        b = 2;
        c = 3;        
    }

    template <typename Stream> void Serialize( Stream & stream )
    {
        serialize_int( stream, a, -10, 10 );
        serialize_int( stream, b, -10, 10 );
        serialize_int( stream, c, -10, 10 );
    }

    void SerializeRead( ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( MeasureStream & stream )
    {
        Serialize( stream );
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

    UpdatePacket() : Packet( PACKET_UPDATE )
    {
        timestamp = 0;
    }

    template <typename Stream> void Serialize( Stream & stream )
    {
        serialize_bits( stream, timestamp, 16 );
    }

    void SerializeRead( ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( MeasureStream & stream )
    {
        Serialize( stream );
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

    DisconnectPacket() : Packet( PACKET_DISCONNECT )
    {
        x = 2;
    }

    template <typename Stream> void Serialize( Stream & stream )
    {
        serialize_int( stream, x, -100, +100 );
    }

    void SerializeRead( ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( MeasureStream & stream )
    {
        Serialize( stream );
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
        Register( PACKET_CONNECT,    [] { return new ConnectPacket();    } );
        Register( PACKET_UPDATE,     [] { return new UpdatePacket();     } );
        Register( PACKET_DISCONNECT, [] { return new DisconnectPacket(); } );
    }
};

void test_bsd_sockets_send_and_receive_ipv4()
{
    printf( "test_bsd_sockets_send_and_receive_ipv4\n" );

    BSDSocketsConfig config;

    PacketFactory packetFactory;

    config.port = 10000;
    config.family = AF_INET;
    config.maxPacketSize = 1024;
    config.packetFactory = &packetFactory;

    BSDSockets interface( config );

    Address address( "127.0.0.1" );
    address.SetPort( config.port );

    TimeBase timeBase;
    timeBase.deltaTime = 0.01f;

    std::chrono::milliseconds ms( (int) ( timeBase.deltaTime * 1000 ) );

    bool receivedConnectPacket = false;
    bool receivedUpdatePacket = false;
    bool receivedDisconnectPacket = false;

    int iterations = 0;

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
        assert ( iterations++ < 10 );

        auto connectPacket = new ConnectPacket();
        auto updatePacket = new UpdatePacket();
        auto disconnectPacket = new DisconnectPacket();

        *connectPacket = connectPacketTemplate;
        *updatePacket = updatePacketTemplate;
        *disconnectPacket = disconnectPacketTemplate;

        interface.SendPacket( address, connectPacket );
        interface.SendPacket( address, updatePacket );
        interface.SendPacket( address, disconnectPacket );

        interface.Update( timeBase );

        std::this_thread::sleep_for( ms );

        while ( true )
        {
            auto packet = interface.ReceivePacket();
            if ( !packet )
                break;

            assert( packet->GetAddress() == address );

            switch ( packet->GetType() )
            {
                case PACKET_CONNECT:
                {
                    auto recv_connectPacket = static_cast<ConnectPacket*>( packet );
                    assert( *recv_connectPacket == connectPacketTemplate );
                    receivedConnectPacket = true;
                }
                break;

                case PACKET_UPDATE:
                {
                    auto recv_updatePacket = static_cast<UpdatePacket*>( packet );
                    assert( *recv_updatePacket == updatePacketTemplate );
                    receivedUpdatePacket = true;
                }
                break;

                case PACKET_DISCONNECT:
                {
                    auto recv_disconnectPacket = static_cast<DisconnectPacket*>( packet );
                    assert( *recv_disconnectPacket == disconnectPacketTemplate );
                    receivedDisconnectPacket = true;
                }
                break;
            }

            delete packet;
        }

        if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
            break;

        timeBase.time += timeBase.deltaTime;
    }
}

void test_bsd_sockets_send_and_receive_ipv6()
{
    printf( "test_bsd_sockets_send_and_receive_ipv6\n" );

    BSDSocketsConfig config;

    PacketFactory packetFactory;

    config.port = 10000;
    config.family = AF_INET6;
    config.maxPacketSize = 1024;
    config.packetFactory = &packetFactory;

    BSDSockets interface( config );

    Address address( "::1" );
    address.SetPort( config.port );

    TimeBase timeBase;
    timeBase.deltaTime = 0.01f;

    std::chrono::milliseconds ms( (int) ( timeBase.deltaTime * 1000 ) );

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

    int iterations = 0;

    while ( true )
    {
        assert ( iterations++ < 10 );

        auto connectPacket = new ConnectPacket();
        auto updatePacket = new UpdatePacket();
        auto disconnectPacket = new DisconnectPacket();

        *connectPacket = connectPacketTemplate;
        *updatePacket = updatePacketTemplate;
        *disconnectPacket = disconnectPacketTemplate;

        interface.SendPacket( address, connectPacket );
        interface.SendPacket( address, updatePacket );
        interface.SendPacket( address, disconnectPacket );

        interface.Update( timeBase );

        std::this_thread::sleep_for( ms );

        while ( true )
        {
            auto packet = interface.ReceivePacket();
            if ( !packet )
                break;

            assert( packet->GetAddress() == address );

            switch ( packet->GetType() )
            {
                case PACKET_CONNECT:
                {
                    auto recv_connectPacket = static_cast<ConnectPacket*>( packet );
                    assert( *recv_connectPacket == connectPacketTemplate );
                    receivedConnectPacket = true;
                }
                break;

                case PACKET_UPDATE:
                {
                    auto recv_updatePacket = static_cast<UpdatePacket*>( packet );
                    assert( *recv_updatePacket == updatePacketTemplate );
                    receivedUpdatePacket = true;
                }
                break;

                case PACKET_DISCONNECT:
                {
                    auto recv_disconnectPacket = static_cast<DisconnectPacket*>( packet );
                    assert( *recv_disconnectPacket == disconnectPacketTemplate );
                    receivedDisconnectPacket = true;
                }
                break;
            }

            delete packet;
        }

        if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
            break;

        timeBase.time += timeBase.deltaTime;
    }
}

void test_bsd_sockets_send_and_receive_multiple_interfaces_ipv4()
{
    printf( "test_bsd_sockets_send_and_receive_multiple_interfaces_ipv4\n" );

    PacketFactory packetFactory;

    BSDSocketsConfig sender_config;
    sender_config.port = 10000;
    sender_config.family = AF_INET;
    sender_config.maxPacketSize = 1024;
    sender_config.packetFactory = &packetFactory;

    BSDSockets interface_sender( sender_config );
    
    BSDSocketsConfig receiver_config;
    receiver_config.port = 10001;
    receiver_config.family = AF_INET;
    receiver_config.maxPacketSize = 1024;
    receiver_config.packetFactory = &packetFactory;

    BSDSockets interface_receiver( receiver_config );

    Address sender_address( "[127.0.0.1]:10000" );
    Address receiver_address( "[127.0.0.1]:10001" );

    TimeBase timeBase;
    timeBase.deltaTime = 0.01f;

    std::chrono::milliseconds ms( (int) ( timeBase.deltaTime * 1000 ) );

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

    int iterations = 0;

    while ( true )
    {
        assert ( iterations++ < 4 );

        auto connectPacket = new ConnectPacket();
        auto updatePacket = new UpdatePacket();
        auto disconnectPacket = new DisconnectPacket();

        *connectPacket = connectPacketTemplate;
        *updatePacket = updatePacketTemplate;
        *disconnectPacket = disconnectPacketTemplate;

        interface_sender.SendPacket( receiver_address, connectPacket );
        interface_sender.SendPacket( receiver_address, updatePacket );
        interface_sender.SendPacket( receiver_address, disconnectPacket );

        interface_sender.Update( timeBase );
        interface_receiver.Update( timeBase );

        std::this_thread::sleep_for( ms );

        while ( true )
        {
            auto packet = interface_receiver.ReceivePacket();
            if ( !packet )
                break;

            assert( packet->GetAddress() == sender_address );

            switch ( packet->GetType() )
            {
                case PACKET_CONNECT:
                {
                    auto recv_connectPacket = static_cast<ConnectPacket*>( packet );
                    assert( *recv_connectPacket == connectPacketTemplate );
                    receivedConnectPacket = true;
                }
                break;

                case PACKET_UPDATE:
                {
                    auto recv_updatePacket = static_cast<UpdatePacket*>( packet );
                    assert( *recv_updatePacket == updatePacketTemplate );
                    receivedUpdatePacket = true;
                }
                break;

                case PACKET_DISCONNECT:
                {
                    auto recv_disconnectPacket = static_cast<DisconnectPacket*>( packet );
                    assert( *recv_disconnectPacket == disconnectPacketTemplate );
                    receivedDisconnectPacket = true;
                }
                break;
            }

            delete packet;
        }

        if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
            break;

        timeBase.time += timeBase.deltaTime;
    }
}

void test_bsd_sockets_send_and_receive_multiple_interfaces_ipv6()
{
    printf( "test_bsd_sockets_send_and_receive_multiple_interfaces_ipv6\n" );

    PacketFactory packetFactory;

    BSDSocketsConfig sender_config;
    sender_config.port = 10000;
    sender_config.family = AF_INET6;
    sender_config.maxPacketSize = 1024;
    sender_config.packetFactory = &packetFactory;

    BSDSockets interface_sender( sender_config );
    
    BSDSocketsConfig receiver_config;
    receiver_config.port = 10001;
    receiver_config.family = AF_INET6;
    receiver_config.maxPacketSize = 1024;
    receiver_config.packetFactory = &packetFactory;

    BSDSockets interface_receiver( receiver_config );

    Address sender_address( "[::1]:10000" );
    Address receiver_address( "[::1]:10001" );

    TimeBase timeBase;
    timeBase.deltaTime = 0.01f;

    std::chrono::milliseconds ms( (int) ( timeBase.deltaTime * 1000 ) );

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

    int iterations = 0;

    while ( true )
    {
        assert ( iterations++ < 4 );

        auto connectPacket = new ConnectPacket();
        auto updatePacket = new UpdatePacket();
        auto disconnectPacket = new DisconnectPacket();

        *connectPacket = connectPacketTemplate;
        *updatePacket = updatePacketTemplate;
        *disconnectPacket = disconnectPacketTemplate;

        interface_sender.SendPacket( receiver_address, connectPacket );
        interface_sender.SendPacket( receiver_address, updatePacket );
        interface_sender.SendPacket( receiver_address, disconnectPacket );

        interface_sender.Update( timeBase );
        interface_receiver.Update( timeBase );

        std::this_thread::sleep_for( ms );

        while ( true )
        {
            auto packet = interface_receiver.ReceivePacket();
            if ( !packet )
                break;

            assert( packet->GetAddress() == sender_address );

            switch ( packet->GetType() )
            {
                case PACKET_CONNECT:
                {
                    auto recv_connectPacket = static_cast<ConnectPacket*>( packet );
                    assert( *recv_connectPacket == connectPacketTemplate );
                    receivedConnectPacket = true;
                }
                break;

                case PACKET_UPDATE:
                {
                    auto recv_updatePacket = static_cast<UpdatePacket*>( packet );
                    assert( *recv_updatePacket == updatePacketTemplate );
                    receivedUpdatePacket = true;
                }
                break;

                case PACKET_DISCONNECT:
                {
                    auto recv_disconnectPacket = static_cast<DisconnectPacket*>( packet );
                    assert( *recv_disconnectPacket == disconnectPacketTemplate );
                    receivedDisconnectPacket = true;
                }
                break;
            }

            delete packet;
        }

        if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
            break;

        timeBase.time += timeBase.deltaTime;
    }
}

int main()
{
    srand( time( nullptr ) );

    if ( !InitializeSockets() )
    {
        printf( "failed to initialize sockets\n" );
        return 1;
    }

    test_bsd_sockets_send_and_receive_ipv4();
    test_bsd_sockets_send_and_receive_ipv6();
    test_bsd_sockets_send_and_receive_multiple_interfaces_ipv4();
    test_bsd_sockets_send_and_receive_multiple_interfaces_ipv6();

    ShutdownSockets();

    return 0;
}
