#include "core/Core.h"
#include "core/Memory.h"
#include "protocol/DataBlockSender.h"
#include "protocol/DataBlockReceiver.h"
#include <stdio.h>

static const int FragmentSize = 1024;
static const int FragmentsPerSecond = 60;
static const int MaxBlockSize = 256 * 1024;

static int packetLossPercent = 0;

class TestDataBlockSender : public protocol::DataBlockSender
{
    protocol::DataBlockReceiver * m_receiver = nullptr;

public:

    TestDataBlockSender( protocol::Block & dataBlock )
        : DataBlockSender( core::memory::default_allocator(), dataBlock, FragmentSize, FragmentsPerSecond ) {}

    void SetReceiver( protocol::DataBlockReceiver & receiver )
    {
        m_receiver = &receiver;
    }

protected:

    void SendFragment( int fragmentId, uint8_t * fragmentData, int fragmentBytes )
    {
        CORE_ASSERT( m_receiver );
        if ( (rand()%100) < packetLossPercent )
            return;
        m_receiver->ProcessFragment( GetBlockSize(), GetNumFragments(), fragmentId, fragmentBytes, fragmentData );
    }
};

class TestDataBlockReceiver : public protocol::DataBlockReceiver
{
    protocol::DataBlockSender * m_sender = nullptr;

public:

    TestDataBlockReceiver()
        : DataBlockReceiver( core::memory::default_allocator(), FragmentSize, MaxBlockSize ) {}

    void SetSender( protocol::DataBlockSender & sender )
    {
        m_sender = &sender;
    }

protected:

    void SendAck( int fragmentId )
    {
        CORE_ASSERT( m_sender );
        if ( (rand()%100) < packetLossPercent )
            return;
        m_sender->ProcessAck( fragmentId );
    }
};

void test_data_block_send_and_receive()
{
    printf( "test_data_block_send_and_receive\n" );

    core::memory::initialize();
    {
        const int BlockSize = 10 * 1024 + 55;

        protocol::Block sentBlock( core::memory::default_allocator(), BlockSize );
        {
            uint8_t * data = sentBlock.GetData();
            for ( int i = 0; i < BlockSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        TestDataBlockSender sender( sentBlock );
        TestDataBlockReceiver receiver;

        sender.SetReceiver( receiver );
        receiver.SetSender( sender );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        for ( int i = 0; i < 100; ++i )
        {
            if ( sender.SendCompleted() )
                break;

            sender.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        CORE_CHECK( sender.SendCompleted() );
        CORE_CHECK( receiver.ReceiveCompleted() );
        CORE_CHECK( receiver.GetBlock() );

        auto block = receiver.GetBlock();
        const uint8_t * data = block->GetData();
        for ( int i = 0; i < BlockSize; ++i )
            CORE_CHECK( data[i] == ( 10 + i ) % 256 );
    }
    core::memory::shutdown();
}

void test_data_block_send_and_receive_packet_loss()
{
    printf( "test_data_block_send_and_receive_packet_loss\n" );

    packetLossPercent = 50;

    core::memory::initialize();
    {
        const int BlockSize = 10 * 1024 + 55;

        protocol::Block sentBlock( core::memory::default_allocator(), BlockSize );
        {
            uint8_t * data = sentBlock.GetData();
            for ( int i = 0; i < BlockSize; ++i )
                data[i] = ( 10 + i ) % 256;
        }

        TestDataBlockSender sender( sentBlock );
        TestDataBlockReceiver receiver;

        sender.SetReceiver( receiver );
        receiver.SetSender( sender );

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.1f;

        for ( int i = 0; i < 100; ++i )
        {
            if ( sender.SendCompleted() )
                break;

            sender.Update( timeBase );

            timeBase.time += timeBase.deltaTime;
        }

        CORE_CHECK( sender.SendCompleted() );
        CORE_CHECK( receiver.ReceiveCompleted() );
        CORE_CHECK( receiver.GetBlock() );

        auto block = receiver.GetBlock();
        const uint8_t * data = block->GetData();
        for ( int i = 0; i < BlockSize; ++i )
            CORE_CHECK( data[i] == ( 10 + i ) % 256 );
    }
    core::memory::shutdown();
}
