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

#ifndef PROTOCOL_DATA_BLOCK_RECEIVER_H
#define PROTOCOL_DATA_BLOCK_RECEIVER_H

#include "core/Core.h"
#include "protocol/Block.h"

namespace core { class Allocator; }

namespace protocol
{
    class DataBlockReceiver
    {
    public:

        DataBlockReceiver( core::Allocator & allocator, int fragmentSize, int maxBlockSize );

        virtual ~DataBlockReceiver();

        void Clear();

        void ProcessFragment( int blockSize, int numFragments, int fragmentId, int fragmentBytes, uint8_t * fragmentData );

        Block * GetBlock();

        int GetNumFragments() const { return m_numFragments; }
        int GetNumReceivedFragments() const { return m_numReceivedFragments; }
        bool ReceiveCompleted() const { return m_numFragments != 0 && m_numReceivedFragments == m_numFragments; }

        bool IsError() const { return m_error != 0; }
        int GetError() const { return m_error; }

    protected:

        virtual void SendAck( int fragmentId ) = 0;

    private:

        core::Allocator * m_allocator;
        uint8_t * m_data;
        int m_fragmentSize;
        int m_maxBlockSize;
        int m_maxFragments;
        int m_blockSize;
        int m_numFragments;
        int m_numReceivedFragments;
        uint8_t * m_receivedFragment;
        int m_error;
        Block m_block;
    };
}

#endif
