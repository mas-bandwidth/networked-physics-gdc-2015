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

#ifndef PROTOCOL_DATA_BLOCK_SENDER_H
#define PROTOCOL_DATA_BLOCK_SENDER_H

#include "core/Core.h"

namespace core { class Allocator; }

namespace protocol
{
    class Block;

    class DataBlockSender
    {
    public:

        DataBlockSender( core::Allocator & allocator, Block & dataBlock, int fragmentSize, int fragmentsPerSecond );

        virtual ~DataBlockSender();

        void Clear();

        void Update( const core::TimeBase & timeBase );

        void ProcessAck( int fragmentId );

        int GetBlockSize() const;
        int GetFragmentSize() const { return m_fragmentSize; }
        int GetNumFragments() const { return m_numFragments; }
        int GetNumAckedFragments() const { return m_numAckedFragments; }
        bool SendCompleted() const { return m_numAckedFragments == m_numFragments; }

    protected:

        virtual void SendFragment( int fragmentId, uint8_t * fragmentData, int fragmentBytes ) = 0;

    private:

        core::Allocator * m_allocator;
        Block * m_dataBlock;
        int m_fragmentSize;
        float m_timeBetweenFragments;
        int m_fragmentIndex;
        int m_numFragments;
        int m_numAckedFragments;
        double m_lastFragmentSendTime;
        uint8_t * m_ackedFragment;        
    };
}

#endif
