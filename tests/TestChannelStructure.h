#ifndef TEST_CHANNEL_STRUCTURE_H
#define TEST_CHANNEL_STRUCTURE_H

#include "TestMessages.h"
#include "ReliableMessageChannel.h"

using namespace protocol;

class TestChannelStructure : public ChannelStructure
{
    TestMessageFactory m_messageFactory;
    ReliableMessageChannelConfig m_config;

public:

    TestChannelStructure()
    {
        m_config.messageFactory = &m_messageFactory;
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
        return new ReliableMessageChannel( m_config );
    }

    ReliableMessageChannelData * CreateReliableMessageChannelData()
    {
        return new ReliableMessageChannelData( m_config );
    }

    const ReliableMessageChannelConfig & GetConfig() const
    {
        return m_config;
    }
};

#endif
