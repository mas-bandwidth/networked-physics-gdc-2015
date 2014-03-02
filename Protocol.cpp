/*
    Sketching out ideas for protocol library in C++11
    Author: Glenn Fiedler <glenn.fiedler@gmail.com>
*/

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
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

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
            // note: this is a dummy stream implementation to be replaced with a bitpacker, range encoder or arithmetic encoder in future

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

    std::string format_string( const std::string & fmt_str, ... ) 
    {
        int final_n, n = fmt_str.size() * 2; /* reserve 2 times as much as the length of the fmt_str */
        std::string str;
        std::unique_ptr<char[]> formatted;
        va_list ap;
        while(1) {
            formatted.reset(new char[n]); /* wrap the plain char array into the unique_ptr */
            strcpy(&formatted[0], fmt_str.c_str());
            va_start(ap, fmt_str);
            final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
            va_end(ap);
            if (final_n < 0 || final_n >= n)
                n += abs(final_n - n + 1);
            else
                break;
        }
        return std::string(formatted.get());
    }

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

        Address( uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t _port = 0 )
        {
            type = AddressType::IPv4;
            address4 = uint32_t(a) | (uint32_t(b)<<8) | (uint32_t(c)<<16) | (uint32_t(d)<<24);
            port = _port;
        }

        explicit Address( uint32_t address, int16_t _port = 0 )
        {
            type = AddressType::IPv4;
            address4 = htonl( address );        // IMPORTANT: stored in network byte order. eg. big endian!
            port = _port;
        }

        explicit Address( uint16_t a, uint16_t b, uint16_t c, uint16_t d,
                          uint16_t e, uint16_t f, uint16_t g, uint16_t h,
                          uint16_t _port = 0 )
        {
            type = AddressType::IPv6;
            address6[0] = htons( a );
            address6[1] = htons( b );
            address6[2] = htons( c );
            address6[3] = htons( d );
            address6[4] = htons( e );
            address6[5] = htons( f );
            address6[6] = htons( g );
            address6[7] = htons( h );
            port = _port;
        }

        explicit Address( const uint16_t _address[], uint16_t _port = 0 )
        {
            type = AddressType::IPv6;
            for ( int i = 0; i < 8; ++i )
                address6[i] = htons( _address[i] );
            port = _port;
        }

        Address( std::string address )
        {
            // first try to parse as an IPv6 address:
            // 1. if the first character is '[' then it's probably an ipv6 in form "[addr6]:portnum"
            // 2. otherwise try to parse as raw IPv6 address, parse using inet_pton

            port = 0;
            if ( address[0] == '[' )
            {
                const int base_index = address.size() - 1;
                for ( int i = 0; i < 6; ++i )   // note: no need to search past 6 characters as ":65535" is longest port value
                {
                    const int index = base_index - i;
                    if ( address[index] == ':' )
                    {
                        const char * port_string = address.substr( index + 1, 6 ).c_str();
                        port = atoi( port_string );
                        address = address.substr( 1, index - 2 );
                    }
                }
            }
            struct in6_addr sockaddr6;
            if ( inet_pton( AF_INET6, address.c_str(), &sockaddr6 ) == 1 )
            {
                memcpy( address6, &sockaddr6, 16 );
                type = AddressType::IPv6;
                return;
            }

            // otherwise it's probably an IPv4 address:
            // 1. look for ":portnum", if found save the portnum and strip it out
            // 2. parse remaining ipv4 address via inet_pton

            const int base_index = address.size() - 1;
            for ( int i = 0; i < 6; ++i )   // note: no need to search past 6 characters as ":65535" is longest port value
            {
                const int index = base_index - i;
                if ( address[index] == ':' )
                {
                    const char * port_string = address.substr( index + 1, 6 ).c_str();
                    port = atoi( port_string );
                    address = address.substr( 0, index );
                }
            }

            struct sockaddr_in sockaddr4;
            if ( inet_pton( AF_INET, address.c_str(), &sockaddr4.sin_addr ) == 1 )
            {
                type = AddressType::IPv4;
                address4 = sockaddr4.sin_addr.s_addr;
            }
            else
            {
                // nope: it's not an IPv4 address. maybe it's a hostname? set address as undefined.
                type = AddressType::Undefined;
                memset( address6, 0, sizeof( address6 ) );
                port = 0;
            }
        }

        const uint32_t GetAddress4() const
        {
            assert( type == AddressType::IPv4 );
            return address4;
        }

        const uint16_t * GetAddress6() const
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
                const uint8_t a = address4 & 0xff;
                const uint8_t b = (address4>>8) & 0xff;
                const uint8_t c = (address4>>16) & 0xff;
                const uint8_t d = (address4>>24) & 0xff;
                if ( port != 0 )
                    return format_string( "%d.%d.%d.%d:%d", a, b, c, d, port );
                else
                    return format_string( "%d.%d.%d.%d", a, b, c, d );
            }
            else if ( type == AddressType::IPv6 )
            {
                char addressString[INET6_ADDRSTRLEN];
                inet_ntop( AF_INET6, &address6, addressString, INET6_ADDRSTRLEN );
                if ( port != 0 )
                    return format_string( "[%s]:%d", addressString, port );
                else
                    return addressString;
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

        bool operator ==( const Address & other ) const
        {
            if ( type != other.type )
                return false;
            if ( port != other.port )
                return false;
            if ( type == AddressType::IPv4 && address4 == other.address4 )
                return true;
            else if ( type == AddressType::IPv6 && memcmp( address6, other.address6, sizeof( address6 ) ) == 0 )
                return true;
            else
                return false;
        }

    private:

        AddressType type;

        union
        {
            uint32_t address4;
            uint16_t address6[8];
        };

        uint16_t port;    
    };

    class Packet : public Object
    {
        Address address;
        int type;
    public:
        Packet( int _type ) :type(_type) {}
        int GetType() const { return type; }
        void SetAddress( const std::string & _address ) { address = _address; }
        const Address & GetAddress() const { return address; }
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

        virtual void Send( const Address & address, std::shared_ptr<Packet> packet ) = 0;

        virtual void Send( const std::string & address, std::shared_ptr<Packet> packet ) = 0;

        virtual std::shared_ptr<Packet> Receive() = 0;
    };

    class NetworkInterface : public Interface
    {
    public:

        void Send( const Address & address, std::shared_ptr<Packet> packet )
        {
            // todo
        }

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

    virtual void Send( const Address & address, std::shared_ptr<Packet> packet )
    {
        packet->SetAddress( "127.0.0.1" );
        packet_queue.push( packet );
    }

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
    assert( connectPacket->GetAddress() == Address( "127.0.0.1" ) );

    assert( updatePacket->GetType() == PACKET_Update );
    assert( updatePacket->GetAddress() == Address( "127.0.0.1" ) );

    assert( disconnectPacket->GetType() == PACKET_Disconnect );
    assert( disconnectPacket->GetAddress() == Address( "127.0.0.1" ) );

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

void test_address4()
{
    cout << "test_address4" << endl;

    {
        Address address( 127, 0, 0, 1 );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 0 );
        assert( address.GetAddress4() == 0x100007f );
        assert( address.ToString() == "127.0.0.1" );
    }

    {
        Address address( 127, 0, 0, 1, 1000 );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 1000 );
        assert( address.GetAddress4() == 0x100007f );
        assert( address.ToString() == "127.0.0.1:1000" );
    }

    {
        Address address( "127.0.0.1" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 0 );
        assert( address.GetAddress4() == 0x100007f );
        assert( address.ToString() == "127.0.0.1" );
    }

    {
        Address address( "127.0.0.1:65535" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 65535 );
        assert( address.GetAddress4() == 0x100007f );
        assert( address.ToString() == "127.0.0.1:65535" );
    }

    {
        Address address( "10.24.168.192:3000" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 3000 );
        assert( address.GetAddress4() == 0xc0a8180a );
        assert( address.ToString() == "10.24.168.192:3000" );
    }

    {
        Address address( "255.255.255.255:65535" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv4 );
        assert( address.GetPort() == 65535 );
        assert( address.GetAddress4() == 0xffffffff );
        assert( address.ToString() == "255.255.255.255:65535" );
    }
}

