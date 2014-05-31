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

class AckChannel : public Channel
{
public:

    vector<bool> & ackedPackets;

    AckChannel( vector<bool> & _ackedPackets )
        : ackedPackets( _ackedPackets ) {}

    shared_ptr<ChannelData> CreateData()
    {
        return nullptr;
    }

    shared_ptr<ChannelData> GetData( uint16_t sequence )
    {
        return nullptr;
    }

    void ProcessData( uint16_t sequence, shared_ptr<ChannelData> data )
    {
        if ( rand() % 10 )
            throw runtime_error( "jinx!" );
    }

    void ProcessAck( uint16_t ack )
    {
//        cout << "acked " << (int)ack << endl;

        ackedPackets[ack] = true;
    }

    void Update( const TimeBase & timeBase )
    {
        // ...
    }
};


void test_acks()
{
    cout << "test_acks" << endl;

    const int NumIterations = 10*1024;

    vector<bool> receivedPackets;
    vector<bool> ackedPackets;

    receivedPackets.resize( NumIterations );
    ackedPackets.resize( NumIterations );

    auto packetFactory = make_shared<PacketFactory>();

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = 4 * 1024;
    connectionConfig.packetFactory = packetFactory;

    Connection connection( connectionConfig );

    connection.AddChannel( make_shared<AckChannel>( ackedPackets ) );

    packetFactory->SetInterface( connection.GetInterface() );

    for ( int i = 0; i < NumIterations; ++i )
    {
        auto packet = connection.WritePacket();

        if ( rand() % 100 == 0 )
        {
            connection.ReadPacket( packet );

            uint16_t sequence = packet->sequence;

//            cout << "received " << (int)sequence << endl;

            receivedPackets[sequence] = true;
        }
    }

    int numAckedPackets = 0;
    int numReceivedPackets = 0;
    for ( int i = 0; i < NumIterations; ++i )
    {
        if ( ackedPackets[i] )
            numAckedPackets++;

        if ( receivedPackets[i] )
            numReceivedPackets++;

        // an acked packet *must* have been received
        if ( ackedPackets[i] && !receivedPackets[i] )
            assert( false );
    }

    assert( numAckedPackets > 0 );
    assert( numReceivedPackets >= numAckedPackets );

//    cout << numReceivedPackets << " packets received, " << numAckedPackets << " packets acked." << endl;
}

int main()
{
    srand( time( NULL ) );

    try
    {
        test_connection();
        test_acks();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
