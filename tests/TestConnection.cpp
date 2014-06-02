#include "Connection.h"

using namespace std;
using namespace protocol;

enum { PACKET_Connection = 0 };

class TestChannel : public ChannelAdapter 
{
public:
    TestChannel() {}
};

class TestChannelStructure : public ChannelStructure
{
public:
    TestChannelStructure()
    {
        AddChannel( "test channel", [] { return make_shared<TestChannel>(); }, [] { return nullptr; } );
        Lock();
    }
};

class PacketFactory : public Factory<Packet>
{
public:

    PacketFactory( shared_ptr<ChannelStructure> channelStructure )
    {
        Register( PACKET_Connection, [channelStructure] { return make_shared<ConnectionPacket>( PACKET_Connection, channelStructure ); } );
    }
};

void test_connection()
{
    cout << "test_connection" << endl;

    auto channelStructure = make_shared<TestChannelStructure>();

    auto packetFactory = make_shared<PacketFactory>( channelStructure );

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = 4 * 1024;
    connectionConfig.packetFactory = packetFactory;
    connectionConfig.channelStructure = channelStructure;

    Connection connection( connectionConfig );

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

class AckChannel : public ChannelAdapter
{
public:
    vector<bool> & ackedPackets;

    AckChannel( vector<bool> & _ackedPackets )
        : ackedPackets( _ackedPackets ) {}

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
};

class AckChannelStructure : public ChannelStructure
{
public:
    AckChannelStructure( vector<bool> & ackedPackets )
    {
        AddChannel( "ack channel", [&ackedPackets] { return make_shared<AckChannel>( ackedPackets ); }, [] { return nullptr; } );
        Lock();
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

    auto channelStructure = make_shared<AckChannelStructure>( ackedPackets );

    auto packetFactory = make_shared<PacketFactory>( channelStructure );

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = 4 * 1024;
    connectionConfig.packetFactory = packetFactory;
    connectionConfig.channelStructure = channelStructure;

    Connection connection( connectionConfig );

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
