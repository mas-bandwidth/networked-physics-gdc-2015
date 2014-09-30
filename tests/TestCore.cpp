#include "core/Core.h"
#include "core/Memory.h"
#include "core/Array.h"
#include "core/Hash.h"
#include "core/Queue.h"
#include "core/StringStream.h"
#include <string.h>
#include <algorithm>

void test_sequence()
{
    printf( "test_sequence\n" );

    CORE_CHECK( core::sequence_greater_than( 0, 0 ) == false );
    CORE_CHECK( core::sequence_greater_than( 1, 0 ) == true );
    CORE_CHECK( core::sequence_greater_than( 0, -1 ) == true );

    CORE_CHECK( core::sequence_less_than( 0, 0 ) == false );
    CORE_CHECK( core::sequence_less_than( 0, 1 ) == true );
    CORE_CHECK( core::sequence_less_than( -1, 0 ) == true );
}

void test_endian()
{
    printf( "test_endian\n" );

    union
    {
        uint8_t bytes[4];
        uint32_t num;
    } x;

    #if CORE_ENDIAN == CORE_LITTLE_ENDIAN

        x.bytes[0] = 7; 
        x.bytes[1] = 5; 
        x.bytes[2] = 3; 
        x.bytes[3] = 1;
    
    #elif CORE_ENDIAN == CORE_BIG_ENDIAN
    
        x.bytes[0] = 1; 
        x.bytes[1] = 3; 
        x.bytes[2] = 5; 
        x.bytes[3] = 7;
    
    #else
    
        #error endianness is not known!

    #endif

    CORE_CHECK( x.num == 0x01030507 );
}

void test_memory()
{
    printf( "test_memory\n" );

    core::memory::initialize();

    core::Allocator & allocator = core::memory::default_allocator();

    void * p = allocator.Allocate( 100 );
    CORE_CHECK( allocator.GetAllocatedSize( p ) >= 100 );
    CORE_CHECK( allocator.GetTotalAllocated() >= 100 );

    void * q = allocator.Allocate( 100 );
    CORE_CHECK( allocator.GetAllocatedSize( q ) >= 100 );
    CORE_CHECK( allocator.GetTotalAllocated() >= 200 );
    
    allocator.Free( p );
    allocator.Free( q );

    core::memory::shutdown();
}

void test_scratch() 
{
    printf( "test_scratch\n" );

    core::memory::initialize( 256 * 1024 );
    {
        core::Allocator & a = core::memory::scratch_allocator();

        uint8_t * p = (uint8_t*) a.Allocate( 10 * 1024 );

        uint8_t * pointers[100];

        for ( int i = 0; i < 100; ++i )
            pointers[i] = (uint8_t*) a.Allocate( 1024 );

        for ( int i = 0; i < 100; ++i )
            a.Free( pointers[i] );

        a.Free( p );

        for ( int i = 0; i < 100; ++i )
            pointers[i] = (uint8_t*) a.Allocate( 4 * 1024 );

        for ( int i = 0; i < 100; ++i )
            a.Free( pointers[i] );
    }
    core::memory::shutdown();
}

void test_temp_allocator() 
{
    printf( "test_temp_allocator\n" );

    core::memory::initialize();
    {
        core::TempAllocator256 temp;

        void * p = temp.Allocate( 100 );

        CORE_CHECK( p );
        CORE_CHECK( temp.GetAllocatedSize( p ) >= 100 );
        memset( p, 100, 0 );

        void * q = temp.Allocate( 256 );

        CORE_CHECK( q );
        CORE_CHECK( temp.GetAllocatedSize( q ) >= 256 );
        memset( q, 256, 0 );

        void * r = temp.Allocate( 2 * 1024 );
        CORE_CHECK( r );
        CORE_CHECK( temp.GetAllocatedSize( r ) >= 2 * 1024 );
        memset( r, 2*1024, 0 );
    }
    core::memory::shutdown();
}

void test_array() 
{
    printf( "test_array\n" );

    core::memory::initialize();

    core::Allocator & a = core::memory::default_allocator();
    {
        core::Array<int> v( a );

        CORE_CHECK( core::array::size(v) == 0 );
        core::array::push_back( v, 3 );
        CORE_CHECK( core::array::size( v ) == 1 );
        CORE_CHECK( v[0] == 3 );

        core::Array<int> v2( v );
        CORE_CHECK( v2[0] == 3 );
        v2[0] = 5;
        CORE_CHECK( v[0] == 3 );
        CORE_CHECK( v2[0] == 5 );
        v2 = v;
        CORE_CHECK( v2[0] == 3 );
        
        CORE_CHECK( core::array::end(v) - core::array::begin(v) == core::array::size(v) );
        CORE_CHECK( *core::array::begin(v) == 3);
        core::array::pop_back(v);
        CORE_CHECK( core::array::empty(v) );

        for ( int i=0; i<100; ++i )
            core::array::push_back( v, i );

        CORE_CHECK( core::array::size(v) == 100 );
    }

    core::memory::shutdown();
}

