#include "Memory.h"
#include "Array.h"
#include "Hash.h"
#include "Queue.h"
#include "StringStream.h"
#include <string.h>
#include <algorithm>

using namespace protocol;

void test_memory()
{
    printf( "test_memory\n" );

    memory::initialize();

    Allocator & allocator = memory::default_allocator();

    void * p = allocator.Allocate( 100 );
    PROTOCOL_CHECK( allocator.GetAllocatedSize( p ) >= 100 );
    PROTOCOL_CHECK( allocator.GetTotalAllocated() >= 100 );

    void * q = allocator.Allocate( 100 );
    PROTOCOL_CHECK( allocator.GetAllocatedSize( q ) >= 100 );
    PROTOCOL_CHECK( allocator.GetTotalAllocated() >= 200 );
    
    allocator.Free( p );
    allocator.Free( q );

    memory::shutdown();
}

void test_scratch() 
{
    printf( "test_scratch\n" );

    memory::initialize( 256 * 1024 );
    {
        Allocator & a = memory::scratch_allocator();

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
    memory::shutdown();
}

void test_temp_allocator() 
{
    printf( "test_temp_allocator\n" );

    memory::initialize();
    {
        TempAllocator256 temp;

        void * p = temp.Allocate( 100 );

        PROTOCOL_CHECK( p );
        PROTOCOL_CHECK( temp.GetAllocatedSize( p ) >= 100 );
        memset( p, 100, 0 );

        void * q = temp.Allocate( 256 );

        PROTOCOL_CHECK( q );
        PROTOCOL_CHECK( temp.GetAllocatedSize( q ) >= 256 );
        memset( q, 256, 0 );

        void * r = temp.Allocate( 2 * 1024 );
        PROTOCOL_CHECK( r );
        PROTOCOL_CHECK( temp.GetAllocatedSize( r ) >= 2 * 1024 );
        memset( r, 2*1024, 0 );
    }
    memory::shutdown();
}

void test_array() 
{
    printf( "test_array\n" );

    memory::initialize();

    Allocator & a = memory::default_allocator();
    {
        Array<int> v( a );

        PROTOCOL_CHECK( array::size(v) == 0 );
        array::push_back( v, 3 );
        PROTOCOL_CHECK( array::size( v ) == 1 );
        PROTOCOL_CHECK( v[0] == 3 );

        Array<int> v2( v );
        PROTOCOL_CHECK( v2[0] == 3 );
        v2[0] = 5;
        PROTOCOL_CHECK( v[0] == 3 );
        PROTOCOL_CHECK( v2[0] == 5 );
        v2 = v;
        PROTOCOL_CHECK( v2[0] == 3 );
        
        PROTOCOL_CHECK( array::end(v) - array::begin(v) == array::size(v) );
        PROTOCOL_CHECK( *array::begin(v) == 3);
        array::pop_back(v);
        PROTOCOL_CHECK( array::empty(v) );

        for ( int i=0; i<100; ++i )
            array::push_back( v, i );

        PROTOCOL_CHECK( array::size(v) == 100 );
    }

    memory::shutdown();
}

void test_hash() 
{
    printf( "test hash\n" );

    memory::initialize();
    {
        TempAllocator128 temp;

        Hash<int> h( temp );
        PROTOCOL_CHECK( hash::get( h, 0, 99 ) == 99 );
        PROTOCOL_CHECK( !hash::has( h, 0 ) );
        hash::remove( h, 0 );
        hash::set( h, 1000, 123 );
        PROTOCOL_CHECK( hash::get( h, 1000, 0 ) == 123 );
        PROTOCOL_CHECK( hash::get( h, 2000, 99 ) == 99 );

        for ( int i = 0; i < 100; ++i )
            hash::set( h, i, i * i );

        for ( int i = 0; i < 100; ++i )
            PROTOCOL_CHECK( hash::get( h, i, 0 ) == i * i );

        hash::remove( h, 1000 );
        PROTOCOL_CHECK( !hash::has( h, 1000 ) );

        hash::remove( h, 2000 );
        PROTOCOL_CHECK( hash::get( h, 1000, 0 ) == 0 );

        for ( int i = 0; i < 100; ++i )
            PROTOCOL_CHECK( hash::get( h, i, 0 ) == i * i );

        hash::clear( h );

        for ( int i = 0; i < 100; ++i )
            PROTOCOL_CHECK( !hash::has( h, i ) );
    }

    memory::shutdown();
}

void test_multi_hash()
{
    printf( "test_multi_hash\n" );

    memory::initialize();
    {
        TempAllocator128 temp;

        Hash<int> h( temp );

        PROTOCOL_CHECK( multi_hash::count( h, 0 ) == 0 );
        multi_hash::insert( h, 0, 1 );
        multi_hash::insert( h, 0, 2 );
        multi_hash::insert( h, 0, 3 );
        PROTOCOL_CHECK( multi_hash::count( h, 0 ) == 3 );

        Array<int> a( temp );
        multi_hash::get( h, 0, a );
        PROTOCOL_CHECK( array::size(a) == 3 );
        std::sort( array::begin(a), array::end(a) );
        PROTOCOL_CHECK( a[0] == 1 && a[1] == 2 && a[2] == 3 );

        multi_hash::remove( h, multi_hash::find_first( h, 0 ) );
        PROTOCOL_CHECK( multi_hash::count( h, 0 ) == 2 );
        multi_hash::remove_all( h, 0 );
        PROTOCOL_CHECK( multi_hash::count( h, 0 ) == 0 );
    }
    memory::shutdown();
}

void test_murmur_hash()
{
    printf( "test_murmur_hash\n" );
    const char * s = "test_string";
    const uint64_t h = murmur_hash_64( s, strlen(s), 0 );
    PROTOCOL_CHECK( h == 0xe604acc23b568f83ull );
}

void test_queue()
{
    printf( "test_queue\n" );

    memory::initialize();
    {
        TempAllocator1024 temp;

        Queue<int> q( temp );

        queue::reserve( q, 10 );

        PROTOCOL_CHECK( queue::space( q ) == 10 );

        queue::push_back( q, 11 );
        queue::push_front( q, 22 );

        PROTOCOL_CHECK( queue::size( q ) == 2 );

        PROTOCOL_CHECK( q[0] == 22 );
        PROTOCOL_CHECK( q[1] == 11 );

        queue::consume( q, 2 );
        PROTOCOL_CHECK( queue::size( q ) == 0 );

        int items[] = { 1,2,3,4,5,6,7,8,9,10 };

        queue::push( q,items,10 );
        
        PROTOCOL_CHECK( queue::size(q) == 10 );
        
        for ( int i = 0; i < 10; ++i )
            PROTOCOL_CHECK( q[i] == i + 1 );
        
        queue::consume( q, queue::end_front(q) - queue::begin_front(q) );
        queue::consume( q, queue::end_front(q) - queue::begin_front(q) );
        
        PROTOCOL_CHECK( queue::size(q) == 0 );
    }
}

void test_pointer_arithmetic()
{
    printf( "test_pointer_arithmetic\n" );

    const uint8_t check = (uint8_t)0xfe;
    const unsigned test_size = 128;

    TempAllocator512 temp;
    Array<uint8_t> buffer( temp );
    array::set_capacity( buffer, test_size );
    memset( array::begin(buffer), 0, array::size(buffer) );

    void * data = array::begin( buffer );
    for ( unsigned i = 0; i != test_size; ++i )
    {
        buffer[i] = check;
        uint8_t * value = (uint8_t*) pointer_add( data, i );
        PROTOCOL_CHECK( *value == buffer[i] );
    }
}

void test_string_stream()
{
    printf( "test_string_stream\n" );

    memory::initialize();
    {
        using namespace string_stream;

        TempAllocator1024 temp;
        Buffer ss( temp );

        ss << "Name";           tab( ss, 20 );    ss << "Score\n";
        repeat( ss, 10, '-' );  tab( ss, 20 );    repeat( ss, 10, '-' ); ss << "\n";
        ss << "Niklas";         tab( ss, 20 );    printf( ss, "%.2f", 2.7182818284f ); ss << "\n";
        ss << "Jim";            tab( ss, 20 );    printf( ss, "%.2f", 3.14159265f ); ss << "\n";

        PROTOCOL_CHECK
        (
            0 == strcmp( c_str( ss ),
                "Name                Score\n"
                "----------          ----------\n"
                "Niklas              2.72\n"
                "Jim                 3.14\n"
            )
        );
    }
    memory::shutdown();
}
