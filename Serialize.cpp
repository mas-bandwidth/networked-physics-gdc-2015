// serialize test

#include <cassert>
#include <string>
#include <vector>
#include <limits>
#include <memory>
#include <vector>
#include <queue>
#include <map>
#include <iostream>
#include <stdexcept>

namespace protocol
{
    class serialize_exception : public std::runtime_error
    {
    public:
        serialize_exception( const std::string & err )
            : std::runtime_error( err ) {}
    };

    struct StreamValue
    {
        StreamValue( int64_t _value, int64_t _min, int64_t _max )
            : value( _value ),
              min( _min ),
              max( _max ) {}

        int64_t value;         // the actual value to be serialized
        int64_t min;           // the minimum value
        int64_t max;           // the maximum value
    };

    enum StreamMode
    {
        STREAM_Read,
        STREAM_Write
    };

    class Stream
    {
    public:

        Stream( StreamMode mode )
        {
            SetMode( mode );
        }

        void SetMode( StreamMode mode )
        {
            m_mode = mode;
            m_readIndex = 0;
        }

        bool IsReading() const 
        {
            return m_mode == STREAM_Read;
        }

        bool IsWriting() const
        {
            return m_mode == STREAM_Write;
        }

        void SerializeValue( int64_t & value, int64_t min, int64_t max )
        {
            if ( m_mode == STREAM_Write )
            {
                assert( value >= min );
                assert( value <= max );
                m_values.push_back( StreamValue( value, min, max ) );
            }
            else
            {
                if ( m_readIndex >= m_values.size() )
                    throw serialize_exception( "read past end of stream" );

                StreamValue & streamValue = m_values[m_readIndex++];
                if ( streamValue.min != min || streamValue.max != max )
                    throw serialize_exception( "stream min/max desync between read and write" );

                if ( streamValue.value < streamValue.min || streamValue.value > streamValue.max )
                    throw serialize_exception( "value read from stream is outside min/max range" );

                value = streamValue.value;
            }
        }

        StreamMode m_mode;
        std::vector<StreamValue> m_values;
        size_t m_readIndex;
    };

    class Object
    {  
    public:
        virtual void Serialize( Stream & stream ) = 0;
    };

    class Packet : public Object
    {
        std::string address;
        int type;
    public:
        Packet( int _type ) :type(_type) {}
        int GetType() const { return type; }
        void SetAddress( const std::string & _address ) { address = _address; }
        const std::string & GetAddress() const { return address; }
    };

    template<typename T> void serialize_int( Stream & stream, T & value, int64_t min, int64_t max )
    {                        
        int64_t int64_value = (int64_t) value;
        stream.SerializeValue( int64_value, min, max );
        value = (T) value;
    }

    void serialize_object( Stream & stream, Object & object )
    {                        
        object.Serialize( stream );
    }

    class Interface
    {
    public:

        virtual void Send( const std::string & address, std::shared_ptr<Packet> packet ) = 0;

        virtual std::shared_ptr<Packet> Receive() = 0;
    };

    template <typename T> class Factory
    {
    public:

        typedef std::function< std::shared_ptr<T>() > create_function;

        void Register( int type, create_function const & function )
        {
            create_map[type] = function;
        }

        std::shared_ptr<T> Create( int type )
        {
            auto itor = create_map.find( type );
            assert( itor != create_map.end() );
            if ( itor == create_map.end() )
                throw std::runtime_error( "invalid object type passed to create" );
            else
                return itor->second();
        }

    private:

        std::map<int,create_function> create_map;
    };  
}

// -------------------------------------------------------

using namespace std;
using namespace protocol;

const int MaxItems = 16;

struct TestObject : public Object
{
    int a,b,c;
    int numItems;
    int items[MaxItems];

    TestObject()
    {
        a = 1;
        b = 2;
        c = 150;
        numItems = MaxItems / 2;
        for ( int i = 0; i < numItems; ++i )
            items[i] = i + 10;
    }

    void Serialize( Stream & stream )
    {
        serialize_int( stream, a, 0, 10 );
        serialize_int( stream, b, -5, +5 );
        serialize_int( stream, c, 100, 10000 );
        serialize_int( stream, numItems, 0, MaxItems - 1 );
        for ( int i = 0; i < numItems; ++i )
            serialize_int( stream, items[i], 0, 255 );
    }
};

