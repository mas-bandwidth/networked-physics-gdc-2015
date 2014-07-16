#ifndef TEST_CHANNEL_STRUCTURE_H
#define TEST_CHANNEL_STRUCTURE_H

#include "TestMessages.h"
#include "ReliableMessageChannel.h"
#include "Memory.h"

using namespace protocol;

class TestChannelStructure : public ChannelStructure
{
    ReliableMessageChannelConfig m_config;

public:

    TestChannelStructure( TestMessageFactory & messageFactory )
        : ChannelStructure( memory::default_allocator(), memory::scratch_allocator(), 1 )
    {
        m_config.messageFactory = &messageFactory;
        m_config.messageAllocator = &memory::default_allocator();
        m_config.smallBlockAllocator = &memory::default_allocator();
        m_config.largeBlockAllocator = &memory::default_allocator();

        PROTOCOL_ASSERT( m_config.messageAllocator );
        PROTOCOL_ASSERT( m_config.smallBlockAllocator );
        PROTOCOL_ASSERT( m_config.largeBlockAllocator );
    }

    const ReliableMessageChannelConfig & GetConfig() const
    {
        return m_config;
    }

protected:

    const char * GetChannelNameInternal( int channelIndex ) const
    {
        return "reliable message channel";
    }
    
    Channel * CreateChannelInternal( int channelIndex )
    {
        return PROTOCOL_NEW( GetChannelAllocator(), ReliableMessageChannel, m_config );
    }

    ChannelData * CreateChannelDataInternal( int channelIndex )
    {
        return PROTOCOL_NEW( GetChannelDataAllocator(), ReliableMessageChannelData, m_config );
    }
};

#endif