void test_hash() 
{
    printf( "test hash\n" );

    core::memory::initialize();
    {
        core::TempAllocator128 temp;

        core::Hash<int> h( temp );
        CORE_CHECK( core::hash::get( h, 0, 99 ) == 99 );
        CORE_CHECK( !core::hash::has( h, 0 ) );
        core::hash::remove( h, 0 );
        core::hash::set( h, 1000, 123 );
        CORE_CHECK( core::hash::get( h, 1000, 0 ) == 123 );
        CORE_CHECK( core::hash::get( h, 2000, 99 ) == 99 );

        for ( int i = 0; i < 100; ++i )
            core::hash::set( h, i, i * i );

        for ( int i = 0; i < 100; ++i )
            CORE_CHECK( core::hash::get( h, i, 0 ) == i * i );

        core::hash::remove( h, 1000 );
        CORE_CHECK( !core::hash::has( h, 1000 ) );

        core::hash::remove( h, 2000 );
        CORE_CHECK( core::hash::get( h, 1000, 0 ) == 0 );

        for ( int i = 0; i < 100; ++i )
            CORE_CHECK( core::hash::get( h, i, 0 ) == i * i );

        core::hash::clear( h );

        for ( int i = 0; i < 100; ++i )
            CORE_CHECK( !core::hash::has( h, i ) );
    }

    core::memory::shutdown();
}

void test_multi_hash()
{
    printf( "test_multi_hash\n" );

    core::memory::initialize();
    {
        core::TempAllocator128 temp;

        core::Hash<int> h( temp );

        CORE_CHECK( core::multi_hash::count( h, 0 ) == 0 );
        core::multi_hash::insert( h, 0, 1 );
        core::multi_hash::insert( h, 0, 2 );
        core::multi_hash::insert( h, 0, 3 );
        CORE_CHECK( core::multi_hash::count( h, 0 ) == 3 );

        core::Array<int> a( temp );
        core::multi_hash::get( h, 0, a );
        CORE_CHECK( core::array::size(a) == 3 );
        std::sort( core::array::begin(a), core::array::end(a) );
        CORE_CHECK( a[0] == 1 && a[1] == 2 && a[2] == 3 );

        core::multi_hash::remove( h, core::multi_hash::find_first( h, 0 ) );
        CORE_CHECK( core::multi_hash::count( h, 0 ) == 2 );
        core::multi_hash::remove_all( h, 0 );
        CORE_CHECK( core::multi_hash::count( h, 0 ) == 0 );
    }
    core::memory::shutdown();
}

void test_murmur_hash()
{
    printf( "test_murmur_hash\n" );
    const char * s = "test_string";
    const uint64_t h = core::murmur_hash_64( s, strlen(s), 0 );
    CORE_CHECK( h == 0xe604acc23b568f83ull );
}

void test_queue()
{
    printf( "test_queue\n" );

    core::memory::initialize();
    {
        core::TempAllocator1024 temp;

        core::Queue<int> q( temp );

        core::queue::reserve( q, 10 );

        CORE_CHECK( core::queue::space( q ) == 10 );

        core::queue::push_back( q, 11 );
        core::queue::push_front( q, 22 );

        CORE_CHECK( core::queue::size( q ) == 2 );

        CORE_CHECK( q[0] == 22 );
        CORE_CHECK( q[1] == 11 );

        core::queue::consume( q, 2 );
        CORE_CHECK( core::queue::size( q ) == 0 );

        int items[] = { 1,2,3,4,5,6,7,8,9,10 };

        core::queue::push( q,items,10 );
        
        CORE_CHECK( core::queue::size(q) == 10 );
        
        for ( int i = 0; i < 10; ++i )
            CORE_CHECK( q[i] == i + 1 );
        
        core::queue::consume( q, core::queue::end_front(q) - core::queue::begin_front(q) );
        core::queue::consume( q, core::queue::end_front(q) - core::queue::begin_front(q) );
        
        CORE_CHECK( core::queue::size(q) == 0 );
    }
}

void test_pointer_arithmetic()
{
    printf( "test_pointer_arithmetic\n" );

    const uint8_t check = (uint8_t)0xfe;
    const unsigned test_size = 128;

    core::TempAllocator512 temp;
    core::Array<uint8_t> buffer( temp );
    core::array::set_capacity( buffer, test_size );
    memset( core::array::begin(buffer), 0, core::array::size(buffer) );

    void * data = core::array::begin( buffer );
    for ( unsigned i = 0; i != test_size; ++i )
    {
        buffer[i] = check;
        uint8_t * value = (uint8_t*) core::pointer_add( data, i );
        CORE_CHECK( *value == buffer[i] );
    }
}

void test_string_stream()
{
    printf( "test_string_stream\n" );

    core::memory::initialize();
    {
        using namespace core::string_stream;

        core::TempAllocator1024 temp;
        Buffer ss( temp );

        ss << "Name";           tab( ss, 20 );    ss << "Score\n";
        repeat( ss, 10, '-' );  tab( ss, 20 );    repeat( ss, 10, '-' ); ss << "\n";
        ss << "Niklas";         tab( ss, 20 );    printf( ss, "%.2f", 2.7182818284f ); ss << "\n";
        ss << "Jim";            tab( ss, 20 );    printf( ss, "%.2f", 3.14159265f ); ss << "\n";

        CORE_CHECK
        (
            0 == strcmp( c_str( ss ),
                "Name                Score\n"
                "----------          ----------\n"
                "Niklas              2.72\n"
                "Jim                 3.14\n"
            )
        );
    }
    core::memory::shutdown();
}
