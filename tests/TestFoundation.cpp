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
    check( allocator.GetAllocatedSize( p ) >= 100 );
    check( allocator.GetTotalAllocated() >= 100 );

    void * q = allocator.Allocate( 100 );
    check( allocator.GetAllocatedSize( q ) >= 100 );
    check( allocator.GetTotalAllocated() >= 200 );
    
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

        check( p );
        check( temp.GetAllocatedSize( p ) >= 100 );
        memset( p, 100, 0 );

        void * q = temp.Allocate( 256 );

        check( q );
        check( temp.GetAllocatedSize( q ) >= 256 );
        memset( q, 256, 0 );

        void * r = temp.Allocate( 2 * 1024 );
        check( r );
        check( temp.GetAllocatedSize( r ) >= 2 * 1024 );
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

        check( array::size(v) == 0 );
        array::push_back( v, 3 );
        check( array::size( v ) == 1 );
        check( v[0] == 3 );

        Array<int> v2( v );
        check( v2[0] == 3 );
        v2[0] = 5;
        check( v[0] == 3 );
        check( v2[0] == 5 );
        v2 = v;
        check( v2[0] == 3 );
        
        check( array::end(v) - array::begin(v) == array::size(v) );
        check( *array::begin(v) == 3);
        array::pop_back(v);
        check( array::empty(v) );

        for ( int i=0; i<100; ++i )
            array::push_back( v, i );

        check( array::size(v) == 100 );
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
        check( hash::get( h, 0, 99 ) == 99 );
        check( !hash::has( h, 0 ) );
        hash::remove( h, 0 );
        hash::set( h, 1000, 123 );
        check( hash::get( h, 1000, 0 ) == 123 );
        check( hash::get( h, 2000, 99 ) == 99 );

        for ( int i = 0; i < 100; ++i )
            hash::set( h, i, i * i );

        for ( int i = 0; i < 100; ++i )
            check( hash::get( h, i, 0 ) == i * i );

        hash::remove( h, 1000 );
        check( !hash::has( h, 1000 ) );

        hash::remove( h, 2000 );
        check( hash::get( h, 1000, 0 ) == 0 );

        for ( int i = 0; i < 100; ++i )
            check( hash::get( h, i, 0 ) == i * i );

        hash::clear( h );

        for ( int i = 0; i < 100; ++i )
            check( !hash::has( h, i ) );
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

        check( multi_hash::count( h, 0 ) == 0 );
        multi_hash::insert( h, 0, 1 );
        multi_hash::insert( h, 0, 2 );
        multi_hash::insert( h, 0, 3 );
        check( multi_hash::count( h, 0 ) == 3 );

        Array<int> a( temp );
        multi_hash::get( h, 0, a );
        check( array::size(a) == 3 );
        std::sort( array::begin(a), array::end(a) );
        check( a[0] == 1 && a[1] == 2 && a[2] == 3 );

        multi_hash::remove( h, multi_hash::find_first( h, 0 ) );
        check( multi_hash::count( h, 0 ) == 2 );
        multi_hash::remove_all( h, 0 );
        check( multi_hash::count( h, 0 ) == 0 );
    }
    memory::shutdown();
}

void test_murmur_hash()
{
    printf( "test_murmur_hash\n" );
    const char * s = "test_string";
    const uint64_t h = murmur_hash_64( s, strlen(s), 0 );
    check( h == 0xe604acc23b568f83ull );
}

void test_queue()
{
    printf( "test_queue\n" );

    memory::initialize();
    {
        TempAllocator1024 temp;

        Queue<int> q( temp );

        queue::reserve( q, 10 );

        check( queue::space( q ) == 10 );

        queue::push_back( q, 11 );
        queue::push_front( q, 22 );

        check( queue::size( q ) == 2 );

        check( q[0] == 22 );
        check( q[1] == 11 );

        queue::consume( q, 2 );
        check( queue::size( q ) == 0 );

        int items[] = { 1,2,3,4,5,6,7,8,9,10 };

        queue::push( q,items,10 );
        
        check( queue::size(q) == 10 );
        
        for ( int i = 0; i < 10; ++i )
            check( q[i] == i + 1 );
        
        queue::consume( q, queue::end_front(q) - queue::begin_front(q) );
        queue::consume( q, queue::end_front(q) - queue::begin_front(q) );
        
        check( queue::size(q) == 0 );
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
        check( *value == buffer[i] );
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

        check
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
