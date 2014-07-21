/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#include "DataBlockReceiver.h"
#include "Allocator.h"

namespace protocol
{
    DataBlockReceiver::DataBlockReceiver( Allocator & allocator, int fragmentSize, int maxBlockSize )
    {
        PROTOCOL_ASSERT( fragmentSize > 0 );
        PROTOCOL_ASSERT( fragmentSize <= MaxFragmentSize );
        PROTOCOL_ASSERT( maxBlockSize > 0 );

        m_allocator = &allocator;
        m_data = (uint8_t*) allocator.Allocate( maxBlockSize );
        m_maxBlockSize = maxBlockSize;
        m_fragmentSize = fragmentSize;
        m_maxFragments = m_maxBlockSize / m_fragmentSize + ( ( m_maxBlockSize % m_fragmentSize ) ? 1 : 0 );
        m_receivedFragment = (uint8_t*) allocator.Allocate( m_maxFragments );

        Clear();
    }

    DataBlockReceiver::~DataBlockReceiver()
    {
        PROTOCOL_ASSERT( m_allocator );
        PROTOCOL_ASSERT( m_data );
        PROTOCOL_ASSERT( m_receivedFragment );

        m_allocator->Free( m_data );
        m_allocator->Free( m_receivedFragment );

        m_allocator = nullptr;
        m_data = nullptr;
        m_receivedFragment = nullptr;
    }

    void DataBlockReceiver::Clear()
    {
        m_blockSize = 0;
        m_numFragments = 0;
        m_numReceivedFragments = 0;
        m_error = 0;
        memset( m_receivedFragment, 0, m_maxFragments );
    }

    void DataBlockReceiver::ReceiveFragment( int blockSize, int numFragments, int fragmentId, int fragmentBytes, uint8_t * fragmentData )
    {
        // todo
    }

    Block * DataBlockReceiver::GetBlock()
    {
        // todo
        return nullptr;
    }
}