void test_serialize_object()
{
    cout << "test_serialize_object" << endl;

    // write the object

    Stream stream( STREAM_Write );
    TestObject writeObject;
    writeObject.Serialize( stream );

    // read the object back

    stream.SetMode( STREAM_Read );
    TestObject readObject;
    readObject.Serialize( stream );

    // verify read object matches written object

    assert( readObject.a == writeObject.a );
    assert( readObject.b == writeObject.b );
    assert( readObject.c == writeObject.c );
    assert( readObject.numItems == writeObject.numItems );
    for ( int i = 0; i < readObject.numItems; ++i )
        assert( readObject.items[i] == writeObject.items[i] );
}

enum PacketType
{
    PACKET_Connect,
    PACKET_Update,
    PACKET_Disconnect
};

struct ConnectPacket : public Packet
{
    int a,b,c;

    ConnectPacket() : Packet( PACKET_Connect )
    {
        a = 1;
        b = 2;
        c = 3;        
    }

    void Serialize( Stream & stream )
    {
        serialize_int( stream, a, 0, 10 );
        serialize_int( stream, b, 0, 10 );
        serialize_int( stream, c, 0, 10 );
    }
};

struct UpdatePacket : public Packet
{
    uint16_t timestamp;

    UpdatePacket() : Packet( PACKET_Update )
    {
        timestamp = 0;
    }

    void Serialize( Stream & stream )
    {
        serialize_int( stream, timestamp, 0, 65535 );
    }
};

struct DisconnectPacket : public Packet
{
    DisconnectPacket() : Packet( PACKET_Disconnect ) {}
    void Serialize( Stream & stream ) {}
};

typedef std::queue< shared_ptr<Packet> > PacketQueue;

class TestInterface : public Interface
{
public:

    virtual void Send( const std::string & address, shared_ptr<Packet> packet )
    {
        packet->SetAddress( "127.0.0.1" );
        packet_queue.push( packet );
    }

    virtual shared_ptr<Packet> Receive()
    {
        if ( packet_queue.empty() )
            return nullptr;
        auto packet = packet_queue.front();
        packet_queue.pop();
        return packet;
    }

    PacketQueue packet_queue;
};

void test_interface()
{
    cout << "test_interface" << endl;

    TestInterface interface;

    interface.Send( "127.0.0.1", make_shared<ConnectPacket>() );
    interface.Send( "127.0.0.1", make_shared<UpdatePacket>() );
    interface.Send( "127.0.0.1", make_shared<DisconnectPacket>() );

    auto connectPacket = interface.Receive();
    auto updatePacket = interface.Receive();
    auto disconnectPacket = interface.Receive();

    assert( connectPacket->GetType() == PACKET_Connect );
    assert( connectPacket->GetAddress() == "127.0.0.1" );

    assert( updatePacket->GetType() == PACKET_Update );
    assert( updatePacket->GetAddress() == "127.0.0.1" );

    assert( disconnectPacket->GetType() == PACKET_Disconnect );
    assert( disconnectPacket->GetAddress() == "127.0.0.1" );

    assert( interface.Receive() == nullptr );
}

void test_factory()
{
    cout << "test_factory" << endl;

    Factory<Packet> factory;

    factory.Register( PACKET_Connect, [] { return make_shared<ConnectPacket>(); } );
    factory.Register( PACKET_Update, [] { return make_shared<UpdatePacket>(); } );
    factory.Register( PACKET_Disconnect, [] { return make_shared<DisconnectPacket>(); } );

    auto connectPacket = factory.Create( PACKET_Connect );
    auto updatePacket = factory.Create( PACKET_Update );
    auto disconnectPacket = factory.Create( PACKET_Disconnect );

    assert( connectPacket->GetType() == PACKET_Connect );
    assert( updatePacket->GetType() == PACKET_Update );
    assert( disconnectPacket->GetType() == PACKET_Disconnect );

    factory.Create( 5 );
}

int main()
{
    try
    {
        test_serialize_object();
        test_interface();
        test_factory();
    }
    catch ( runtime_error & e )
    {
        cout << string( "error: " ) + e.what() << endl;
    }

    return 0;
}
