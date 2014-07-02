#include "Common.h"

using namespace protocol;

#include "Block.h"

void test_block()
{
    printf( "test_block\n" );

    memory::initialize();
    {
        const int BlockSize = 256 * 1024;
        // todo: convert to custom allocator. maybe a block factory even?
        Block * block = new Block( memory::default_allocator(), BlockSize );
        assert( block );
        assert( block->GetAllocator() == &memory::default_allocator() );
        assert( block->GetSize() == BlockSize );
        assert( block->IsValid() );
        uint8_t * data = block->GetData();
        memset( data, 0, BlockSize );
        delete block;
    }
    memory::shutdown();
}
