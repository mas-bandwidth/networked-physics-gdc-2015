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
            if ( itor == create_map.end() )
                throw std::runtime_error( "invalid object type id in factory create" );
            else
                return itor->second();
        }

    private:

        std::map<int,create_function> create_map;
    };

    class Interface
    {
    public:

        virtual void Send( const std::string & address, std::shared_ptr<Packet> packet ) = 0;

        virtual std::shared_ptr<Packet> Receive() = 0;
    };

    class NetworkInterface : public Interface
    {
    public:

        void Send( const std::string & address, std::shared_ptr<Packet> packet )
        {
            // todo
        }

        std::shared_ptr<Packet> Receive()
        {
            // todo
            return nullptr;
        }

    private:

        // todo: send queue

        // todo: recv queue

        // todo: worker thread for sending packets (wakes up when work to do to send packets...)

        // todo: worker thread for receiving packets (use blocking sockets initially...)
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
}

// --------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void test_dns()
{
   struct addrinfo hints, *res, *p;
   char ipstr[INET6_ADDRSTRLEN];

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
   hints.ai_socktype = SOCK_DGRAM;

   const char hostname[] = "google.com";
   printf( "IP addresses for %s:\n", hostname );

    if ( getaddrinfo( hostname, NULL, &hints, &res ) != 0) 
    {
        cout << "error: getaddrinfo failed" << endl;
        return;
    }

   for( p = res;p != NULL; p = p->ai_next )
   {
       void *addr;
       const char *ipver;
       // get the pointer to the address itself,
       // different fields in IPv4 and IPv6:
       if (p->ai_family == AF_INET) 
       { 
            // IPv4
           struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
           addr = &(ipv4->sin_addr);/*error*/
           ipver = "IPv4";
       } 
       else 
       { 
            // IPv6
           struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
           addr = &(ipv6->sin6_addr);/*error*/
           ipver = "IPv6";
       }
       // convert the IP to a string and print it:
       inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
       printf( "  %s: %s\n", ipver, ipstr );
   }
   freeaddrinfo( res );
}

// --------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

void test_address()
{
    int    rc;
    struct in6_addr serveraddr;
    struct addrinfo hints, *res = NULL;

    const char * server = "127.0.0.1:300";
    //"2001:db8::1";
    //"[2001:db8::1]:80";
    //"google.com";   

    memset( &hints, 0, sizeof(hints) );
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    /********************************************************************/
    /* Check if we were provided the address of the server using        */
    /* inet_pton() to convert the text form of the address to binary    */
    /* form. If it is numeric then we want to prevent getaddrinfo()     */
    /* from doing any name resolution.                                  */
    /********************************************************************/
    rc = inet_pton( AF_INET, server, &serveraddr );
    if (rc == 1)    /* valid IPv4 text address? */
    {
        cout << "valid IPv4 address" << endl;
        hints.ai_family = AF_INET;
        hints.ai_flags |= AI_NUMERICHOST;
    }
    else 
    {
        rc = inet_pton( AF_INET6, server, &serveraddr );
        if (rc == 1) /* valid IPv6 text address? */
        {
            cout << "valid IPv6 address" << endl;
            hints.ai_family = AF_INET6;
            hints.ai_flags |= AI_NUMERICHOST;
        }
        else
        {
            cout << "hostname that needs to be resolved: " + string( server ) << endl;
        }
    }

    /********************************************************************/
    /* Get the address information for the server using getaddrinfo().  */
    /********************************************************************/
    cout << "before getaddrinfo" << endl;
    rc = getaddrinfo( server, "4000", &hints, &res );
    if (rc != 0)
    {
        cout << "error: getaddrinfo" << endl; 
        return;
    }

    cout << "getaddrinfo succeeded" << endl;
}

// --------------------------------------------------------------

enum class AddressType : char
{
    Undefined,
    IPv4,
    IPv6
};

class Address
{
public:

    Address()
    {
        type = AddressType::Undefined;
        memset( address6, 0, sizeof( address6 ) );
        port = 0;
    }

    explicit Address( uint32_t address, int16_t _port )
    {
        type = AddressType::IPv4;
        address4 = address;
        port = _port;
    }

    explicit Address( uint8_t address[], uint16_t _port )
    {
        type = AddressType::IPv6;
        memcpy( address6, address, sizeof( address6 ) );
        port = _port;
    }

