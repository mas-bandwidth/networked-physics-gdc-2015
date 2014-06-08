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
        serialize_bits( stream, timestamp, 16 );
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

void test_resolve_wrapper_send_to_hostname()
{
    cout << "test_resolve_wrapper_send_to_hostname" << endl;

    BSDSocketsConfig config;

    auto packetFactory = make_shared<PacketFactory>();

    config.port = 10000;
    config.family = AF_INET6;
    config.maxPacketSize = 1024;
    config.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto bsdSockets = make_shared<BSDSockets>( config );

    ResolveWrapperConfig wrapperConfig;
    wrapperConfig.resolver = make_shared<DNSResolver>();
    wrapperConfig.networkInterface = bsdSockets;

    auto interface = make_shared<ResolveWrapper>( wrapperConfig );

    int numPackets = 10;

    for ( int i = 0; i < numPackets; ++i )
    {
        auto packet = packetFactory->Create( PACKET_Connect );

        interface->SendPacket( "localhost", config.port, packet );
    }

    auto start = chrono::steady_clock::now();

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

    auto finish = chrono::steady_clock::now();

    auto delta = finish - start;

    cout << chrono::duration<double,milli>( delta ).count() << " ms" << endl;
}

void test_resolve_wrapper_send_to_hostname_port()
{
    cout << "test_resolve_wrapper_send_to_hostname_port" << endl;

    BSDSocketsConfig config;

    auto packetFactory = make_shared<PacketFactory>();

    config.port = 10000;
    config.family = AF_INET6;
    config.maxPacketSize = 1024;
    config.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );

    auto bsdSockets = make_shared<BSDSockets>( config );

    ResolveWrapperConfig wrapperConfig;
    wrapperConfig.resolver = make_shared<DNSResolver>();
    wrapperConfig.networkInterface = bsdSockets;

    auto interface = make_shared<ResolveWrapper>( wrapperConfig );

    int numPackets = 10;

    for ( int i = 0; i < numPackets; ++i )
    {
        auto packet = packetFactory->Create( PACKET_Connect );

        interface->SendPacket( "localhost:10000", packet );
    }

    auto start = chrono::steady_clock::now();

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

    auto finish = chrono::steady_clock::now();

    auto delta = finish - start;

    cout << chrono::duration<double,milli>( delta ).count() << " ms" << endl;
}

int main()
{
    srand( time( NULL ) );

    if ( !InitializeSockets() )
    {
        cerr << "failed to initialize sockets" << endl;
        return 1;
    }

    try
    {
        test_resolve_wrapper_send_to_hostname();
        test_resolve_wrapper_send_to_hostname_port();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    ShutdownSockets();

    return 0;
}
