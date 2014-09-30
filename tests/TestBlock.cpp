#include "core/Core.h"
#include "core/Memory.h"
#include "protocol/Block.h"

void test_block()
{
    printf( "test_block\n" );

    core::memory::initialize();
    
    {
        const int BlockSize = 256 * 1024;
        core::Allocator & a = core::memory::default_allocator();
        protocol::Block * block = CORE_NEW( a, protocol::Block, a, BlockSize );
        CORE_CHECK( block );
        CORE_CHECK( block->GetAllocator() == &core::memory::default_allocator() );
        CORE_CHECK( block->GetSize() == BlockSize );
        CORE_CHECK( block->IsValid() );
        uint8_t * data = block->GetData();
        memset( data, 0, BlockSize );
        CORE_DELETE( a, Block, block );
    }

    core::memory::shutdown();
}