    Address( std::string address )
    {
        // todo: if address starts with "[" then it is an IPv6 address with port
        // strip out the [ ] and port in this case, then leave the string as the bit inside []

        // try to parse what remains as an IPv6 address. if it passes then go with that.

        // otherwise it's probably an IPv6 address. 
        // 1. look for port, if found strip out
        // 2. parse ipv4 address

        port = 0;
        const int base_index = address.size() - 1;
        for ( int i = 0; i < 6; ++i )   // note: no need to search past 6 characters as ":65535" is longest port value
        {
            const int index = base_index - i;
            if ( address[index] == ':' )
            {
                const char * port_string = address.substr( index + 1, 6 ).c_str();
                printf( "port string = %s\n", port_string );
                port = atoi( port_string );
                printf( "found port: %d\n", port );

                address = address.substr( 0, index );

                // todo: detect IPv6. if first and last characters are '[' and ']' respectively. trim the string again

                printf( "remaining address = %s\n", address.c_str() );
            }
        }

        struct sockaddr_in sockaddr4;
        int rc = inet_pton( AF_INET, address.c_str(), &(sockaddr4.sin_addr) );
        if ( rc == 1 )
        {
            cout << "valid IPv4 address" << endl;
            type = AddressType::IPv4;
            address4 = sockaddr4.sin_addr.s_addr;
            printf( "ipv4 address: %x\n", address4 );
        }
        else 
        {
            struct in6_addr sockaddr6;
            rc = inet_pton( AF_INET6, address.c_str(), &sockaddr6 );
            if ( rc == 1 )
            {
                cout << "valid IPv6 address" << endl;
                memcpy( address6, &sockaddr6, 16 );
                type = AddressType::IPv6;
            }
            else
            {
                type = AddressType::Undefined;
                memset( address6, 0, sizeof( address6 ) );
                port = 0;
            }
        }
    }

    const uint32_t GetAddress4() const
    {
        assert( type == AddressType::IPv4 );
        return address4;
    }

    const uint8_t * GetAddress6() const
    {
        assert( type == AddressType::IPv6 );
        return address6;
    }

    void SetPort( uint64_t _port )
    {
        port = _port;
    }

    const uint16_t GetPort() const 
    {
        return port;
    }

    AddressType GetType() const
    {
        return type;
    }

    std::string ToString() const
    {
        if ( type == AddressType::IPv4 )
        {
            // todo: convert to standard string representation
            return "127.0.0.1:200";
        }
        else if ( type == AddressType::IPv6 )
        {
            // todo: convert to string representation
            return "[::1]:200";
        }
        else
        {
            return "undefined";
        }
    }

    bool IsValid() const
    {
        return type != AddressType::Undefined;
    }

private:

    AddressType type;

    union
    {
        uint32_t address4;
        uint8_t address6[16];
    };

    uint16_t port;    
};

void test_address_4()
{
    cout << "test_address2" << endl;

    {
        Address address( "127.0.0.1" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 0 );
        assert( address.GetAddress4() == 0x100007f );

        // todo: convert ipv6 address back to string
        //assert( address.ToString() == "127.0.0.1" );
    }

    {
        Address address( "127.0.0.1:65535" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 65535 );
        assert( address.GetAddress4() == 0x100007f );

        // todo: convert ipv6 address back to string
        //assert( address.ToString() == "127.0.0.1:65535" );
    }

    {
        Address address( "255.255.255.255:65535" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 65535 );
        assert( address.GetAddress4() == 0xffffffff );

        // todo: convert ipv6 address back to string
        //assert( address.ToString() == "255.255.255.255:65535" );
    }
}

void test_address_6()
{
    {
        Address address( "::1" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 0 );
        const uint8_t address6[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 };
        assert( memcmp( address.GetAddress6(), address6, 16 ) == 1 );

        // todo: check to string
    }

    /*
    Address address( "[::1]:300" );
    assert( address.IsValid() );
    assert( address.GetType() == AddressType::IPv6 );
    assert( address.GetPort() == 300 );
    const uint8_t address6[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 };
    assert( memcmp( address.GetAddress6(), address6, 16 ) == 1 );
    assert( address.ToString() == "[::1]:300" );
    */

    // todo: other ipv6 addresses
}

// --------------------------------------------------------------

int main()
{
    try
    {
        /*
        test_serialize_object();
        test_interface();
        test_factory();
        test_dns();
        test_address();
        */
        test_address_4();
        //test_address_6();
    }
    catch ( runtime_error & e )
    {
        cout << string( "error: " ) + e.what() << endl;
    }

    return 0;
}

// ----------------------------------------------------------------

// const expression

#if 0
enum Flags { good=0, fail=1, bad=2, eof=4 };

    constexpr int operator|(Flags f1, Flags f2) { return Flags(int(f1)|int(f2)); }

    void f(Flags x)
    {
        switch (x) {
        case bad:         /* ... */ break;
        case eof:         /* ... */ break;
        case bad|eof:     /* ... */ break;
        default:          /* ... */ break;
        }
    }
#endif

// quite cute

/*
    template<class T, class U>
    auto mul(T x, U y) -> decltype(x*y)
    {
        return x*y;
    }
*/

// ----------------------------------------------------------------