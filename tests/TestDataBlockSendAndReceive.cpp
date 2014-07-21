#include "Common.h"
#include "DataBlockSender.h"
#include "DataBlockReceiver.h"
#include "Memory.h"
#include <stdio.h>

using namespace protocol;

static const int FragmentSize = 1024;
static const int FragmentsPerSecond = 60;
static const int MaxBlockSize = 256 * 1024;

static int packetLossPercent = 0;

class TestDataBlockSender : public DataBlockSender
{
    DataBlockReceiver * m_receiver = nullptr;

public:

    TestDataBlockSender( Block & dataBlock )
        : DataBlockSender( memory::default_allocator(), dataBlock, FragmentSize, FragmentsPerSecond ) {}

    void SetReceiver( DataBlockReceiver & receiver )
    {
        m_receiver = &receiver;
    }

protected:

    void SendFragment( int fragmentId, uint8_t * fragmentData, int fragmentBytes )
    {
        PROTOCOL_ASSERT( m_receiver );
        if ( (rand()%100) < packetLossPercent )
            return;
        m_receiver->ProcessFragment( GetBlockSize(), GetNumFragments(), fragmentId, fragmentBytes, fragmentData );
    }
};

class TestDataBlockReceiver : public DataBlockReceiver
{
    DataBlockSender * m_sender = nullptr;

public:

    TestDataBlockReceiver()
        : DataBlockReceiver( memory::default_allocator(), FragmentSize, MaxBlockSize ) {}

    void SetSender( DataBlockSender & sender )
    {
        m_sender = &sender;
    }

protected:

    void SendAck( int fragmentId )
    {
        PROTOCOL_ASSERT( m_sender );
        if ( (rand()%100) < packetLossPercent )
            return;
        m_sender->ProcessAck( fragmentId );
    }
};

void test_data_block_send_and_receive()
{
    printf( "test_data_block_send_and_receive\n" );

    memory::initialize();
    {
        const int BlockSize = 10 * 1024 + 55;

        Block sentBlock( memory::default_allocator(), BlockSize );
        {
            uint8_t * data = sentBlock.GetData();
            for ( int i = 0; i < BlockSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        TestDataBlockSender sender( sentBlock );
        TestDataBlockReceiver receiver;

        sender.SetReceiver( receiver );
        receiver.SetSender( sender );

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        for ( int i = 0; i < 100; ++i )
        {
            if ( sender.SendCompleted() )
                break;

            sender.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        PROTOCOL_CHECK( sender.SendCompleted() );
        PROTOCOL_CHECK( receiver.ReceiveCompleted() );
        PROTOCOL_CHECK( receiver.GetBlock() );

        auto block = receiver.GetBlock();
        const uint8_t * data = block->GetData();
        for ( int i = 0; i < BlockSize; ++i )
            PROTOCOL_CHECK( data[i] == ( 10 + i ) % 256 );
    }
    memory::shutdown();
}

void test_data_block_send_and_receive_packet_loss()
{
    printf( "test_data_block_send_and_receive_packet_loss\n" );

    packetLossPercent = 50;

    memory::initialize();
    {
        const int BlockSize = 10 * 1024 + 55;

        Block sentBlock( memory::default_allocator(), BlockSize );
        {
            uint8_t * data = sentBlock.GetData();
            for ( int i = 0; i < BlockSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        TestDataBlockSender sender( sentBlock );
        TestDataBlockReceiver receiver;

        sender.SetReceiver( receiver );
        receiver.SetSender( sender );

        TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        for ( int i = 0; i < 100; ++i )
        {
            if ( sender.SendCompleted() )
                break;

            sender.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        PROTOCOL_CHECK( sender.SendCompleted() );
        PROTOCOL_CHECK( receiver.ReceiveCompleted() );
        PROTOCOL_CHECK( receiver.GetBlock() );

        auto block = receiver.GetBlock();
        const uint8_t * data = block->GetData();
        for ( int i = 0; i < BlockSize; ++i )
            PROTOCOL_CHECK( data[i] == ( 10 + i ) % 256 );
    }
    memory::shutdown();
}
