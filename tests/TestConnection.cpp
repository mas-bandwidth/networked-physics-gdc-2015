#include "Connection.h"

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
        AddChannel( "test channel", [] { return new TestChannel(); }, [] { return nullptr; } );
        Lock();
    }
};

class PacketFactory : public Factory<Packet>
{
public:

    PacketFactory( ChannelStructure * channelStructure )
    {
        Register( PACKET_Connection, [channelStructure] { return new ConnectionPacket( PACKET_Connection, channelStructure ); } );
    }
};

void test_connection()
{
    printf( "test_connection\n" );

    TestChannelStructure channelStructure;

    PacketFactory packetFactory( &channelStructure );

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = 4 * 1024;
    connectionConfig.packetFactory = &packetFactory;
    connectionConfig.channelStructure = &channelStructure;

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
}

class AckChannel : public ChannelAdapter
{
public:

    std::vector<bool> & ackedPackets;

    AckChannel( std::vector<bool> & _ackedPackets )
        : ackedPackets( _ackedPackets ) {}

    bool ProcessData( uint16_t sequence, ChannelData * data )
    {
        return rand() % 10 != 0;
    }

    void ProcessAck( uint16_t ack )
    {
//        printf( "acked %d\n", (int) ack );
        ackedPackets[ack] = true;
    }
};

class AckChannelStructure : public ChannelStructure
{
public:
    AckChannelStructure( std::vector<bool> & ackedPackets )
    {
        AddChannel( "ack channel", [&ackedPackets] { return new AckChannel( ackedPackets ); }, [] { return nullptr; } );
        Lock();
    }
};

void test_acks()
{
    printf( "test_acks\n" );

    const int NumIterations = 10*1024;

    std::vector<bool> receivedPackets;
    std::vector<bool> ackedPackets;

    receivedPackets.resize( NumIterations );
    ackedPackets.resize( NumIterations );

    AckChannelStructure channelStructure( ackedPackets );

    PacketFactory packetFactory( &channelStructure );

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = 4 * 1024;
    connectionConfig.packetFactory = &packetFactory;
    connectionConfig.channelStructure = &channelStructure;

    Connection connection( connectionConfig );

    for ( int i = 0; i < NumIterations; ++i )
    {
        auto packet = connection.WritePacket();

        if ( rand() % 100 == 0 )
        {
            connection.ReadPacket( packet );

            if ( packet )
            {
                uint16_t sequence = packet->sequence;

    //            printf( "received %d\n", (int)sequence );

                receivedPackets[sequence] = true;

                delete packet;
            }
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

//    printf( "%d packets received, %d packets acked\n", numReceivedPackets, numAckedPackets );
}

int main()
{
    srand( time( nullptr ) );

    test_connection();
    test_acks();

    return 0;
}
