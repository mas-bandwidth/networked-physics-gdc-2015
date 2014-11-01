#ifndef TEST_CHANNEL_STRUCTURE_H
#define TEST_CHANNEL_STRUCTURE_H

#include "protocol/ReliableMessageChannel.h"
#include "core/Memory.h"
#include "TestMessages.h"

class TestChannelStructure : public protocol::ChannelStructure
{
    protocol::ReliableMessageChannelConfig m_config;

public:

    TestChannelStructure( TestMessageFactory & messageFactory )
        : ChannelStructure( core::memory::default_allocator(), core::memory::scratch_allocator(), 1 )
    {
        m_config.messageFactory = &messageFactory;
        m_config.messageAllocator = &core::memory::default_allocator();
        m_config.smallBlockAllocator = &core::memory::default_allocator();
        m_config.largeBlockAllocator = &core::memory::default_allocator();

        CORE_ASSERT( m_config.messageAllocator );
        CORE_ASSERT( m_config.smallBlockAllocator );
        CORE_ASSERT( m_config.largeBlockAllocator );
    }

    const protocol::ReliableMessageChannelConfig & GetConfig() const
    {
        return m_config;
    }

protected:

    const char * GetChannelNameInternal( int channelIndex ) const
    {
        return "reliable message channel";
    }
    
    protocol::Channel * CreateChannelInternal( int channelIndex )
    {
        return CORE_NEW( GetChannelAllocator(), protocol::ReliableMessageChannel, m_config );
    }

    protocol::ChannelData * CreateChannelDataInternal( int channelIndex )
    {
        return CORE_NEW( GetChannelDataAllocator(), protocol::ReliableMessageChannelData, m_config );
    }
};

#endif
