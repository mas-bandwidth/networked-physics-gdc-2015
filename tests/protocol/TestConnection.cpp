#include "protocol/Connection.h"
#include "core/Memory.h"
#include "TestPackets.h"

class FakeChannel : public protocol::ChannelAdapter 
{
public:
    FakeChannel() {}
};

class FakeChannelStructure : public protocol::ChannelStructure
{
public:

    FakeChannelStructure() : ChannelStructure( core::memory::default_allocator(), core::memory::scratch_allocator(), 1 ) {}

protected:

    const char * GetChannelNameInternal( int channelIndex ) const
    {
        return "fake channel";
    }

    protocol::Channel * CreateChannelInternal( int channeIndex )
    {
        return CORE_NEW( GetChannelAllocator(), FakeChannel );
    }

    protocol::ChannelData * CreateChannelDataInternal( int channeIndex )
    {
        return nullptr;
    }
};

void test_connection()
{
    printf( "test_connection\n" );

    core::memory::initialize();
    {
        FakeChannelStructure channelStructure;

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        protocol::ConnectionConfig connectionConfig;
        connectionConfig.maxPacketSize = 4 * 1024;
        connectionConfig.packetFactory = &packetFactory;
        connectionConfig.channelStructure = &channelStructure;

        protocol::Connection connection( connectionConfig );

        const int NumAcks = 100;

        for ( int i = 0; i < NumAcks*2; ++i )
        {
            auto packet = connection.WritePacket();

            CORE_CHECK( packet );

            connection.ReadPacket( packet );
            packetFactory.Destroy( packet );
            packet = nullptr;

            if ( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_ACKED ) >= NumAcks )
                break;
        }

        CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_ACKED ) == NumAcks );
        CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_WRITTEN ) == NumAcks + 1 );
        CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_READ ) == NumAcks + 1 );
        CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_DISCARDED ) == 0 );
    }
    core::memory::shutdown();
}

class AckChannel : public protocol::ChannelAdapter
{
public:

    int * ackedPackets = nullptr;

    AckChannel( int * _ackedPackets )
        : ackedPackets( _ackedPackets ) {}

    bool ProcessData( uint16_t sequence, protocol::ChannelData * data )
    {
        return rand() % 10 != 0;
    }

    void ProcessAck( uint16_t ack )
    {
//        printf( "acked %d\n", (int) ack );
        ackedPackets[ack] = true;
    }
};

class AckChannelStructure : public protocol::ChannelStructure
{
    int * ackedPackets = nullptr;

public:

    AckChannelStructure( int * _ackedPackets )
        : ChannelStructure( core::memory::default_allocator(), core::memory::scratch_allocator(), 1 )
    {
        ackedPackets = _ackedPackets;
    }

protected:

    const char * GetChannelNameInternal( int channelIndex ) const
    {
        return "ack channel";
    }

    protocol::Channel * CreateChannelInternal( int channelIndex )
    {
        return CORE_NEW( GetChannelAllocator(), AckChannel, ackedPackets );
    }

    protocol::ChannelData * CreateChannelDataInternal( int channelIndex )
    {
        return nullptr;
    }
};

void test_acks()
{
    printf( "test_acks\n" );

    core::memory::initialize();
    {
        const int NumIterations = 10*1024;

        int receivedPackets[NumIterations];
        int ackedPackets[NumIterations];

        memset( receivedPackets, 0, sizeof( receivedPackets ) );
        memset( ackedPackets, 0, sizeof( ackedPackets ) );

        AckChannelStructure channelStructure( ackedPackets );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        protocol::ConnectionConfig connectionConfig;
        connectionConfig.packetFactory = &packetFactory;
        connectionConfig.channelStructure = &channelStructure;

        protocol::Connection connection( connectionConfig );

        for ( int i = 0; i < NumIterations; ++i )
        {
            auto packet = connection.WritePacket();

            CORE_CHECK( packet );

            if ( rand() % 100 == 0 )
            {
                connection.ReadPacket( packet );

                if ( packet )
                {
                    uint16_t sequence = packet->sequence;

        //            printf( "received %d\n", (int)sequence );

                    receivedPackets[sequence] = true;
                }
            }

            packetFactory.Destroy( packet );
            packet = nullptr;
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
                CORE_CHECK( false );
        }

        CORE_CHECK( numAckedPackets > 0 );
        CORE_CHECK( numReceivedPackets >= numAckedPackets );

    //    printf( "%d packets received, %d packets acked\n", numReceivedPackets, numAckedPackets );
    }
    core::memory::shutdown();
}
