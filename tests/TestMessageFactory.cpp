#include "TestMessages.h"
#include "Memory.h"

using namespace protocol;

void test_message_factory()
{
    printf( "test_message_factory\n" );

    memory::initialize();
    {
        TestMessageFactory messageFactory( memory::default_allocator() );

        auto testMessage = (TestMessage*) messageFactory.Create( MESSAGE_TEST );
        PROTOCOL_CHECK( testMessage );
        PROTOCOL_CHECK( testMessage->GetRefCount() == 1 );
        PROTOCOL_CHECK( testMessage->GetType() == MESSAGE_TEST );

        auto blockMessage = (BlockMessage*) messageFactory.Create( MESSAGE_BLOCK );
        PROTOCOL_CHECK( blockMessage );
        PROTOCOL_CHECK( blockMessage->GetRefCount() == 1 );
        PROTOCOL_CHECK( blockMessage->GetType() == MESSAGE_BLOCK );

        Block block( memory::default_allocator(), 1024 );
        uint8_t * data = block.GetData();
        memset( data, 1024, 0 );
        blockMessage->Connect( block );

        messageFactory.Release( blockMessage );
        messageFactory.Release( testMessage );
    }
    memory::shutdown();
}
