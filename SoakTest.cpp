#include "Connection.h"
#include "BSDSocketsInterface.h"

using namespace std;
using namespace protocol;

enum { PACKET_Connection = 0 };

class PacketFactory : public Factory<Packet>
{
public:
    PacketFactory()
    {
        Register( PACKET_Connection, [this] { return make_shared<ConnectionPacket>( PACKET_Connection, m_interface ); } );
    }

    void SetInterface( shared_ptr<ConnectionInterface> interface )
    {
        assert( interface );
        m_interface = interface;
    }

private:

    shared_ptr<ConnectionInterface> m_interface;
};

void soak_test()
{
    cout << "[soak test]" << endl;

    auto packetFactory = make_shared<PacketFactory>();

    BSDSocketsInterfaceConfig interfaceConfig;
    interfaceConfig.port = 10000;
    interfaceConfig.family = AF_INET6;
    interfaceConfig.maxPacketSize = 4096;
    interfaceConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );
    
    BSDSocketsInterface interface( interfaceConfig );

    Address address( "::1" );
    address.SetPort( interfaceConfig.port );

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = 4 * 1024;
    connectionConfig.packetFactory = packetFactory;

    Connection connection( connectionConfig );

    packetFactory->SetInterface( connection.GetInterface() );

    double dt = 1.0 / 100;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    int iterations = 0;

    while ( ++iterations < 200 )
    {
        auto packet = connection.WritePacket();

        // todo: actually send messages and so on

        interface.SendPacket( address, packet );

        cout << "sent packet to address " << address.ToString() << endl;

        interface.Update();

        this_thread::sleep_for( ms );

        while ( true )
        {
            auto packet = interface.ReceivePacket();
            if ( !packet )
                break;

            if ( rand() % 10 )
            {
                cout << "random drop packet" << endl;
                continue;
            }

            assert( packet->GetAddress() == address );
            assert( packet->GetType() == PACKET_Connection );

            cout << "received packet from address " << packet->GetAddress().ToString() << endl;

            auto connectionPacket = static_pointer_cast<ConnectionPacket>( packet );
            connection.ReadPacket( connectionPacket );
        }

        // todo: receive messages and print out the ones received
    }
}

int main()
{
    if ( !InitializeSockets() )
    {
        cerr << "failed to initialize sockets" << endl;
        return 1;
    }

    try
    {
        soak_test();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    ShutdownSockets();

    return 0;
}
