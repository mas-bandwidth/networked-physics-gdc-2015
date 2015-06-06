// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#include "protocol/Channel.h"
#include "core/Memory.h"

namespace protocol
{
    ChannelStructure::ChannelStructure( core::Allocator & channelAllocator, core::Allocator & channelDataAllocator, int numChannels )
    {
        CORE_ASSERT( numChannels > 0 );
        m_numChannels = numChannels;
        m_channelAllocator = &channelAllocator;
        m_channelDataAllocator = &channelDataAllocator;
    }

    ChannelStructure::~ChannelStructure()
    {
        CORE_ASSERT( m_numChannels > 0 );
        CORE_ASSERT( m_channelAllocator );
        CORE_ASSERT( m_channelDataAllocator );
        m_numChannels = 0;
        m_channelAllocator = nullptr;
        m_channelDataAllocator = nullptr;
    }

    const char * ChannelStructure::GetChannelName( int channelIndex ) const
    {
        CORE_ASSERT( channelIndex >= 0 );
        CORE_ASSERT( channelIndex < m_numChannels );
        return GetChannelNameInternal( channelIndex );
    }

    Channel * ChannelStructure::CreateChannel( int channelIndex )
    {
        CORE_ASSERT( channelIndex >= 0 );
        CORE_ASSERT( channelIndex < m_numChannels );
        return CreateChannelInternal( channelIndex );
    }

    ChannelData * ChannelStructure::CreateChannelData( int channelIndex )
    {
        CORE_ASSERT( channelIndex >= 0 );
        CORE_ASSERT( channelIndex < m_numChannels );
        return CreateChannelDataInternal( channelIndex );
    }

    void ChannelStructure::DestroyChannel( Channel * channel )
    {
        CORE_ASSERT( channel );
        CORE_ASSERT( m_channelAllocator );
        CORE_DELETE( *m_channelAllocator, Channel, channel );
    }

    void ChannelStructure::DestroyChannelData( ChannelData * channelData )
    {
        CORE_ASSERT( channelData );
        CORE_ASSERT( m_channelDataAllocator );
        CORE_DELETE( *m_channelDataAllocator, ChannelData, channelData );
    }

    core::Allocator & ChannelStructure::GetChannelAllocator()
    { 
        return *m_channelAllocator;
    }

    core::Allocator & ChannelStructure::GetChannelDataAllocator()
    { 
        return *m_channelDataAllocator;
    }
}
