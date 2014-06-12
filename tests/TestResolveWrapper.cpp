#include "BSDSockets.h"
#include "DNSResolver.h"
#include "ResolveWrapper.h"

using namespace std;
using namespace protocol;

enum PacketType
{
    PACKET_Connect = 1,
    PACKET_Update,
    PACKET_Disconnect
};

struct ConnectPacket : public Packet
{
    int a,b,c;

    ConnectPacket() : Packet( PACKET_Connect )
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
        Register( PACKET_Connect,    [] { return new ConnectPacket();    } );
        Register( PACKET_Update,     [] { return new UpdatePacket();     } );
        Register( PACKET_Disconnect, [] { return new DisconnectPacket(); } );
    }
};

void test_resolve_wrapper_send_to_hostname()
{
    printf( "test_resolve_wrapper_send_to_hostname\n" );

    BSDSocketsConfig config;

    PacketFactory packetFactory;

    config.port = 10000;
    config.family = AF_INET6;
    config.maxPacketSize = 1024;
    config.packetFactory = &packetFactory;

    auto bsdSockets = new BSDSockets( config );

    DNSResolver resolver;

    ResolveWrapperConfig wrapperConfig;
    wrapperConfig.resolver = &resolver;
    wrapperConfig.networkInterface = bsdSockets;

    auto interface = new ResolveWrapper( wrapperConfig );

    int numPackets = 10;

    for ( int i = 0; i < numPackets; ++i )
    {
        auto packet = packetFactory.Create( PACKET_Connect );

        interface->SendPacket( "localhost", config.port, packet );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    chrono::milliseconds ms( (int) ( timeBase.deltaTime * 1000 ) );

    for ( int i = 0; i < 20; ++i )
    {
        interface->Update( timeBase );

        if ( bsdSockets->GetCounter( BSDSockets::PacketsSent ) == numPackets )
            break;

        this_thread::sleep_for( ms );

        timeBase.time += timeBase.deltaTime;
    }

    assert( bsdSockets->GetCounter( BSDSockets::PacketsSent ) == numPackets );
    assert( bsdSockets->GetCounter( BSDSockets::PacketsReceived ) == numPackets );

    delete bsdSockets;
    delete interface;
}

void test_resolve_wrapper_send_to_hostname_port()
{
    printf( "test_resolve_wrapper_send_to_hostname_port\n" );

    BSDSocketsConfig config;

    PacketFactory packetFactory;

    config.port = 10000;
    config.family = AF_INET6;
    config.maxPacketSize = 1024;
    config.packetFactory = &packetFactory;

    auto bsdSockets = new BSDSockets( config );

    DNSResolver resolver;

    ResolveWrapperConfig wrapperConfig;
    wrapperConfig.resolver = &resolver;
    wrapperConfig.networkInterface = bsdSockets;

    auto interface = new ResolveWrapper( wrapperConfig );

    int numPackets = 10;

    for ( int i = 0; i < numPackets; ++i )
    {
        auto packet = packetFactory->Create( PACKET_Connect );

        interface->SendPacket( "localhost:10000", packet );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    chrono::milliseconds ms( (int) ( timeBase.deltaTime * 1000 ) );

    for ( int i = 0; i < 20; ++i )
    {
        interface->Update( timeBase );

        if ( bsdSockets->GetCounter( BSDSockets::PacketsSent ) == numPackets )
            break;

        this_thread::sleep_for( ms );

        timeBase.time += timeBase.deltaTime;
    }

    assert( bsdSockets->GetCounter( BSDSockets::PacketsSent ) == numPackets );
    assert( bsdSockets->GetCounter( BSDSockets::PacketsReceived ) == numPackets );

    delete interface;
    delete bsdSockets;
}

int main()
{
    srand( time( NULL ) );

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
