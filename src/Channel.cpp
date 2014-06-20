/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#include "Channel.h"

namespace protocol
{
    void ChannelStructure::AddChannel( const std::string & name,
                                       CreateChannelFunction createChannel,
                                       CreateChannelDataFunction createChannelData )
    {
        assert( !m_locked );
        if ( m_locked )
            return;

        assert( m_numChannels < MaxChannels - 1 );

        ChannelEntry & entry = m_channelEntries[m_numChannels];

        entry.name = name;
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

    int ChannelStructure::GetNumChannels()
    {
        return m_numChannels;
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