void test_address6()
{
    cout << "test_address6" << endl;

    // without port numbers

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6[0], address6[1], address6[2], address6[2],
                         address6[4], address6[5], address6[6], address6[7] );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        assert( address.ToString() == "fe80::202:b3ff:fe1e:8329" );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6 );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        assert( address.ToString() == "fe80::202:b3ff:fe1e:8329" );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6 );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        assert( address.ToString() == "::1" );
    }

    // same addresses but with port numbers

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6[0], address6[1], address6[2], address6[2],
                         address6[4], address6[5], address6[6], address6[7], 65535 );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        assert( address.ToString() == "[fe80::202:b3ff:fe1e:8329]:65535" );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6, 65535 );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        assert( address.ToString() == "[fe80::202:b3ff:fe1e:8329]:65535" );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6, 65535 );

        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            assert( htons( address6[i] ) == address.GetAddress6()[i] );

        assert( address.ToString() == "[::1]:65535" );
    }

    // parse addresses from strings (no ports)

    {
        Address address( "fe80::202:b3ff:fe1e:8329" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 0 );
        assert( address.ToString() == "fe80::202:b3ff:fe1e:8329" );
    }

    {
        Address address( "::1" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 0 );
        assert( address.ToString() == "::1" );
    }

    // parse addresses from strings (with ports)

    {
        Address address( "[fe80::202:b3ff:fe1e:8329]:65535" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 65535 );
        assert( address.ToString() == "[fe80::202:b3ff:fe1e:8329]:65535" );
    }

    {
        Address address( "[::1]:65535" );
        assert( address.IsValid() );
        assert( address.GetType() == AddressType::IPv6 );
        assert( address.GetPort() == 65535 );
        assert( address.ToString() == "[::1]:65535" );
    }
}

// --------------------------------------------------------------

int main()
{
    try
    {
        test_serialize_object();
        test_interface();
        test_factory();
        /*
        test_dns();
        test_address();
        */
        test_address4();
        test_address6();
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