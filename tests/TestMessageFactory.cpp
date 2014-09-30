#include "core/Memory.h"
#include "TestMessages.h"
#include <string.h>

void test_message_factory()
{
    printf( "test_message_factory\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        auto testMessage = (TestMessage*) messageFactory.Create( MESSAGE_TEST );
        CORE_CHECK( testMessage );
        CORE_CHECK( testMessage->GetRefCount() == 1 );
        CORE_CHECK( testMessage->GetType() == MESSAGE_TEST );

        auto blockMessage = (protocol::BlockMessage*) messageFactory.Create( MESSAGE_BLOCK );
        CORE_CHECK( blockMessage );
        CORE_CHECK( blockMessage->GetRefCount() == 1 );
        CORE_CHECK( blockMessage->GetType() == MESSAGE_BLOCK );

        protocol::Block block( core::memory::default_allocator(), 1024 );
        uint8_t * data = block.GetData();
        memset( data, 1024, 0 );
        blockMessage->Connect( block );

        messageFactory.Release( blockMessage );
        messageFactory.Release( testMessage );
    }
    core::memory::shutdown();
}
