#include "BSDSocket.h"
#include "DNSResolver.h"
#include "ResolveWrapper.h"

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

void test_resolve_wrapper_send_to_hostname()
{
    printf( "test_resolve_wrapper_send_to_hostname\n" );

    BSDSocketConfig config;

    PacketFactory packetFactory;

    config.port = 10000;
    config.family = AF_INET6;
    config.maxPacketSize = 1024;
    config.packetFactory = &packetFactory;

    auto bsdSocket = new BSDSocket( config );

    DNSResolver resolver;

    ResolveWrapperConfig wrapperConfig;
    wrapperConfig.resolver = &resolver;
    wrapperConfig.networkInterface = bsdSocket;

    auto interface = new ResolveWrapper( wrapperConfig );

    int numPackets = 10;

    for ( int i = 0; i < numPackets; ++i )
    {
        auto packet = packetFactory.Create( PACKET_CONNECT );

        interface->SendPacket( "localhost", config.port, packet );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    std::chrono::milliseconds ms( (int) ( timeBase.deltaTime * 1000 ) );

    for ( int i = 0; i < 20; ++i )
    {
        interface->Update( timeBase );

        if ( bsdSocket->GetCounter( BSD_SOCKET_COUNTER_PACKETS_SENT ) == numPackets )
            break;

        std::this_thread::sleep_for( ms );

        timeBase.time += timeBase.deltaTime;
    }

    assert( bsdSocket->GetCounter( BSD_SOCKET_COUNTER_PACKETS_SENT ) == numPackets );
    assert( bsdSocket->GetCounter( BSD_SOCKET_COUNTER_PACKETS_RECEIVED ) == numPackets );

    delete bsdSocket;
    delete interface;
}

void test_resolve_wrapper_send_to_hostname_port()
{
    printf( "test_resolve_wrapper_send_to_hostname_port\n" );

    BSDSocketConfig config;

    PacketFactory packetFactory;

    config.port = 10000;
    config.family = AF_INET6;
    config.maxPacketSize = 1024;
    config.packetFactory = &packetFactory;

    auto bsdSocket = new BSDSocket( config );

    DNSResolver resolver;

    ResolveWrapperConfig wrapperConfig;
    wrapperConfig.resolver = &resolver;
    wrapperConfig.networkInterface = bsdSocket;

    auto interface = new ResolveWrapper( wrapperConfig );

    int numPackets = 10;

    for ( int i = 0; i < numPackets; ++i )
    {
        auto packet = packetFactory.Create( PACKET_CONNECT );

        interface->SendPacket( "localhost:10000", packet );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    std::chrono::milliseconds ms( (int) ( timeBase.deltaTime * 1000 ) );

    for ( int i = 0; i < 20; ++i )
    {
        interface->Update( timeBase );

        if ( bsdSocket->GetCounter( BSD_SOCKET_COUNTER_PACKETS_SENT ) == numPackets )
            break;

        std::this_thread::sleep_for( ms );

        timeBase.time += timeBase.deltaTime;
    }

    assert( bsdSocket->GetCounter( BSD_SOCKET_COUNTER_PACKETS_SENT ) == numPackets );
    assert( bsdSocket->GetCounter( BSD_SOCKET_COUNTER_PACKETS_RECEIVED ) == numPackets );

    delete interface;
    delete bsdSocket;
}

int main()
{
    srand( time( nullptr ) );

    if ( !InitializeSockets() )
    {
        printf( "failed to initialize sockets\n" );
        return 1;
    }

    test_resolve_wrapper_send_to_hostname();
    test_resolve_wrapper_send_to_hostname_port();

    ShutdownSockets();

    return 0;
}
