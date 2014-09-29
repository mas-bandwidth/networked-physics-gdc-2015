/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#include "DataBlockSender.h"
#include "Allocator.h"
#include "Block.h"

namespace protocol
{
    DataBlockSender::DataBlockSender( Allocator & allocator, Block & dataBlock, int fragmentSize, int fragmentsPerSecond )
    {
        PROTOCOL_ASSERT( dataBlock.GetSize() > 0 );
        PROTOCOL_ASSERT( dataBlock.GetData() );
        PROTOCOL_ASSERT( fragmentSize > 0 );
        PROTOCOL_ASSERT( fragmentSize <= MaxFragmentSize );
        PROTOCOL_ASSERT( fragmentsPerSecond > 0 );

        m_allocator = &allocator;
        m_dataBlock = &dataBlock;
        m_fragmentSize = fragmentSize;
        m_timeBetweenFragments = 1.0f / fragmentsPerSecond;
        m_numFragments = dataBlock.GetSize() / m_fragmentSize + ( ( dataBlock.GetSize() % m_fragmentSize ) ? 1 : 0 );
        m_ackedFragment = (uint8_t*) m_allocator->Allocate( m_numFragments );

        Clear();
    }

    DataBlockSender::~DataBlockSender()
    {
        PROTOCOL_ASSERT( m_allocator );
        PROTOCOL_ASSERT( m_ackedFragment );

        m_allocator->Free( m_ackedFragment );
        m_ackedFragment = nullptr;
        m_allocator = nullptr;
    }

    void DataBlockSender::Clear()
    {
        PROTOCOL_ASSERT( m_ackedFragment );

        m_fragmentIndex = 0;
        m_numAckedFragments = 0;
        m_lastFragmentSendTime = 0.0;
        memset( m_ackedFragment, 0, m_numFragments );
    }

    void DataBlockSender::Update( const TimeBase & timeBase )
    {
        if ( m_lastFragmentSendTime + m_timeBetweenFragments >= timeBase.time )
            return;

        m_lastFragmentSendTime = timeBase.time;

        PROTOCOL_ASSERT( m_numAckedFragments < m_numFragments );

        for ( int i = 0; i < m_numFragments; ++i )
        {
            if ( !m_ackedFragment[m_fragmentIndex] )
                break;
            m_fragmentIndex++;
            m_fragmentIndex %= m_numFragments;
        }

        PROTOCOL_ASSERT( m_fragmentIndex >= 0 );
        PROTOCOL_ASSERT( m_fragmentIndex < m_numFragments );
        PROTOCOL_ASSERT( !m_ackedFragment[m_fragmentIndex] );

        int fragmentBytes = m_fragmentSize;
        if ( m_fragmentIndex == m_numFragments - 1 )
            fragmentBytes = m_dataBlock->GetSize() - ( m_numFragments - 1 ) * m_fragmentSize;

        PROTOCOL_ASSERT( fragmentBytes > 0 );
        PROTOCOL_ASSERT( fragmentBytes <= MaxFragmentSize );

        SendFragment( m_fragmentIndex, m_dataBlock->GetData() + m_fragmentIndex * m_fragmentSize, fragmentBytes );

        m_fragmentIndex = ( m_fragmentIndex + 1 ) % m_numFragments;
    }

    void DataBlockSender::ProcessAck( int fragmentId )
    {
        if ( fragmentId > m_numFragments )
            return;

        PROTOCOL_ASSERT( fragmentId >= 0 );
        PROTOCOL_ASSERT( fragmentId <= m_numFragments );

        if ( !m_ackedFragment[fragmentId] )
        {
            m_ackedFragment[fragmentId] = 1;
            m_numAckedFragments++;
            PROTOCOL_ASSERT( m_numAckedFragments >= 0 );
            PROTOCOL_ASSERT( m_numAckedFragments <= m_numFragments );
        }
    }

    int DataBlockSender::GetBlockSize() const
    {
        PROTOCOL_ASSERT( m_dataBlock );
        return m_dataBlock->GetSize();
    }
}
