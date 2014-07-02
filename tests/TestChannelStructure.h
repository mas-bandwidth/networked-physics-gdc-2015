#ifndef TEST_CHANNEL_STRUCTURE_H
#define TEST_CHANNEL_STRUCTURE_H

#include "TestMessages.h"
#include "ReliableMessageChannel.h"

using namespace protocol;

class TestChannelStructure : public ChannelStructure
{
    ReliableMessageChannelConfig m_config;

public:

    TestChannelStructure( TestMessageFactory & messageFactory )
    {
        m_config.messageFactory = &messageFactory;
        m_config.messageAllocator = &memory::default_allocator();
        m_config.smallBlockAllocator = &memory::default_allocator();
        m_config.largeBlockAllocator = &memory::default_allocator();

        assert( m_config.messageAllocator );
        assert( m_config.smallBlockAllocator );
        assert( m_config.largeBlockAllocator );

        AddChannel( "reliable message channel", 
                    [this] { return CreateReliableMessageChannel(); }, 
                    [this] { return CreateReliableMessageChannelData(); } );

        Lock();
    }

    ReliableMessageChannel * CreateReliableMessageChannel()
    {
        // todo: convert to custom allocator
        return new ReliableMessageChannel( m_config );
    }

    ReliableMessageChannelData * CreateReliableMessageChannelData()
    {
        return PROTOCOL_NEW( memory::default_scratch_allocator(), ReliableMessageChannelData, m_config );
    }

    const ReliableMessageChannelConfig & GetConfig() const
    {
        return m_config;
    }
};

#endif
