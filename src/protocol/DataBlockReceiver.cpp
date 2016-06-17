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

#include "protocol/DataBlockReceiver.h"
#include "protocol/ProtocolConstants.h"
#include "protocol/ProtocolEnums.h"
#include "core/Allocator.h"

namespace protocol
{
    DataBlockReceiver::DataBlockReceiver( core::Allocator & allocator, int fragmentSize, int maxBlockSize )
    {
        CORE_ASSERT( fragmentSize > 0 );
        CORE_ASSERT( fragmentSize <= MaxFragmentSize );
        CORE_ASSERT( maxBlockSize > 0 );

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
        CORE_ASSERT( m_allocator );
        CORE_ASSERT( m_data );
        CORE_ASSERT( m_receivedFragment );

        m_block.Disconnect();

        m_allocator->Free( m_data );
        m_allocator->Free( m_receivedFragment );

        m_allocator = NULL;
        m_data = NULL;
        m_receivedFragment = NULL;
    }

    void DataBlockReceiver::Clear()
    {
        m_blockSize = 0;
        m_numFragments = 0;
        m_numReceivedFragments = 0;
        m_error = 0;
        m_block.Disconnect();
        memset( m_receivedFragment, 0, m_maxFragments );
    }

    void DataBlockReceiver::ProcessFragment( int blockSize, int numFragments, int fragmentId, int fragmentBytes, uint8_t * fragmentData )
    {
        if ( blockSize > m_maxBlockSize )
        {
            m_error = DATA_BLOCK_RECEIVER_ERROR_BLOCK_TOO_LARGE;
            return;
        }

        if ( m_error != 0 )
            return;

        // 1. intensive validation!

        if ( m_blockSize == 0 )
            m_blockSize = blockSize;

        if ( m_blockSize != blockSize )
            return;

        if ( m_numFragments > m_maxFragments )
            return;

        if ( m_numFragments == 0 )
            m_numFragments = numFragments;

        if ( m_numFragments != numFragments )
            return;

        if ( fragmentId >= m_numFragments )
            return;

        const int start = fragmentId * m_fragmentSize;
        const int finish = start + fragmentBytes;

        CORE_ASSERT( finish <= m_blockSize );
        if ( finish > m_blockSize )
            return;

        // 2. send an ack

        SendAck( fragmentId );

        // 3. process the fragment

        if ( !m_receivedFragment[fragmentId] )
        {
            m_receivedFragment[fragmentId] = 1;
            m_numReceivedFragments++;

            CORE_ASSERT( m_numReceivedFragments >= 0 );
            CORE_ASSERT( m_numReceivedFragments <= m_numFragments );

            memcpy( m_data + start, fragmentData, fragmentBytes );
        }
    }

    Block * DataBlockReceiver::GetBlock()
    {
        if ( ReceiveCompleted() && m_blockSize > 0 )
        {
            m_block.Disconnect();
            m_block.Connect( *m_allocator, m_data, m_blockSize );
            return &m_block;
        }
        else
            return NULL;
    }
}

