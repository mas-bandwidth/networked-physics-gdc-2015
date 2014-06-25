/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#include "Channel.h"

namespace protocol
{
    void ChannelStructure::AddChannel( const char * name,
                                       CreateChannelFunction createChannel,
                                       CreateChannelDataFunction createChannelData )
    {
        assert( !m_locked );
        if ( m_locked )
            return;

        assert( m_numChannels < MaxChannels - 1 );

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
        assert( channelIndex >= 0 );
        assert( channelIndex < m_numChannels );
        return m_channelEntries[channelIndex].name;
    }

    Channel * ChannelStructure::CreateChannel( int channelIndex )
    {
        assert( m_locked );
        assert( channelIndex >= 0 );
        assert( channelIndex < m_numChannels );
        return m_channelEntries[channelIndex].createChannel();
    }

    ChannelData * ChannelStructure::CreateChannelData( int channelIndex )
    {
        assert( m_locked );
        assert( channelIndex >= 0 );
        assert( channelIndex < m_numChannels );
        return m_channelEntries[channelIndex].createChannelData();
    }
}
