/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#include "Channel.h"
#include "Memory.h"

namespace protocol
{
    ChannelStructure::ChannelStructure( Allocator & channelAllocator, Allocator & channelDataAllocator, int numChannels )
    {
        PROTOCOL_ASSERT( numChannels > 0 );
        m_numChannels = numChannels;
        m_channelAllocator = &channelAllocator;
        m_channelDataAllocator = &channelDataAllocator;
    }

    ChannelStructure::~ChannelStructure()
    {
        PROTOCOL_ASSERT( m_numChannels > 0 );
        PROTOCOL_ASSERT( m_channelAllocator );
        PROTOCOL_ASSERT( m_channelDataAllocator );
        m_numChannels = 0;
        m_channelAllocator = nullptr;
        m_channelDataAllocator = nullptr;
    }

    const char * ChannelStructure::GetChannelName( int channelIndex ) const
    {
        PROTOCOL_ASSERT( channelIndex >= 0 );
        PROTOCOL_ASSERT( channelIndex < m_numChannels );
        return GetChannelNameInternal( channelIndex );
    }

    Channel * ChannelStructure::CreateChannel( int channelIndex )
    {
        PROTOCOL_ASSERT( channelIndex >= 0 );
        PROTOCOL_ASSERT( channelIndex < m_numChannels );
        return CreateChannelInternal( channelIndex );
    }

    ChannelData * ChannelStructure::CreateChannelData( int channelIndex )
    {
        PROTOCOL_ASSERT( channelIndex >= 0 );
        PROTOCOL_ASSERT( channelIndex < m_numChannels );
        return CreateChannelDataInternal( channelIndex );
    }

    void ChannelStructure::DestroyChannel( Channel * channel )
    {
        PROTOCOL_ASSERT( channel );
        PROTOCOL_ASSERT( m_channelAllocator );
        PROTOCOL_DELETE( *m_channelAllocator, Channel, channel );
    }

    void ChannelStructure::DestroyChannelData( ChannelData * channelData )
    {
        PROTOCOL_ASSERT( channelData );
        PROTOCOL_ASSERT( m_channelDataAllocator );
        PROTOCOL_DELETE( *m_channelDataAllocator, ChannelData, channelData );
    }

    Allocator & ChannelStructure::GetChannelAllocator()
    { 
        return *m_channelAllocator;
    }

    Allocator & ChannelStructure::GetChannelDataAllocator()
    { 
        return *m_channelDataAllocator;
    }
}
