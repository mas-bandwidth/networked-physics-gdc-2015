#include "protocol/Object.h"
#include "protocol/Stream.h"
#include <stdio.h>
#include <string.h>

const int MaxItems = 16;

struct TestObject : public protocol::Object
{
    int a,b,c;
    uint32_t d : 8;
    uint32_t e : 8;
    uint32_t f : 8;
    bool g;
    int numItems;
    int items[MaxItems];

    TestObject()
    {
        a = 0;
        b = 0;
        c = 0;
        d = 0;
        e = 0;
        f = 0;
        g = false;
        numItems = 0;
        memset( items, 0, sizeof(items) );
    }

    void Init()
    {
        a = 1;
        b = -2;
        c = 150;
        d = 55;
        e = 255;
        f = 127;
        g = true;
        numItems = MaxItems / 2;
        for ( int i = 0; i < numItems; ++i )
            items[i] = i + 10;        
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_int( stream, a, 0, 10 );
        serialize_int( stream, b, -5, +5 );
        serialize_int( stream, c, -100, 10000 );

        serialize_bits( stream, d, 6 );
        serialize_bits( stream, e, 8 );
        serialize_bits( stream, f, 7 );

        serialize_bool( stream, g );

        serialize_int( stream, numItems, 0, MaxItems - 1 );
        for ( int i = 0; i < numItems; ++i )
            serialize_bits( stream, items[i], 8 );
    }
};

void test_stream()
{
    printf( "test_stream\n" );

    const int BufferSize = 256;

    uint8_t buffer[BufferSize];

    // write the object

    TestObject writeObject;
    writeObject.Init();
    {
        protocol::WriteStream writeStream( buffer, BufferSize );
        writeObject.SerializeWrite( writeStream );
        writeStream.Flush();
    }

    // read the object back

    TestObject readObject;
    {
        protocol::ReadStream readStream( buffer, BufferSize );
        readObject.SerializeRead( readStream );
    }

    // verify read object matches written object

    CORE_CHECK( readObject.a == writeObject.a );
    CORE_CHECK( readObject.b == writeObject.b );
    CORE_CHECK( readObject.c == writeObject.c );
    CORE_CHECK( readObject.d == writeObject.d );
    CORE_CHECK( readObject.e == writeObject.e );
    CORE_CHECK( readObject.f == writeObject.f );
    CORE_CHECK( readObject.g == writeObject.g );
    CORE_CHECK( readObject.numItems == writeObject.numItems );
    for ( int i = 0; i < readObject.numItems; ++i )
        CORE_CHECK( readObject.items[i] == writeObject.items[i] );
}

struct ContextA
{
    int min = -10;
    int max = +5;
};

struct ContextB
{
    int min = -50;
    int max = 23;
};

struct TestContextObject : public protocol::Object
{
    int a,b;

    TestContextObject()
    {
        a = 0;
        b = 0;
    }

    template <typename Stream> void Serialize( Stream & stream )
    {
        auto context_a = (const ContextA*) stream.GetContext( 0 );
        auto context_b = (const ContextB*) stream.GetContext( 1 );

        CORE_CHECK( context_a );
        CORE_CHECK( context_b );

        serialize_int( stream, a, context_a->min, context_a->max );
        serialize_int( stream, b, context_b->min, context_b->max );
    }

    void SerializeRead( protocol::ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( protocol::WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( protocol::MeasureStream & stream )
    {
        Serialize( stream );
    }
};

void test_stream_context()
{
    printf( "test_stream_context\n" );

    const int BufferSize = 256;

    uint8_t buffer[BufferSize];

    ContextA context_a;
    ContextB context_b;

    const void * context[] = { &context_a, &context_b };

    // write the object with context

    TestContextObject writeObject;
    writeObject.a = 2;
    writeObject.b = 7;
    {
        protocol::WriteStream writeStream( buffer, BufferSize );

        CORE_CHECK( writeStream.GetContext(0) == nullptr );
        CORE_CHECK( writeStream.GetContext(1) == nullptr );

        writeStream.SetContext( &context[0] );

        CORE_CHECK( writeStream.GetContext(0) == &context_a );
        CORE_CHECK( writeStream.GetContext(1) == &context_b );

        writeObject.SerializeWrite( writeStream );

        writeStream.Flush();
    }

    // read the object back with context

    TestContextObject readObject;
    {
        protocol::ReadStream readStream( buffer, BufferSize );

        readStream.SetContext( &context[0] );

        CORE_CHECK( readStream.GetContext(0) == &context_a );
        CORE_CHECK( readStream.GetContext(1) == &context_b );

        readObject.SerializeRead( readStream );
    }

    // verify read object matches written object

    CORE_CHECK( readObject.a == writeObject.a );
    CORE_CHECK( readObject.b == writeObject.b );
}
