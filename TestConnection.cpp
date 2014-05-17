#include "Connection.h"

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
        m_interface = interface;
    }

private:

    shared_ptr<ConnectionInterface> m_interface;
};

void test_connection()
{
    cout << "test_connection" << endl;

    auto packetFactory = make_shared<PacketFactory>();

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = 4 * 1024;
    connectionConfig.packetFactory = packetFactory;

    Connection connection( connectionConfig );

    packetFactory->SetInterface( connection.GetInterface() );

    const int NumAcks = 100;

    for ( int i = 0; i < NumAcks*2; ++i )
    {
        auto packet = connection.WritePacket();

        connection.ReadPacket( packet );

        if ( connection.GetCounter( Connection::PacketsAcked ) >= NumAcks )
            break;
    }

    assert( connection.GetCounter( Connection::PacketsAcked ) == NumAcks );
    assert( connection.GetCounter( Connection::PacketsWritten ) == NumAcks + 1 );
    assert( connection.GetCounter( Connection::PacketsRead ) == NumAcks + 1 );
    assert( connection.GetCounter( Connection::PacketsDiscarded ) == 0 );
    assert( connection.GetCounter( Connection::ReadPacketFailures ) == 0 );
}

int main()
{
    try
    {
        test_connection();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
