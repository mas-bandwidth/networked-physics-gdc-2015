/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_DATA_BLOCK_RECEIVER_H
#define PROTOCOL_DATA_BLOCK_RECEIVER_H

#include "Common.h"
#include "Block.h"

namespace protocol
{
    class Allocator;

    class DataBlockReceiver
    {
    public:

        DataBlockReceiver( Allocator & allocator, int fragmentSize, int maxBlockSize );

        ~DataBlockReceiver();

        void Clear();

        void ReceiveFragment( int blockSize, int numFragments, int fragmentId, int fragmentBytes, uint8_t * fragmentData );

        Block * GetBlock();

        int GetNumFragments() const { return m_numFragments; }
        int GetNumReceivedFragments() const { return m_numReceivedFragments; }
        bool ReceiveCompleted() const { return m_numReceivedFragments == m_numFragments; }

        bool IsError() const { return m_error != 0; }
        int GetError() const { return m_error; }

    protected:

        virtual void SendAck( int fragmentId ) = 0;

    private:

        Allocator * m_allocator;
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
