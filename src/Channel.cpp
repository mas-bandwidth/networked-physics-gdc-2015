/*
    Network Protocol Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#include "Channel.h"
#include "Memory.h"
#include <string.h>

namespace protocol
{
    ChannelStructure::ChannelStructure( Allocator & channelAllocator, Allocator & channelDataAllocator )
    {
        m_channelAllocator = &channelAllocator;
        m_channelDataAllocator = &channelDataAllocator;
    }

    ChannelStructure::~ChannelStructure()
    {
        PROTOCOL_ASSERT( m_channelAllocator );
        PROTOCOL_ASSERT( m_channelDataAllocator );
        m_channelAllocator = nullptr;
        m_channelDataAllocator = nullptr;
    }

    void ChannelStructure::AddChannel( const char * name,
                                       CreateChannelFunction createChannel,
                                       CreateChannelDataFunction createChannelData )
    {
        PROTOCOL_ASSERT( !m_locked );

        if ( m_locked )
            return;

        PROTOCOL_ASSERT( m_numChannels < MaxChannels - 1 );

        ChannelEntry & entry = m_channelEntries[m_numChannels];

        strncpy( entry.name, name, MaxChannelName - 1 );
        entry.name[MaxChannelName-1] = '\0';
        entry.createChannel = createChannel;
        entry.createChannelData = createChannelData;

        m_numChannels++;
    }

    void ChannelStructure::Lock()
    {
        m_locked = true;
    }

    bool ChannelStructure::IsLocked() const
    {
        return m_locked;
    }

    int ChannelStructure::GetNumChannels() const
    {
        return m_numChannels;
    }

    const char * ChannelStructure::GetChannelName( int channelIndex ) const
    {
        PROTOCOL_ASSERT( channelIndex >= 0 );
        PROTOCOL_ASSERT( channelIndex < m_numChannels );
        return m_channelEntries[channelIndex].name;
    }

    Channel * ChannelStructure::CreateChannel( int channelIndex )
    {
        PROTOCOL_ASSERT( m_locked );
        PROTOCOL_ASSERT( channelIndex >= 0 );
        PROTOCOL_ASSERT( channelIndex < m_numChannels );
        return m_channelEntries[channelIndex].createChannel();
    }

    void ChannelStructure::DestroyChannel( Channel * channel )
    {
        PROTOCOL_ASSERT( channel );
        PROTOCOL_ASSERT( m_channelAllocator );
        PROTOCOL_DELETE( *m_channelAllocator, Channel, channel );
    }

    ChannelData * ChannelStructure::CreateChannelData( int channelIndex )
    {
        PROTOCOL_ASSERT( m_locked );
        PROTOCOL_ASSERT( channelIndex >= 0 );
        PROTOCOL_ASSERT( channelIndex < m_numChannels );
        return m_channelEntries[channelIndex].createChannelData();
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
