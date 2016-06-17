/*
    Networked Physics Demo

    Copyright © 2008 - 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "protocol/DataBlockSender.h"
#include "protocol/ProtocolConstants.h"
#include "protocol/Block.h"
#include "core/Allocator.h"

namespace protocol
{
    DataBlockSender::DataBlockSender( core::Allocator & allocator, Block & dataBlock, int fragmentSize, int fragmentsPerSecond )
    {
        CORE_ASSERT( dataBlock.GetSize() > 0 );
        CORE_ASSERT( dataBlock.GetData() );
        CORE_ASSERT( fragmentSize > 0 );
        CORE_ASSERT( fragmentSize <= MaxFragmentSize );
        CORE_ASSERT( fragmentsPerSecond > 0 );

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
        CORE_ASSERT( m_allocator );
        CORE_ASSERT( m_ackedFragment );

        m_allocator->Free( m_ackedFragment );
        m_ackedFragment = NULL;
        m_allocator = NULL;
    }

    void DataBlockSender::Clear()
    {
        CORE_ASSERT( m_ackedFragment );

        m_fragmentIndex = 0;
        m_numAckedFragments = 0;
        m_lastFragmentSendTime = 0.0;
        memset( m_ackedFragment, 0, m_numFragments );
    }

    void DataBlockSender::Update( const core::TimeBase & timeBase )
    {
        if ( m_lastFragmentSendTime + m_timeBetweenFragments >= timeBase.time )
            return;

        m_lastFragmentSendTime = timeBase.time;

        CORE_ASSERT( m_numAckedFragments < m_numFragments );

        for ( int i = 0; i < m_numFragments; ++i )
        {
            if ( !m_ackedFragment[m_fragmentIndex] )
                break;
            m_fragmentIndex++;
            m_fragmentIndex %= m_numFragments;
        }

        CORE_ASSERT( m_fragmentIndex >= 0 );
        CORE_ASSERT( m_fragmentIndex < m_numFragments );
        CORE_ASSERT( !m_ackedFragment[m_fragmentIndex] );

        int fragmentBytes = m_fragmentSize;
        if ( m_fragmentIndex == m_numFragments - 1 )
            fragmentBytes = m_dataBlock->GetSize() - ( m_numFragments - 1 ) * m_fragmentSize;

        CORE_ASSERT( fragmentBytes > 0 );
        CORE_ASSERT( fragmentBytes <= MaxFragmentSize );

        SendFragment( m_fragmentIndex, m_dataBlock->GetData() + m_fragmentIndex * m_fragmentSize, fragmentBytes );

        m_fragmentIndex = ( m_fragmentIndex + 1 ) % m_numFragments;
    }

    void DataBlockSender::ProcessAck( int fragmentId )
    {
        if ( fragmentId > m_numFragments )
            return;

        CORE_ASSERT( fragmentId >= 0 );
        CORE_ASSERT( fragmentId <= m_numFragments );

        if ( !m_ackedFragment[fragmentId] )
        {
            m_ackedFragment[fragmentId] = 1;
            m_numAckedFragments++;
            CORE_ASSERT( m_numAckedFragments >= 0 );
            CORE_ASSERT( m_numAckedFragments <= m_numFragments );
        }
    }

    int DataBlockSender::GetBlockSize() const
    {
        CORE_ASSERT( m_dataBlock );
        return m_dataBlock->GetSize();
    }
}
