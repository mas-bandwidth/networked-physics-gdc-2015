/*
    Sketching out ideas for a network protocol library
    Author: Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#include <cassert>
#include <string>
#include <vector>
#include <limits>
#include <memory>
#include <vector>
#include <thread>
#include <future>
#include <queue>
#include <chrono>
#include <map>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PLATFORM_WINDOWS  1
#define PLATFORM_MAC      2
#define PLATFORM_UNIX     3

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#else
#define PLATFORM PLATFORM_UNIX
#endif

#if PLATFORM == PLATFORM_WINDOWS

    #include <winsock2.h>
    #pragma comment( lib, "wsock32.lib" )

#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>

#else

    #error unknown platform!

#endif

namespace protocol
{
    using namespace std;

    struct TimeBase
    {
        TimeBase() : time(0), deltaTime(0) {}

        double time;                    // frame time. 0.0 is start of process
        double deltaTime;               // delta time this frame in seconds.
    };

    string format_string( const char * fmt_str, ... ) 
    {
        int final_n, n = 256;
        string str;
        unique_ptr<char[]> formatted;
        va_list ap;
        while(1) 
        {
            formatted.reset(new char[n]);
            strcpy(&formatted[0], fmt_str);
            va_start(ap, fmt_str);
            final_n = vsnprintf(&formatted[0], n, fmt_str, ap);
            va_end(ap);
            if (final_n < 0 || final_n >= n)
                n += abs(final_n - n + 1);
            else
                break;
        }
        return string(formatted.get());
    }

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
                    throw runtime_error( "read past end of stream" );

                StreamValue & streamValue = m_values[m_readIndex++];

                if ( streamValue.min != min || streamValue.max != max )
                    throw runtime_error( format_string( "min/max stream mismatch: [%lld,%lld] vs. [%lld,%lld]", streamValue.min, streamValue.max, min, max ) );

                if ( streamValue.value < min || streamValue.value > max )
                    throw runtime_error( format_string( "value %lld read from stream is outside min/max range [%lld,%lld]", streamValue.value, min, max ) );

                value = streamValue.value;
            }
        }

        const uint8_t * GetWriteBuffer( size_t & bufferSize )
        {
            bufferSize = sizeof( StreamValue ) * m_values.size();
            return reinterpret_cast<uint8_t*>( &m_values[0] );
        }

        void SetReadBuffer( uint8_t * readBuffer, size_t bufferSize )
        {
            int numValues = bufferSize / sizeof( StreamValue );
//            cout << "numValues = " << numValues << endl;
            StreamValue * streamValues = reinterpret_cast<StreamValue*>( readBuffer );
            m_values = vector<StreamValue>( streamValues, streamValues + numValues );
            m_readIndex = 0;
            /*
            cout << "-----------------------------" << endl;
            cout << "read values:" << endl;
            for ( int i = 0; i < m_values.size(); ++i )
            {
                cout << " + value = " << m_values[i].value << " min = " << m_values[i].min << " max = " << m_values[i].max << endl;
            }
            cout << "-----------------------------" << endl;
            */
        }

    private:

        StreamMode m_mode;
        size_t m_readIndex;
        vector<StreamValue> m_values;
    };

    class Object
    {  
    public:
        virtual ~Object() {}
        virtual void Serialize( Stream & stream ) = 0;
    };

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
            Clear();
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

        explicit Address( const sockaddr_storage & addr )
        {
            if ( addr.ss_family == AF_INET )
            {
                const sockaddr_in & addr_ipv4 = reinterpret_cast<const sockaddr_in&>( addr );
                type = AddressType::IPv4;
                address4 = addr_ipv4.sin_addr.s_addr;
                port = ntohs( addr_ipv4.sin_port );
            }
            else if ( addr.ss_family == AF_INET6 )
            {
                const sockaddr_in6 & addr_ipv6 = reinterpret_cast<const sockaddr_in6&>( addr );
                type = AddressType::IPv6;
                memcpy( address6, &addr_ipv6.sin6_addr, 16 );
                port = ntohs( addr_ipv6.sin6_port );
            }
            else
            {
                throw runtime_error( "unknown address family" );
            }
        }

        explicit Address( const sockaddr_in6 & addr_ipv6 )
        {
            type = AddressType::IPv6;
            memcpy( address6, &addr_ipv6.sin6_addr, 16 );
            port = ntohs( addr_ipv6.sin6_port );
        }

        explicit Address( addrinfo * p )
        {
            port = 0;
            if ( p->ai_family == AF_INET )
            { 
                type = AddressType::IPv4;
                struct sockaddr_in * ipv4 = (struct sockaddr_in *)p->ai_addr;
                address4 = ipv4->sin_addr.s_addr;
                port = ntohs( ipv4->sin_port );
            } 
            else if ( p->ai_family == AF_INET6 )
            { 
                type = AddressType::IPv6;
                struct sockaddr_in6 * ipv6 = (struct sockaddr_in6 *)p->ai_addr;
                memcpy( address6, &ipv6->sin6_addr, 16 );
                port = ntohs( ipv6->sin6_port );
            }
            else
            {
                Clear();
            }
        }

        Address( string address )
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
                Clear();
            }
        }

        void Clear()
        {
            type = AddressType::Undefined;
            memset( address6, 0, sizeof( address6 ) );
            port = 0;
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

        string ToString() const
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
        void SetAddress( const Address & _address ) { address = _address; }
        const Address & GetAddress() const { return address; }
    };

    typedef queue<shared_ptr<Packet>> PacketQueue;

    template<typename T> void serialize_int( Stream & stream, T & value, int64_t min, int64_t max )
    {                        
        int64_t int64_value = (int64_t) value;
        stream.SerializeValue( int64_value, min, max );
        value = (T) int64_value;
    }

    void serialize_object( Stream & stream, Object & object )
    {                        
        object.Serialize( stream );
    }

    template <typename T> class Factory
    {
    public:

        typedef function< shared_ptr<T>() > create_function;

        Factory()
        {
            max_type = 0;
        }

        void Register( int type, create_function const & function )
        {
            create_map[type] = function;
            max_type = max( type, max_type );
        }

        shared_ptr<T> Create( int type )
        {
            auto itor = create_map.find( type );
            if ( itor == create_map.end() )
                throw runtime_error( "invalid object type id in factory create" );
            else
                return itor->second();
        }

        int GetMaxType() const
        {
            return max_type;
        }

    private:

        int max_type;
        map<int,create_function> create_map;
    };

    struct ResolveResult
    {
        vector<Address> addresses;
    };

    enum class ResolveStatus
    {
        InProgress,
        Succeeded,
        Failed
    };

    typedef function< void( const string & name, shared_ptr<ResolveResult> result ) > ResolveCallback;

    struct ResolveEntry
    {
        ResolveStatus status;
        shared_ptr<ResolveResult> result;
        future<shared_ptr<ResolveResult>> future;
        vector<ResolveCallback> callbacks;
    };

    class Resolver
    {
    public:

        virtual void Resolve( const string & name, ResolveCallback cb = nullptr ) = 0;

        virtual void Update() = 0;

        virtual void Clear() = 0;

        virtual shared_ptr<ResolveEntry> GetEntry( const string & name ) = 0;
    };

    static shared_ptr<ResolveResult> DNSResolve_Blocking( const string & name, int family = AF_UNSPEC, int socktype = SOCK_DGRAM )
    {
        struct addrinfo hints, *res, *p;
        memset( &hints, 0, sizeof hints );
        hints.ai_family = family;
        hints.ai_socktype = socktype;

        const char * hostname = name.c_str();

        if ( getaddrinfo( hostname, nullptr, &hints, &res ) != 0 )
            return nullptr;

        auto result = make_shared<ResolveResult>();

        for ( p = res; p != nullptr; p = p->ai_next )
        {
            auto address = Address( p );
            if ( address.IsValid() )
                result->addresses.push_back( address );
        }

        freeaddrinfo( res );

        if ( result->addresses.size() == 0 )
            return nullptr;

        return result;
    }

    typedef map<string,shared_ptr<ResolveEntry>> ResolveMap;

    class DNSResolver : public Resolver
    {
    public:

        DNSResolver( int family = AF_UNSPEC, int socktype = SOCK_DGRAM )
        {
            m_family = family;
            m_socktype = socktype;
        }

        virtual void Resolve( const string & name, ResolveCallback callback )
        {
            auto itor = map.find( name );
            if ( itor != map.end() )
            {
                auto name = itor->first;
                auto entry = itor->second;
                switch ( entry->status )
                {
                    case ResolveStatus::InProgress:
                        if ( callback )
                            entry->callbacks.push_back( callback );
                        break;

                    case ResolveStatus::Succeeded:
                    case ResolveStatus::Failed:             // note: result is nullptr if resolve failed
                        if ( callback )
                            callback( name, entry->result );      
                        break;
                }
                return;
            }

            auto entry = make_shared<ResolveEntry>();
            entry->status = ResolveStatus::InProgress;
            if ( callback != nullptr )
                entry->callbacks.push_back( callback );
            const int family = m_family;
            const int socktype = m_socktype;
            entry->future = async( launch::async, [name, family, socktype] () -> shared_ptr<ResolveResult> 
            { 
                return DNSResolve_Blocking( name, family, socktype );
            } );

            map[name] = entry;

            in_progress[name] = entry;
        }

        virtual void Update()
        {
            for ( auto itor = in_progress.begin(); itor != in_progress.end(); )
            {
                auto name = itor->first;
                auto entry = itor->second;

                if ( entry->future.wait_for( chrono::seconds(0) ) == future_status::ready )
                {
                    entry->result = entry->future.get();
                    entry->status = entry->result ? ResolveStatus::Succeeded : ResolveStatus::Failed;
                    for ( auto callback : entry->callbacks )
                        callback( name, entry->result );
                    in_progress.erase( itor++ );
                }
                else
                    ++itor;
            }
        }

        virtual void Clear()
        {
            map.clear();
        }

        virtual shared_ptr<ResolveEntry> GetEntry( const string & name )
        {
            auto itor = map.find( name );
            if ( itor != map.end() )
                return itor->second;
            else
                return nullptr;
        }

    private:

        int m_family;
        int m_socktype;
        ResolveMap map;
        ResolveMap in_progress;
    };

    class Interface
    {
    public:

        virtual ~Interface() {}

        virtual void SendPacket( const Address & address, shared_ptr<Packet> packet ) = 0;

        virtual void SendPacket( const string & hostname, uint16_t port, shared_ptr<Packet> packet ) = 0;

        virtual shared_ptr<Packet> ReceivePacket() = 0;

        virtual void Update() = 0;
    };

    class NetworkInterface : public Interface
    {
    public:

        struct Config
        {
            Config()
            {
                port = 10000;
                family = AF_INET;                   // default to IPv6 for now. may change this.
                maxPacketSize = 10*1024;
            }

            uint16_t port;                          // port to bind UDP socket to
            int family;                             // socket family: eg. AF_INET (IPv4 only), AF_INET6 (IPv6 only)
            int maxPacketSize;                      // maximum packet size
            shared_ptr<Resolver> resolver;          // resolver eg: DNS (optional)
            shared_ptr<Factory<Packet>> factory;    // packet factory (required)
        };

        enum Counters
        {
            PacketsSent,                            // number of packets sent (eg. added to send queue)
            PacketsReceived,                        // number of packets received (eg. added to recv queue)
            PacketsDiscarded,                       // number of packets discarded on send because we couldn't resolve host
            SendToFailures,                         // number of packets lost due to sendto failure
            SerializeWriteFailures,                 // number of serialize write failures
            SerializeReadFailures,                  // number of serialize read failures
            NumCounters
        };

        NetworkInterface( const Config & config = Config() )
            : m_config( config )
        {
            assert( m_config.factory );             // IMPORTANT: You must supply a packet factory!
            assert( m_config.maxPacketSize > 0 );

            m_receiveBuffer.resize( m_config.maxPacketSize );

            cout << "creating network interface on port " << m_config.port << endl;

            m_counters.resize( NumCounters, 0 );

            // create socket

            assert( m_config.family == AF_INET || m_config.family == AF_INET6 );

            m_socket = socket( m_config.family, SOCK_DGRAM, IPPROTO_UDP );

            if ( m_socket <= 0 )
            {
                cout << "socket failed: " << strerror( errno ) << endl;
                throw runtime_error( "network interface failed to create socket" );
            }

            // force IPv6 only if necessary

            if ( m_config.family == AF_INET6 )
            {
                int yes = 1;
                setsockopt( m_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&yes, sizeof(yes) );
            }

            // bind to port

            if ( m_config.family == AF_INET6 )
            {
                sockaddr_in6 sock_address;
                sock_address.sin6_family = AF_INET6;
                sock_address.sin6_addr = in6addr_any;
                sock_address.sin6_port = htons( m_config.port );
            
                if ( ::bind( m_socket, (const sockaddr*) &sock_address, sizeof(sock_address) ) < 0 )
                {
                    cout << "bind failed: " << strerror( errno ) << endl;
                    throw runtime_error( "network interface failed to bind socket (ipv6)" );
                }
            }
            else if ( m_config.family == AF_INET )
            {
                sockaddr_in sock_address;
                sock_address.sin_family = AF_INET;
                sock_address.sin_addr.s_addr = INADDR_ANY;
                sock_address.sin_port = htons( m_config.port );
            
                if ( ::bind( m_socket, (const sockaddr*) &sock_address, sizeof(sock_address) ) < 0 )
                {
                    cout << "bind failed: " << strerror( errno ) << endl;
                    throw runtime_error( "network interface failed to bind socket (ipv4)" );
                }
            }

            // set non-blocking io

            #if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
        
                int nonBlocking = 1;
                if ( fcntl( m_socket, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
                    throw runtime_error( "network interface failed to set non-blocking on socket" );
            
            #elif PLATFORM == PLATFORM_WINDOWS
        
                DWORD nonBlocking = 1;
                if ( ioctlsocket( m_socket, FIONBIO, &nonBlocking ) != 0 )
                    throw runtime_error( "network interface failed to set non-blocking on socket" );

            #else

                #error unsupported platform

            #endif
        }

        ~NetworkInterface()
        {
            if ( m_socket != 0 )
            {
                #if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
                close( m_socket );
                #elif PLATFORM == PLATFORM_WINDOWS
                closesocket( m_socket );
                #else
                #error unsupported platform
                #endif
                m_socket = 0;
            }
        }

        void SendPacket( const Address & address, shared_ptr<Packet> packet )
        {
            assert( address.IsValid() );
            packet->SetAddress( address );
            m_send_queue.push( packet );
        }

        void SendPacket( const string & hostname, uint16_t port, shared_ptr<Packet> packet )
        {
            if ( !m_config.resolver )
                throw runtime_error( "cannot resolve address: resolver is null" );

            Address address;
            address.SetPort( port );
            packet->SetAddress( address );

            auto resolveEntry = m_config.resolver->GetEntry( hostname );

            if ( resolveEntry )
            {
                switch ( resolveEntry->status )
                {
                    case ResolveStatus::Succeeded:
                    {
                        assert( resolveEntry->result->addresses.size() >= 1 );
                        Address address = resolveEntry->result->addresses[0];
                        address.SetPort( port );
                        packet->SetAddress( address );
                        m_send_queue.push( packet );
                        m_counters[PacketsSent]++;
                        cout << "resolve succeeded: sending packet to " << address.ToString() << endl;
                    }
                    break;

                    case ResolveStatus::InProgress:
                    {
                        cout << "resolve in progress: buffering packet" << endl;
                        auto resolve_send_queue = m_resolve_send_queues[hostname];
                        assert( resolve_send_queue );
                        resolve_send_queue->push( packet );
                    }
                    break;

                    case ResolveStatus::Failed:
                    {
                        cout << "resolve failed: discarding packet for \"" << hostname << "\"" << endl;
                        m_counters[PacketsDiscarded]++;
                    }
                    break;
                }
            }
            else
            {
                cout << "resolving \"" << hostname << "\": buffering packet" << endl;
                m_config.resolver->Resolve( hostname );
                auto resolve_send_queue = make_shared<PacketQueue>();
                resolve_send_queue->push( packet );
                m_resolve_send_queues[hostname] = resolve_send_queue;
            }
        }

        shared_ptr<Packet> ReceivePacket()
        {
            if ( m_receive_queue.empty() )
                return nullptr;
            auto packet = m_receive_queue.front();
            m_receive_queue.pop();
            return packet;
        }

        void Update()
        {
            if ( !m_config.resolver )
                return;

            m_config.resolver->Update();

            for ( auto itor = m_resolve_send_queues.begin(); itor != m_resolve_send_queues.end(); )
            {
                auto hostname = itor->first;
                auto resolve_send_queue = itor->second;

                auto entry = m_config.resolver->GetEntry( hostname );
                assert( entry );
                if ( entry->status != ResolveStatus::InProgress )
                {
                    if ( entry->status == ResolveStatus::Succeeded )
                    {
                        assert( entry->result->addresses.size() > 0 );
                        auto address = entry->result->addresses[0];
                        cout << format_string( "resolved \"%s\" to %s", hostname.c_str(), address.ToString().c_str() ) << endl;

                        m_counters[PacketsSent] += resolve_send_queue->size();
                        while ( !resolve_send_queue->empty() )
                        {
                            auto packet = resolve_send_queue->front();
                            resolve_send_queue->pop();
                            const uint16_t port = packet->GetAddress().GetPort();
                            address.SetPort( port );
                            cout << "sent buffered packet to " << address.ToString() << endl;
                            packet->SetAddress( address );
                            m_send_queue.push( packet );
                        }
                        m_resolve_send_queues.erase( itor++ );
                    }
                    else if ( entry->status == ResolveStatus::Failed )
                    {
                        cout << "failed to resolve \"" << hostname << "\". discarding " << resolve_send_queue->size() << " packets" << endl;
                        m_counters[PacketsDiscarded] += resolve_send_queue->size();
                        m_resolve_send_queues.erase( itor++ );
                    }
                }
                else
                    ++itor;
            }

            SendPackets();

            ReceivePackets();
        }

        uint64_t GetCounter( int index ) const
        {
            assert( index >= 0 );
            assert( index < NumCounters );
            return m_counters[index];
        }

    private:

        void SendPackets()
        {
            while ( !m_send_queue.empty() )
            {
                auto packet = m_send_queue.front();
                m_send_queue.pop();

                try
                {
                    Stream stream( STREAM_Write );

                    const int maxPacketType = m_config.factory->GetMaxType();
                    int packetType = packet->GetType();
                    serialize_int( stream, packetType, 0, maxPacketType );
                    packet->Serialize( stream );                

                    size_t bufferSize = 0;
                    const uint8_t * writeBuffer = stream.GetWriteBuffer( bufferSize );

                    if ( bufferSize > m_config.maxPacketSize )
                        throw runtime_error( format_string( "packet is larger than max size %llu", m_config.maxPacketSize ) );

                    if ( !SendPacketInternal( packet->GetAddress(), writeBuffer, bufferSize ) )
                        cout << "failed to send packet" << endl;
                }
                catch ( runtime_error & error )
                {
                    cout << "failed to serialize write packet: " << error.what() << endl;
                    m_counters[SerializeWriteFailures]++;
                    continue;
                }
            }
        }

        void ReceivePackets()
        {
            while ( true )
            {
                Address address;
                int received_bytes = ReceivePacketInternal( address, &m_receiveBuffer[0], m_receiveBuffer.size() );
                if ( !received_bytes )
                    break;

                try
                {
                    Stream stream( STREAM_Read );

                    stream.SetReadBuffer( &m_receiveBuffer[0], received_bytes );

                    const int maxPacketType = m_config.factory->GetMaxType();
                    int packetType = 0;
                    serialize_int( stream, packetType, 0, maxPacketType );

                    auto packet = m_config.factory->Create( packetType );

                    if ( !packet )
                        throw runtime_error( "failed to create packet from type" );

                    packet->Serialize( stream );
                    packet->SetAddress( address );

                    m_receive_queue.push( packet );
                }
                catch ( runtime_error & error )
                {
                    cout << "failed to serialize read packet: " << error.what() << endl;
                    m_counters[SerializeReadFailures]++;
                    continue;
                }
            }
        }

        bool SendPacketInternal( const Address & address, const uint8_t * data, size_t bytes )
        {
            assert( m_socket );
            assert( address.IsValid() );
            assert( bytes > 0 );
            assert( bytes <= m_config.maxPacketSize );

            bool result = false;

            if ( address.GetType() == AddressType::IPv6 )
            {
                cout << "sent ipv6 packet" << endl;
                sockaddr_in6 s_addr;
                memset( &s_addr, 0, sizeof(s_addr) );
                s_addr.sin6_family = AF_INET6;
                s_addr.sin6_port = htons( address.GetPort() );
                memcpy( &s_addr.sin6_addr, address.GetAddress6(), sizeof( s_addr.sin6_addr ) );
                const int sent_bytes = sendto( m_socket, (const char*)data, bytes, 0, (sockaddr*)&s_addr, sizeof(sockaddr_in6) );
                result = sent_bytes == bytes;
            }
            else if ( address.GetType() == AddressType::IPv4 )
            {
                cout << "sent ipv4 packet" << endl;
                sockaddr_in s_addr;
                s_addr.sin_family = AF_INET;
                s_addr.sin_addr.s_addr = address.GetAddress4();
                s_addr.sin_port = htons( (unsigned short) address.GetPort() );
                const int sent_bytes = sendto( m_socket, (const char*)data, bytes, 0, (sockaddr*)&s_addr, sizeof(sockaddr_in) );
                result = sent_bytes == bytes;
            }

            if ( !result )
            {
                cout << "sendto failed: " << strerror( errno ) << endl;
                m_counters[SendToFailures]++;
            }

            return result;
        }
    
        int ReceivePacketInternal( Address & sender, void * data, int size )
        {
            assert( data );
            assert( size > 0 );
            assert( m_socket );

            #if PLATFORM == PLATFORM_WINDOWS
            typedef int socklen_t;
            #endif
            
            sockaddr_storage from;
            socklen_t fromLength = sizeof( from );

            int result = recvfrom( m_socket, (char*)data, size, 0, (sockaddr*)&from, &fromLength );

            if ( errno == EAGAIN )
                return 0;

            if ( result < 0 )
            {
                cout << "recvfrom failed: " << strerror( errno ) << endl;
                return 0;
            }

            sender = Address( from );

            assert( result >= 0 );

            return result;
        }

        const Config m_config;

        int m_socket;
        PacketQueue m_send_queue;
        PacketQueue m_receive_queue;
        map<string,shared_ptr<PacketQueue>> m_resolve_send_queues;
        vector<uint64_t> m_counters;
        vector<uint8_t> m_receiveBuffer;
    };

    inline bool InitializeSockets()
    {
        #if PLATFORM == PLATFORM_WINDOWS
        WSADATA WsaData;
        return WSAStartup( MAKEWORD(2,2), &WsaData ) == NO_ERROR;
        #else
        return true;
        #endif
    }

    inline void ShutdownSockets()
    {
        #if PLATFORM == PLATFORM_WINDOWS
        WSACleanup();
        #endif
    }    

    bool sequence_greater_than( uint16_t s1, uint16_t s2 )
    {
        return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) || 
               ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
    }

    bool sequence_less_than( uint16_t s1, uint16_t s2 )
    {
        return sequence_greater_than( s2, s1 );
    }

    template <typename T> class SlidingWindow
    {
    public:

        SlidingWindow( int size )
        {
            assert( size > 0 );
            m_first_entry = true;
            m_sequence = 0;
            m_entries.resize( size );
        }

        void Reset()
        {
            m_first_entry = true;
            m_sequence = 0;
            auto size = m_entries.size();
            assert( size );
            m_entries.clear();
            m_entries.resize( size );
        }

        bool Insert( const T & entry )
        {
            assert( entry.valid );

            if ( m_first_entry )
            {
                m_sequence = entry.sequence;
                m_first_entry = false;
            }
            else if ( sequence_greater_than( entry.sequence, m_sequence ) )
            {
                m_sequence = entry.sequence;
            }
            else if ( sequence_less_than( entry.sequence, ( m_sequence - ( m_entries.size() - 1 ) ) ) )
            {
                return false;
            }

            const int index = entry.sequence % m_entries.size();
            assert( index >= 0 );
            assert( index < m_entries.size() );
            m_entries[index] = entry;

            return true;
        }

        T * InsertFast( uint16_t sequence )
        {
            if ( m_first_entry )
            {
                m_sequence = sequence;
                m_first_entry = false;
            }
            else if ( sequence_greater_than( sequence, m_sequence ) )
            {
                m_sequence = sequence;
            }
            else if ( sequence_less_than( sequence, ( m_sequence - ( m_entries.size() - 1 ) ) ) )
            {
                return nullptr;
            }

            const int index = sequence % m_entries.size();
            assert( index >= 0 );
            assert( index < m_entries.size() );
            auto entry = &m_entries[index];
            entry->valid = 1;
            entry->sequence = sequence;
            return entry;
        }

        bool HasSlotAvailable( uint16_t sequence ) const
        {
            const int index = sequence % m_entries.size();
            assert( index >= 0 );
            assert( index < m_entries.size() );
            return !m_entries[index].valid;
        }

        const T * Find( uint16_t sequence ) const
        {
            const int index = sequence % m_entries.size();
            assert( index >= 0 );
            assert( index < m_entries.size() );
            if ( m_entries[index].valid && m_entries[index].sequence == sequence )
                return &m_entries[index];
            else
                return NULL;
        }

        T * Find( uint16_t sequence )
        {
            const int index = sequence % m_entries.size();
            assert( index >= 0 );
            assert( index < m_entries.size() );
            if ( m_entries[index].valid && m_entries[index].sequence == sequence )
                return &m_entries[index];
            else
                return NULL;
        }

        uint16_t GetSequence() const 
        {
            return m_sequence;
        }

    private:

        bool m_first_entry;
        uint16_t m_sequence;
        vector<T> m_entries;
    };

    struct SentPacketData
    {
        SentPacketData()
            : valid(0), acked(0), sequence(0) {}

        SentPacketData( uint16_t _sequence )
            : valid(1), acked(0), sequence( _sequence ) {}

        uint32_t valid : 1;                     // is this packet entry valid?
        uint32_t acked : 1;                     // has packet been acked?
        uint32_t sequence : 16;                 // packet sequence #
    };

    struct ReceivedPacketData
    {
        ReceivedPacketData()
            : valid(0), sequence(0) {}

        ReceivedPacketData( uint16_t _sequence )
            : valid(1), sequence( _sequence ) {}

        uint32_t valid : 1;                     // is this packet entry valid?
        uint32_t sequence : 16;                 // packet sequence #
    };

    typedef SlidingWindow<SentPacketData> SentPackets;
    typedef SlidingWindow<ReceivedPacketData> ReceivedPackets;

    void GenerateAckBits( ReceivedPackets & received_packets, 
                          uint16_t & ack,
                          uint32_t & ack_bits )
    {
        ack = received_packets.GetSequence();
        ack_bits = 0;
        for ( int i = 0; i < 32; ++i )
        {
            uint16_t sequence = ack - i;
            if ( received_packets.Find( sequence ) )
                ack_bits |= ( 1 << i );
        }
    }

    class ChannelData : public Object
    {
        // ...
    };

    class Channel
    {
    public:

        virtual ~Channel()
        {
            // ...
        }

        virtual shared_ptr<ChannelData> CreateData() = 0;

        virtual shared_ptr<ChannelData> GetData( uint16_t sequence ) = 0;

        virtual void ProcessData( uint16_t sequence, shared_ptr<ChannelData> data ) = 0;

        virtual void ProcessAck( uint16_t ack ) = 0;

        virtual void Update( const TimeBase & timeBase ) = 0;
    };

    class ConnectionInterface
    {
    public:

        virtual int GetNumChannels() = 0;

        virtual shared_ptr<ChannelData> CreateChannelData( int index ) = 0;
    };

    struct ConnectionPacket : public Packet
    {
        uint16_t sequence;
        uint16_t ack;
        uint32_t ack_bits;
        shared_ptr<ConnectionInterface> interface;
        vector<shared_ptr<ChannelData>> channelData;

        ConnectionPacket( shared_ptr<ConnectionInterface> _interface, int type ) : Packet( type )
        {
            assert( _interface );
            interface = _interface;
            sequence = 0;
            ack = 0;
            ack_bits = 0;
            channelData.resize( interface->GetNumChannels() );
        }

        void Serialize( Stream & stream )
        {
            serialize_int( stream, sequence, 0, 65535 );
            serialize_int( stream, ack, 0, 65535 );
            serialize_int( stream, ack_bits, 0, 0xFFFFFFFF );

            if ( stream.IsWriting() )
            {
                for ( auto data : channelData )
                {
                    int has_data = data != nullptr;
                    serialize_int( stream, has_data, 0, 1 );
                    if ( data )
                        data->Serialize( stream );
                }
            }
            else                
            {
                for ( int i = 0; i < channelData.size(); ++i )
                {
                    int has_data = 0;
                    serialize_int( stream, has_data, 0, 1 );
                    if ( has_data )
                    {
                        channelData[i] = interface->CreateChannelData( i );
                        
                        if ( !channelData[i] )
                            throw runtime_error( format_string( "serialize read failed to create channel data [%d]", i ) );

                        channelData[i]->Serialize( stream );
                    }
                }
            }
        }

        bool operator ==( const ConnectionPacket & other ) const
        {
            return sequence == other.sequence &&
                        ack == other.ack &&
                   ack_bits == other.ack_bits;
        }

        bool operator !=( const ConnectionPacket & other ) const
        {
            return !( *this == other );
        }
    };

    class Connection
    {
    public:

        struct Config
        {
            Config()
            {
                packetType = 0;
                maxPacketSize = 1024;
                slidingWindowSize = 256;
            }

            int packetType;
            int maxPacketSize;
            int slidingWindowSize;
        };

    private:

        struct InterfaceImplementation : public ConnectionInterface
        {
            vector<shared_ptr<Channel>> channels;

            int GetNumChannels() 
            { 
                return channels.size();
            }

            shared_ptr<ChannelData> CreateChannelData( int index )
            {
                assert( index >= 0 );
                assert( index < channels.size() );
                if ( index >= 0 || index < channels.size() )
                    return channels[index]->CreateData();
                else
                    return nullptr;
            }
        };

        const Config m_config;                              // const configuration data
        TimeBase m_timeBase;                                // network time base
        shared_ptr<SentPackets> m_sentPackets;              // sliding window of recently sent packets
        shared_ptr<ReceivedPackets> m_receivedPackets;      // sliding window of recently received packets
        vector<uint64_t> m_counters;                        // counters for unit testing, stats etc.
        shared_ptr<InterfaceImplementation> m_interface;    // connection interface shared with connection packet.

    public:

        enum Counters
        {
            PacketsRead,                            // number of packets read
            PacketsWritten,                         // number of packets written
            PacketsDiscarded,                       // number of packets discarded
            PacketsAcked,                           // number of packets acked
            ReadPacketFailures,                     // number of read packet failures
            NumCounters
        };

        shared_ptr<ConnectionInterface> GetInterface()
        {
            assert( m_interface );
            return m_interface;
        }

        Connection( const Config & config )
            : m_config( config )
        {
            m_sentPackets = make_shared<SentPackets>( m_config.slidingWindowSize );
            m_receivedPackets = make_shared<ReceivedPackets>( m_config.slidingWindowSize );
            m_counters.resize( NumCounters, 0 );
            m_interface = make_shared<InterfaceImplementation>();

            Reset();
        }

        void AddChannel( shared_ptr<Channel> channel )
        {
            assert( channel );
            m_interface->channels.push_back( channel );
        }

        shared_ptr<Channel> GetChannel( int index )
        {
            if ( index >= 0 && index < m_interface->channels.size() )
                return m_interface->channels[index];
            else
                return nullptr;
        }

        void Reset()
        {
            m_sentPackets->Reset();
            m_receivedPackets->Reset();
        }

        void Update( const TimeBase & timeBase )
        {
            m_timeBase = timeBase;
            for ( auto channel : m_interface->channels )
                channel->Update( timeBase );
        }

        const TimeBase & GetTimeBase() const
        {
            return m_timeBase;
        }

        shared_ptr<ConnectionPacket> WritePacket()
        {
            auto packet = make_shared<ConnectionPacket>( GetInterface(), m_config.packetType );

            packet->sequence = m_sentPackets->GetSequence() + 1;

            GenerateAckBits( *m_receivedPackets, packet->ack, packet->ack_bits );

            for ( int i = 0; i < m_interface->channels.size(); ++i )
                packet->channelData[i] = m_interface->channels[i]->GetData( packet->sequence );

            m_sentPackets->Insert( packet->sequence );

            m_counters[PacketsWritten]++;

            return packet;
        }

        void ReadPacket( shared_ptr<ConnectionPacket> packet )
        {
            assert( packet );
            assert( packet->GetType() == m_config.packetType );
            assert( packet->channelData.size() == m_interface->channels.size() );

//            cout << "read packet " << packet->sequence << endl;

            m_counters[PacketsRead]++;

            try
            {
                for ( int i = 0; i < packet->channelData.size(); ++i )
                {
                    if ( packet->channelData[i] )
                        m_interface->channels[i]->ProcessData( packet->sequence, packet->channelData[i] );
                }
            }
            catch ( runtime_error & e )
            {
                m_counters[ReadPacketFailures]++;
                return;                
            }

            if ( !m_receivedPackets->Insert( packet->sequence ) )
            {
                m_counters[PacketsDiscarded]++;
                return;
            }

            ProcessAcks( packet->ack, packet->ack_bits );
        }

        uint64_t GetCounter( int index ) const
        {
            assert( index >= 0 );
            assert( index < NumCounters );
            return m_counters[index];
        }

    private:

        void ProcessAcks( uint16_t ack, uint32_t ack_bits )
        {
            for ( int i = 0; i < 32; ++i )
            {
                if ( ack_bits & 1 )
                {                    
                    const uint16_t sequence = ack - i;
                    SentPacketData * packetData = m_sentPackets->Find( sequence );
                    if ( packetData && !packetData->acked )
                    {
                        PacketAcked( sequence );
                        packetData->acked = 1;
                    }
                }
                ack_bits >>= 1;
            }
        }

        void PacketAcked( uint16_t sequence )
        {
//            cout << "packet " << sequence << " acked" << endl;

            m_counters[PacketsAcked]++;
            for ( auto channel : m_interface->channels )
                channel->ProcessAck( sequence );
        }
    };

    class Message : public Object
    {
    public:

        Message( int type ) : m_type(type) {}

        int GetId() const {  return m_id; }
        int GetType() const { return m_type; }

        void SetId( uint16_t id ) { m_id = id; }

    private:
      
        uint16_t m_id;  
        uint16_t m_type;       
    };

    typedef vector<uint8_t> Block;

    class BlockMessage : public Message
    {
    public:

        BlockMessage() : Message(0) {}

        BlockMessage( shared_ptr<Block> block ) : Message(0) { m_block = block; }

        virtual void Serialize( Stream & stream ) { assert(false); }

        shared_ptr<Block> GetBlock() { return m_block; }

    private:

        shared_ptr<Block> m_block;
    };

    struct MessageChannelConfig
    {
        MessageChannelConfig()
        {
            factory = nullptr;
            resendRate = 0.1f;
            sendQueueSize = 1024;
            receiveQueueSize = 1024;
            sentPacketsSize = 256;
            maxMessagesPerPacket = 32;
        }

        shared_ptr<Factory<Message>> factory;
        float resendRate;
        int sendQueueSize;
        int receiveQueueSize;
        int maxMessagesPerPacket;
        int sentPacketsSize;
    };

    class MessageChannelData : public ChannelData
    {
        const MessageChannelConfig * config;

    public:

        MessageChannelData( const MessageChannelConfig & _config ) : config( &_config ) {}

        vector<shared_ptr<Message>> messages;

        void Serialize( Stream & stream )
        {
            assert( config );
            assert( config->factory );

            int numMessages = stream.IsWriting() ? messages.size() : 0;
            serialize_int( stream, numMessages, 0, config->maxMessagesPerPacket );
            if ( stream.IsReading() )
                messages.resize( numMessages );

            for ( int i = 0; i < numMessages; ++i )
            {
                #ifdef _DEBUG
                if ( stream.IsWriting() )
                    assert( messages[i] );
                #endif

                int messageType = stream.IsWriting() ? messages[i]->GetType() : 0;
                serialize_int( stream, messageType, 0, config->factory->GetMaxType() );
                if ( stream.IsReading() )
                {
                    messages[i] = config->factory->Create( messageType );
                    assert( messages[i] );
                    assert( messages[i]->GetType() == messageType );
                }

                uint16_t messageId = stream.IsReading() ? messages[i]->GetId() : 0;
                serialize_int( stream, messageId, 0, 65535 );
                if ( stream.IsReading() )
                {
                    messages[i]->SetId( messageId );
                    assert( messages[i]->GetId() == messageId );
                }

                messages[i]->Serialize( stream );
            }
        }
    };

    class MessageChannel : public Channel
    {
        struct SendQueueEntry
        {
            shared_ptr<Message> message;
            uint32_t sequence : 16;                      // this is actually the message id      
            uint32_t valid : 1;
            double timeLastSent;
            SendQueueEntry()
                : valid(0) {}
            SendQueueEntry( shared_ptr<Message> _message, uint16_t _sequence )
                : message( _message ), sequence( _sequence ), valid(1), timeLastSent(0) {}
        };

        struct SentPacketEntry
        {
            uint32_t sequence : 16;                      // this is the packet sequence #
            uint32_t acked : 1;
            uint32_t valid : 1;
            double timeSent;
            vector<uint16_t> messageIds;
            SentPacketEntry() : valid(0) {}
        };

        struct ReceiveQueueEntry
        {
            shared_ptr<Message> message;
            uint32_t sequence : 16;                      // this is actually the message id      
            uint32_t valid : 1;
            double timeReceived;
            ReceiveQueueEntry()
                : valid(0) {}
            ReceiveQueueEntry( shared_ptr<Message> _message, uint16_t _sequence, double _timeReceived )
                : message( _message ), sequence( _sequence ), valid(1), timeReceived( _timeReceived ) {}
        };

    public:

        enum Counters
        {
            MessagesSent,
            MessagesWritten,
            MessagesRead,
            MessagesReceived,
            MessagesDiscardedLate,
            MessagesDiscardedEarly,
            NumCounters
        };

        MessageChannel( const MessageChannelConfig & config ) : m_config( config )
        {
            assert( config.factory );       // IMPORTANT: You must supply a message factory!

            m_sendMessageId = 0;
            m_receiveMessageId = 0;

            m_sendQueue = make_shared<SlidingWindow<SendQueueEntry>>( m_config.sendQueueSize );
            m_sentPackets = make_shared<SlidingWindow<SentPacketEntry>>( m_config.sentPacketsSize );
            m_receiveQueue = make_shared<SlidingWindow<ReceiveQueueEntry>>( m_config.receiveQueueSize );

            m_counters.resize( NumCounters, 0 );
        }

        bool CanSendMessage() const
        {
            return m_sendQueue->HasSlotAvailable( m_sendMessageId );
        }

        void SendMessage( shared_ptr<Message> message )
        {
            cout << "queue message for send: " << m_sendMessageId << endl;

            if ( !m_sendQueue->HasSlotAvailable( m_sendMessageId ) )
                throw runtime_error( "message send queue overflow" );

            message->SetId( m_sendMessageId );

            cout << "message id is " << message->GetId() << endl;

            m_sendQueue->Insert( SendQueueEntry( message, m_sendMessageId ) );

            #ifdef _DEBUG
            auto entry = m_sendQueue->Find( m_sendMessageId );
            assert( entry );
            assert( entry->valid );
            assert( entry->sequence == m_sendMessageId );
            assert( entry->message );
            assert( entry->message->GetId() == m_sendMessageId );
            #endif

            m_counters[MessagesSent]++;

            m_sendMessageId++;
        }

        shared_ptr<Message> ReceiveMessage()
        {
            auto entry = m_receiveQueue->Find( m_receiveMessageId );
            if ( !entry )
                return nullptr;

            m_counters[MessagesReceived]++;
            m_receiveMessageId++;
            
            auto message = entry->message;

            #ifdef _DEBUG
            assert( message );
            assert( message->GetId() == m_receiveMessageId );
            #endif

            cout << "dequeue for receive: " << message->GetId() << endl;

            entry->valid = 0;
            entry->message = nullptr;
            
            return message;
        }

        shared_ptr<ChannelData> CreateData()
        {
            return make_shared<MessageChannelData>( m_config );
        }

        shared_ptr<ChannelData> GetData( uint16_t sequence )
        {
            // first gather messages to include in the packet

            int numMessageIds = 0;
            uint16_t messageIds[m_config.maxMessagesPerPacket];
            const uint16_t baseId = m_sendMessageId - m_config.sendQueueSize + 1;
            for ( int i = 0; i < m_config.sendQueueSize; ++i )
            {
                uint16_t id = ( baseId + i ) % m_config.sendQueueSize;
                SendQueueEntry * entry = m_sendQueue->Find( id );
                if ( entry && entry->timeLastSent + m_config.resendRate < m_timeBase.time )
                {
                    messageIds[numMessageIds++] = id;
                    entry->timeLastSent = m_timeBase.time;
                }
                if ( numMessageIds == m_config.maxMessagesPerPacket )
                    break;
            }
            assert( numMessageIds >= 0 );
            assert( numMessageIds <= m_config.maxMessagesPerPacket );

            // if there are no messages then we don't have any data to send

            if ( numMessageIds == 0 )
                return nullptr;

            // add sent packet data containing message ids included in this packet

            auto sentPacketData = m_sentPackets->InsertFast( sequence );
            assert( sentPacketData );
            sentPacketData->acked = 0;
            sentPacketData->timeSent = m_timeBase.time;
            sentPacketData->messageIds.resize( numMessageIds );
            for ( int i = 0; i < numMessageIds; ++i )
                sentPacketData->messageIds[i] = messageIds[i];

            // update counter: nem messages written

            m_counters[MessagesWritten] += numMessageIds;

            // construct channel data for packet

            auto data = make_shared<MessageChannelData>( m_config );

            data->messages.resize( numMessageIds );
            for ( int i = 0; i < numMessageIds; ++i )
            {
                auto entry = m_sendQueue->Find( messageIds[i] );
                assert( entry );
                assert( entry->message );
                data->messages[i] = entry->message;
            }

            return data;
        }

        void ProcessData( uint16_t sequence, shared_ptr<ChannelData> channelData )
        {
            assert( channelData );

            auto data = reinterpret_cast<MessageChannelData&>( *channelData );

            cout << "process message channel data: " << sequence << endl;

            bool earlyMessage = false;

            const uint16_t minMessageId = m_receiveMessageId;
            const uint16_t maxMessageId = m_receiveMessageId + m_config.receiveQueueSize - 1;

            // process messages included in this packet data

            for ( auto message : data.messages )
            {
                assert( message );

                const uint16_t messageId = message->GetId();

                if ( sequence_less_than( messageId, minMessageId ) )
                {
                    // If messages are older than the last received message id we can
                    // ignore them. They have already been queued up and delivered to
                    // the application included via some previously processed packet.

                    m_counters[MessagesDiscardedLate]++;
                }
                else if ( sequence_greater_than( messageId, maxMessageId ) )
                {
                    // If a message arrives that is *newer* than what the receive queue 
                    // can buffer we must discard this packet. Otherwise, early messages
                    // that we cannot buffer will be false acked and never resent.

                    earlyMessage = true;

                    m_counters[MessagesDiscardedEarly]++;
                }
                else
                {
                    bool result = m_receiveQueue->Insert( ReceiveQueueEntry( message, messageId, m_timeBase.time ) );
                    assert( result );
                }

                m_counters[MessagesRead]++;
            }

            if ( earlyMessage )
                throw runtime_error( "received early message" );
        }

        void ProcessAck( uint16_t ack )
        {
            cout << "process ack: " << ack << endl;

            auto sentPacket = m_sentPackets->Find( ack );
            if ( !sentPacket || sentPacket->acked )
                return;

            for ( auto messageId : sentPacket->messageIds )
            {
                auto sendQueueEntry = m_sendQueue->Find( messageId );
                if ( sendQueueEntry )
                {
                    assert( sendQueueEntry->message );
                    assert( sendQueueEntry->message->GetId() == messageId );

                    cout << "acked message " << messageId << endl;

                    sendQueueEntry->valid = 0;
                    sendQueueEntry->message = nullptr;
                }
            }

            sentPacket->acked = 1;
        }

        void Update( const TimeBase & timeBase )
        {
            m_timeBase = timeBase;
        }

    private:

        MessageChannelConfig m_config;                                      // constant configuration data
        TimeBase m_timeBase;                                                // current time base from last update
        uint16_t m_sendMessageId;                                           // id for next message added to send queue
        uint16_t m_receiveMessageId;                                        // id for next message to be received
        shared_ptr<SlidingWindow<SendQueueEntry>> m_sendQueue;              // message send queue
        shared_ptr<SlidingWindow<SentPacketEntry>> m_sentPackets;           // sent packets (for acks)
        shared_ptr<SlidingWindow<ReceiveQueueEntry>> m_receiveQueue;        // message receive queue
        vector<uint64_t> m_counters;                                        // counters
    };

} //------------------------------------------------------

using namespace std;
using namespace protocol;

enum PacketType
{
    PACKET_Connection,           // for connection class
    PACKET_Connect,
    PACKET_Update,
    PACKET_Disconnect
};

enum MessageType
{
    MESSAGE_Block = 0,           // IMPORTANT: 0 is reserved for block messages
    MESSAGE_Test
};

struct TestMessage : public Message
{
    TestMessage() : Message( MESSAGE_Test )
    {
        sequence = 0;
    }

    void Serialize( Stream & stream )
    {        
        serialize_int( stream, sequence, 0, 65535 );
    }

    uint16_t sequence;
};

class MessageFactory : public Factory<Message>
{
public:
    MessageFactory()
    {
        Register( MESSAGE_Block, [] { return make_shared<BlockMessage>(); } );
        Register( MESSAGE_Test, [] { return make_shared<TestMessage>(); } );
    }
};

void test_message_channel()
{
    cout << "test_message_channel" << endl;

    Connection::Config connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = 4 * 1024;
    Connection connection( connectionConfig );

    auto messageFactory = make_shared<MessageFactory>();
    MessageChannelConfig channelConfig;
    channelConfig.factory = static_pointer_cast<Factory<Message>>( messageFactory );
    auto channel = make_shared<MessageChannel>( channelConfig );
    connection.AddChannel( channel );

    const int NumMessagesSent = 32;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        auto message = make_shared<TestMessage>();
        message->sequence = i;
        channel->SendMessage( message );
    }

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    int numMessagesReceived = 0;

    for ( int i = 0; i < 100; ++i )
    {  
        auto writePacket = connection.WritePacket();

        Stream stream( STREAM_Write );
        writePacket->Serialize( stream );

        stream.SetMode( STREAM_Read );
        auto readPacket = make_shared<ConnectionPacket>( connection.GetInterface(), PACKET_Connection );
        readPacket->Serialize( stream );

        if ( ( rand() % 10 ) == 0 )
            connection.ReadPacket( readPacket );

        assert( connection.GetCounter( Connection::PacketsRead ) <= i+1 );
        assert( connection.GetCounter( Connection::PacketsWritten ) == i+1 );
        assert( connection.GetCounter( Connection::PacketsDiscarded ) == 0 );
        assert( connection.GetCounter( Connection::PacketsAcked ) <= i+1 );
        assert( connection.GetCounter( Connection::ReadPacketFailures ) == 0 );

        while ( true )
        {
            auto message = channel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceived );
            assert( message->GetType() == MESSAGE_Test );

            auto testMessage = static_pointer_cast<TestMessage>( message );

            cout << "received message " << testMessage->sequence << endl;

            assert( testMessage->sequence == numMessagesReceived );

            ++numMessagesReceived;
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;

        connection.Update( timeBase );

        timeBase.time += timeBase.deltaTime;
    }
}

// -------------------------------------------------------

#if 0

const int MaxBlockSize = 1024;

typedef vector<uint8_t> Block;

struct ReliableBlock : public ChannelData
{
    ReliableBlock( shared_ptr<Block> _block, uint16_t _blockId )
        : blockId( _blockId ), block( _block ) {}

    uint16_t blockId;
    shared_ptr<Block> block;

    void Serialize( Stream & stream )
    {
        serialize_int( stream, blockId, 0, 65535 );

        if ( stream.IsWriting() )
        {  
            assert( block );
            int bytes = block->size();
            assert( bytes > 0 );
            serialize_int( stream, bytes, 1, MaxBlockSize );
            for ( int i = 0; i < bytes; ++i )
                serialize_int( stream, block->at(i), 0, 255 );
        }
        else
        {
            int bytes = 0;
            serialize_int( stream, bytes, 1, MaxBlockSize );
            block = make_shared<Block>( bytes, 0 );
            for ( int i = 0; i < bytes; ++i )
                serialize_int( stream, block->at(i), 0, 255 );
        }
    }
};

struct SentBlockData
{
    SentBlockData()
        : valid(0), sequence(0), blockId(0) {}

    SentBlockData( uint16_t _sequence, uint16_t _blockId )
        : valid(1), sequence( _sequence ), blockId( _blockId ) {}

    uint32_t valid : 1;
    uint32_t sequence : 16;
    uint32_t blockId : 16;
};

typedef SlidingWindow<SentBlockData> SentBlocks;

class ReliableBlockChannel : public Channel
{
public:

    struct Config
    {
        Config()
        {
            resendRate = 0.1f;
            slidingWindowSize = 256;
        }

        float resendRate;
        int slidingWindowSize;
    };

private:
    
    Config m_config;                                            // configuration data for channel
    
    uint16_t m_sendBlockId;                                     // block id for next block sent
    uint16_t m_receiveBlockId;                                  // block id we expect to receive next
    uint16_t m_currentSendBlockId;                              // current block id we are sending until acked

    double m_lastSendTime;                                      // time that we last sent a block (to enforce maximum send rate)

    queue<shared_ptr<ReliableBlock>> m_sendQueue;               // block send queue
    queue<shared_ptr<ReliableBlock>> m_receiveQueue;            // block receive queue

    shared_ptr<SentBlocks> m_sentBlocks;                        // sliding window used to map packet acks to blocks

    TimeBase m_timeBase;                                        // time base from last update

public:

    ReliableBlockChannel( const Config & config = Config() )
    {
        m_config = config;
        m_sendBlockId = 0;
        m_receiveBlockId = 0;
        m_currentSendBlockId = 0xFFFF;
        m_sentBlocks = make_shared<SentBlocks>( m_config.slidingWindowSize );
    }

    void SendBlock( shared_ptr<Block> block )
    {
//        cout << "queue block for send: " << m_send_block_id << endl;
        assert( block->size() > 0 );
        m_sendQueue.push( make_shared<ReliableBlock>( block, m_sendBlockId++ ) );
    }

    shared_ptr<Block> ReceiveBlock()
    {
        if ( m_receiveQueue.empty() )
            return nullptr;
        auto reliableBlock = m_receiveQueue.front();
        m_receiveQueue.pop();
        return reliableBlock->block;
    }

    shared_ptr<ChannelData> CreateData()
    {
        return make_shared<ReliableBlock>( nullptr, 0 );
    }

    shared_ptr<ChannelData> GetDataForPacket( uint16_t sequence )
    {
        if ( m_sendQueue.empty() )
            return nullptr;

        auto reliableBlock = m_sendQueue.front();

        if ( m_currentSendBlockId == reliableBlock->blockId && m_timeBase.time - m_lastSendTime <= m_config.resendRate )
            return nullptr;

        m_currentSendBlockId = reliableBlock->blockId;
        m_lastSendTime = m_timeBase.time;

        AddSentBlockData( sequence, reliableBlock->blockId );

//        cout << format_string( "block %d sent in packet %d", reliable_block->id, sequence ) << endl;

        return reliableBlock;
    }

    void AddSentBlockData( uint16_t sequence, uint16_t blockId )
    {
        m_sentBlocks->Insert( SentBlockData( sequence, blockId ) );
    }

    void ProcessDataFromPacket( shared_ptr<ChannelData> channelData )
    {
        assert( channelData );

        auto reliableBlock = reinterpret_cast<ReliableBlock&>( *channelData );
        
//        cout << "process data: block id = " << data.block_id << endl;

        if ( sequence_less_than( reliableBlock.blockId, m_receiveBlockId ) )
            return;

        if ( sequence_greater_than( reliableBlock.blockId, m_receiveBlockId ) )
            throw runtime_error( "received future block. discarding packet" );

        assert( reliableBlock.blockId == m_receiveBlockId );

        m_receiveQueue.push( make_shared<ReliableBlock>( reliableBlock ) );

        m_receiveBlockId++;
    }

    void ProcessAck( uint16_t ack )
    {
//        cout << "process ack: " << ack << endl;

        if ( m_sendQueue.empty() )
            return;

        auto sentBlock = m_sentBlocks->Find( ack );        
        if ( sentBlock )
        {
//            cout << " -> block " << data->block_id << " sent in packet " << ack << endl;
            if ( sentBlock->blockId == m_sendQueue.front()->blockId )
                m_sendQueue.pop();
        }
        else
        {
//            cout << " -> no block sent in packet " << ack << endl;
        }
    }

    void Update( const TimeBase & timeBase )
    {
        m_timeBase = timeBase;
    }
};

void test_reliable_block_channel()
{
    cout << "test_reliable_block_channel" << endl;

    const int NumBlocksToSend = 10;
    const int BlockSize = 256;

    Connection::Config config;

    config.packetType = PACKET_Connection;
    config.maxPacketSize = 4 * 1024;

    Connection connection( config );

    auto channel = make_shared<ReliableBlockChannel>();

    connection.AddChannel( channel );

    for ( int i = 0; i < NumBlocksToSend; ++i )
    {
        auto block = make_shared<Block>( BlockSize, i );
        channel->SendBlock( block );
    }

    int numBlocksReceived = 0;

    TimeBase timeBase;
    timeBase.deltaTime = 0.1f;

    for ( int i = 0; i < 10000; ++i )
    {  
        auto writePacket = connection.WritePacket();

        Stream stream( STREAM_Write );
        writePacket->Serialize( stream );

        stream.SetMode( STREAM_Read );
        auto readPacket = make_shared<ConnectionPacket>( connection.GetInterface(), PACKET_Connection );
        readPacket->Serialize( stream );

        if ( ( rand() % 10 ) == 0 )
            connection.ReadPacket( readPacket );
        
        while ( true )
        {
            auto block = channel->ReceiveBlock();
            if ( !block )
                break;
            assert( block->size() == BlockSize );
            for ( int i = 0; i < BlockSize; ++i )
                assert( block->at(i) == numBlocksReceived );

//            cout << "block received" << endl;

            ++numBlocksReceived;
        }

        assert( connection.GetCounter( Connection::PacketsRead ) <= i+1 );
        assert( connection.GetCounter( Connection::PacketsWritten ) == i+1 );
        assert( connection.GetCounter( Connection::PacketsDiscarded ) == 0 );
        assert( connection.GetCounter( Connection::PacketsAcked ) <= i+1 );
        assert( connection.GetCounter( Connection::ReadPacketFailures ) == 0 );

        if ( numBlocksReceived == NumBlocksToSend )
            break;

        connection.Update( timeBase );

        timeBase.time += timeBase.deltaTime;
    }

    assert( numBlocksReceived == NumBlocksToSend );
}

#endif

// -------------------------------------------------------

void test_serialize_object();
void test_interface();
void test_factory();
void test_address4();
void test_address6();
void test_dns_resolve();
void test_dns_resolve_failure();
void test_network_interface_send_to_hostname();
void test_network_interface_send_to_hostname_failure();
void test_network_interface_send_and_receive_ipv4();
void test_network_interface_send_and_receive_ipv6();
void test_sequence();
void test_sliding_window();
void test_generate_ack_bits();
void test_connection();

int main()
{
    InitializeSockets();

    try
    {
        /*
        test_serialize_object();
        test_interface();
        test_factory();
        test_address4();
        test_address6();
        test_dns_resolve();
        test_dns_resolve_failure();
        test_network_interface_send_to_hostname();
        test_network_interface_send_to_hostname_failure();
        test_network_interface_send_and_receive_ipv4();
        test_network_interface_send_and_receive_ipv6();
        test_sequence();
        test_sliding_window();
        test_generate_ack_bits();
        test_connection();
        */
        test_message_channel();
        #if 0
        test_reliable_block_channel();
        #endif
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    ShutdownSockets();

    return 0;
}

// --------------------------------------------------------------

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
        serialize_int( stream, a, -10, 10 );
        serialize_int( stream, b, -10, 10 );
        serialize_int( stream, c, -10, 10 );
    }

    bool operator ==( const ConnectPacket & other ) const
    {
        return a == other.a && b == other.b && c == other.c;
    }

    bool operator !=( const ConnectPacket & other ) const
    {
        return !( *this == other );
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

    bool operator ==( const UpdatePacket & other ) const
    {
        return timestamp == other.timestamp;
    }

    bool operator !=( const UpdatePacket & other ) const
    {
        return !( *this == other );
    }
};

struct DisconnectPacket : public Packet
{
    int x;

    DisconnectPacket() : Packet( PACKET_Disconnect ) 
    {
        x = 2;
    }

    void Serialize( Stream & stream )
    {
        serialize_int( stream, x, -100, +100 );
    }

    bool operator ==( const DisconnectPacket & other ) const
    {
        return x == other.x;
    }

    bool operator !=( const DisconnectPacket & other ) const
    {
        return !( *this == other );
    }
};

class PacketFactory : public Factory<Packet>
{
public:
    PacketFactory()
    {
        Register( PACKET_Connect, [] { return make_shared<ConnectPacket>(); } );
        Register( PACKET_Update, [] { return make_shared<UpdatePacket>(); } );
        Register( PACKET_Disconnect, [] { return make_shared<DisconnectPacket>(); } );
    }
};

class TestInterface : public Interface
{
public:

    virtual void SendPacket( const Address & address, shared_ptr<Packet> packet )
    {
        packet->SetAddress( address );
        packet_queue.push( packet );
    }

    virtual void SendPacket( const string & address, uint16_t port, shared_ptr<Packet> packet )
    {
        assert( false );        // not supported
    }

    virtual shared_ptr<Packet> ReceivePacket()
    {
        if ( packet_queue.empty() )
            return nullptr;
        auto packet = packet_queue.front();
        packet_queue.pop();
        return packet;
    }

    virtual void Update()
    {
        // ...
    }

    PacketQueue packet_queue;
};

void test_interface()
{
    cout << "test_interface" << endl;

    TestInterface interface;

    Address address( 127,0,0,1, 1000 );

    interface.SendPacket( address, make_shared<ConnectPacket>() );
    interface.SendPacket( address, make_shared<UpdatePacket>() );
    interface.SendPacket( address, make_shared<DisconnectPacket>() );

    auto connectPacket = interface.ReceivePacket();
    auto updatePacket = interface.ReceivePacket();
    auto disconnectPacket = interface.ReceivePacket();

    assert( connectPacket->GetType() == PACKET_Connect );
    assert( connectPacket->GetAddress() == address );

    assert( updatePacket->GetType() == PACKET_Update );
    assert( updatePacket->GetAddress() == address );

    assert( disconnectPacket->GetType() == PACKET_Disconnect );
    assert( disconnectPacket->GetAddress() == address );

    assert( interface.ReceivePacket() == nullptr );
}

void test_factory()
{
    cout << "test_factory" << endl;

    PacketFactory factory;

    auto connectPacket = factory.Create( PACKET_Connect );
    auto updatePacket = factory.Create( PACKET_Update );
    auto disconnectPacket = factory.Create( PACKET_Disconnect );

    assert( connectPacket->GetType() == PACKET_Connect );
    assert( updatePacket->GetType() == PACKET_Update );
    assert( disconnectPacket->GetType() == PACKET_Disconnect );
}

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

void test_dns_resolve()
{
    cout << "test_dns_resolve" << endl;

    DNSResolver resolver;

    int num_google_success_callbacks = 0;
    int num_google_failure_callbacks = 0;

    string google_hostname( "google.com" );

    cout << "resolving " << google_hostname << endl;

    const int num_google_iterations = 10;

    for ( int i = 0; i < num_google_iterations; ++i )
    {
        resolver.Resolve( google_hostname, [&google_hostname, &num_google_success_callbacks, &num_google_failure_callbacks] ( const string & name, shared_ptr<ResolveResult> result ) 
        { 
            assert( name == google_hostname );
            assert( result );
            if ( result )
                ++num_google_success_callbacks;
            else
                ++num_google_failure_callbacks;
        } );
    }

    auto google_entry = resolver.GetEntry( google_hostname );
    assert( google_entry );
    assert( google_entry->status == ResolveStatus::InProgress );

    auto start = chrono::steady_clock::now();

    double t = 0.0;
    double dt = 0.1f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    for ( int i = 0; i < 50; ++i )
    {
        resolver.Update();

        if ( num_google_success_callbacks == num_google_iterations )
            break;

        this_thread::sleep_for( ms );

        t += dt;
    }

    assert( num_google_success_callbacks == num_google_iterations );
    assert( num_google_failure_callbacks == 0 );

    auto finish = chrono::steady_clock::now();

    auto delta = finish - start;

    cout << chrono::duration<double,milli>( delta ).count() << " ms" << endl;

    google_entry = resolver.GetEntry( google_hostname );
    assert( google_entry );
    assert( google_entry->status == ResolveStatus::Succeeded );

    cout << google_hostname << ":" << endl;
    for ( auto & address : google_entry->result->addresses )
        cout << " + " << address.ToString() << endl;
}

void test_dns_resolve_failure()
{
    cout << "test_dns_resolve_failure" << endl;

    DNSResolver resolver;

    bool resolved = false;

    string garbage_hostname( "aoeusoanthuoaenuhansuhtasthas" );

    cout << "resolving garbage hostname: " << garbage_hostname << endl;

    resolver.Resolve( garbage_hostname, [&resolved, &garbage_hostname] ( const string & name, shared_ptr<ResolveResult> result ) 
    { 
        assert( name == garbage_hostname );
        assert( result == nullptr );
        resolved = true;
    } );

    auto entry = resolver.GetEntry( garbage_hostname );
    assert( entry );
    assert( entry->status == ResolveStatus::InProgress );

    auto start = chrono::steady_clock::now();

    double t = 0.0;
    double dt = 0.1f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    for ( int i = 0; i < 50; ++i )
    {
        resolver.Update();

        if ( resolved )
            break;

        this_thread::sleep_for( ms );

        t += dt;
    }

    auto finish = chrono::steady_clock::now();

    auto delta = finish - start;

    cout << chrono::duration<double,milli>( delta ).count() << " ms" << endl;

    entry = resolver.GetEntry( garbage_hostname );
    assert( entry );
    assert( entry->status == ResolveStatus::Failed );
    assert( entry->result == nullptr );
}

void test_network_interface_send_to_hostname()
{
    cout << "test_network_interface_send_to_hostname" << endl;

    NetworkInterface::Config config;

    auto factory = make_shared<PacketFactory>();

    config.port = 10000;
    config.maxPacketSize = 1024;
    config.factory = static_pointer_cast<Factory<Packet>>( factory );
    config.resolver = make_shared<DNSResolver>();

    NetworkInterface interface( config );

    int numPackets = 10;

    for ( int i = 0; i < numPackets; ++i )
    {
        auto packet = factory->Create( PACKET_Connect );

        interface.SendPacket( "google.com", config.port, packet );
    }

    auto start = chrono::steady_clock::now();

    double t = 0.0;
    double dt = 0.1f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    for ( int i = 0; i < 20; ++i )
    {
        interface.Update();

        if ( interface.GetCounter( NetworkInterface::PacketsSent ) == numPackets )
            break;

        this_thread::sleep_for( ms );

        t += dt;
    }

    auto finish = chrono::steady_clock::now();

    auto delta = finish - start;

    cout << chrono::duration<double,milli>( delta ).count() << " ms" << endl;
}

void test_network_interface_send_to_hostname_failure()
{
    cout << "test_network_interface_send_to_hostname_failure" << endl;

    NetworkInterface::Config config;

    auto factory = make_shared<PacketFactory>();

    config.port = 10000;
    config.maxPacketSize = 1024;
    config.factory = static_pointer_cast<Factory<Packet>>( factory );
    config.resolver = make_shared<DNSResolver>();

    NetworkInterface interface( config );

    int numPackets = 10;

    for ( int i = 0; i < numPackets; ++i )
    {
        auto packet = factory->Create( PACKET_Connect );

        interface.SendPacket( "aoesortuhantuehanthua", config.port, packet );
    }

    auto start = chrono::steady_clock::now();

    double t = 0.0;
    double dt = 0.1f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    for ( int i = 0; i < 20; ++i )
    {
        interface.Update();

        if ( interface.GetCounter( NetworkInterface::PacketsDiscarded ) == numPackets )
            break;

        this_thread::sleep_for( ms );

        t += dt;
    }

    auto finish = chrono::steady_clock::now();

    auto delta = finish - start;

    cout << chrono::duration<double,milli>( delta ).count() << " ms" << endl;
}

void test_network_interface_send_and_receive_ipv4()
{
    cout << "test_network_interface_send_and_receive_ipv4" << endl;

    NetworkInterface::Config config;

    auto factory = make_shared<PacketFactory>();

    config.port = 10000;
    config.maxPacketSize = 1024;
    config.factory = static_pointer_cast<Factory<Packet>>( factory );
    config.resolver = make_shared<DNSResolver>( AF_INET );

    NetworkInterface interface( config );

    Address address( "127.0.0.1" );
    address.SetPort( config.port );

    double dt = 0.01f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    bool receivedConnectPacket = false;
    bool receivedUpdatePacket = false;
    bool receivedDisconnectPacket = false;

    int iterations = 0;

    while ( true )
    {
        assert ( iterations++ < 10 );

        auto connectPacket = make_shared<ConnectPacket>();
        auto updatePacket = make_shared<UpdatePacket>();
        auto disconnectPacket = make_shared<DisconnectPacket>();

        connectPacket->a = 2;
        connectPacket->b = 6;
        connectPacket->c = -1;

        updatePacket->timestamp = 500;

        disconnectPacket->x = -100;

        interface.SendPacket( address, connectPacket );
        interface.SendPacket( address, updatePacket );
        interface.SendPacket( address, disconnectPacket );

        interface.Update();

        this_thread::sleep_for( ms );

        while ( true )
        {
            auto packet = interface.ReceivePacket();
            if ( !packet )
                break;

            assert( packet->GetAddress() == address );

            switch ( packet->GetType() )
            {
                case PACKET_Connect:
                {
                    cout << "received connect packet" << endl;
                    auto recv_connectPacket = static_pointer_cast<ConnectPacket>( packet );
                    assert( *recv_connectPacket == *connectPacket );
                    receivedConnectPacket = true;
                }
                break;

                case PACKET_Update:
                {
                    cout << "received update packet" << endl;
                    auto recv_updatePacket = static_pointer_cast<UpdatePacket>( packet );
                    assert( *recv_updatePacket == *updatePacket );
                    receivedUpdatePacket = true;
                }
                break;

                case PACKET_Disconnect:
                {
                    cout << "received disconnect packet" << endl;
                    auto recv_disconnectPacket = static_pointer_cast<DisconnectPacket>( packet );
                    assert( *recv_disconnectPacket == *disconnectPacket );
                    receivedDisconnectPacket = true;
                }
                break;
            }
        }

        if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
            break;
    }
}

void test_network_interface_send_and_receive_ipv6()
{
    cout << "test_network_interface_send_and_receive_ipv6" << endl;

    NetworkInterface::Config config;

    auto factory = make_shared<PacketFactory>();

    config.port = 10000;
    config.family = AF_INET6;
    config.maxPacketSize = 1024;
    config.factory = static_pointer_cast<Factory<Packet>>( factory );
    config.resolver = make_shared<DNSResolver>( AF_INET6 );

    NetworkInterface interface( config );

    Address address( "::1" );
    address.SetPort( config.port );

    double dt = 0.01f;
    chrono::milliseconds ms( (int) ( dt * 1000 ) );

    bool receivedConnectPacket = false;
    bool receivedUpdatePacket = false;
    bool receivedDisconnectPacket = false;

    int iterations = 0;

    while ( true )
    {
        assert ( iterations++ < 10 );

        auto connectPacket = make_shared<ConnectPacket>();
        auto updatePacket = make_shared<UpdatePacket>();
        auto disconnectPacket = make_shared<DisconnectPacket>();

        connectPacket->a = 2;
        connectPacket->b = 6;
        connectPacket->c = -1;

        updatePacket->timestamp = 500;

        disconnectPacket->x = -100;

        interface.SendPacket( address, connectPacket );
        interface.SendPacket( address, updatePacket );
        interface.SendPacket( address, disconnectPacket );

        interface.Update();

        this_thread::sleep_for( ms );

        while ( true )
        {
            auto packet = interface.ReceivePacket();
            if ( !packet )
                break;

            cout << "receive packet from address " << packet->GetAddress().ToString() << endl;

            assert( packet->GetAddress() == address );

            switch ( packet->GetType() )
            {
                case PACKET_Connect:
                {
                    cout << "received connect packet" << endl;
                    auto recv_connectPacket = static_pointer_cast<ConnectPacket>( packet );
                    assert( *recv_connectPacket == *connectPacket );
                    receivedConnectPacket = true;
                }
                break;

                case PACKET_Update:
                {
                    cout << "received update packet" << endl;
                    auto recv_updatePacket = static_pointer_cast<UpdatePacket>( packet );
                    assert( *recv_updatePacket == *updatePacket );
                    receivedUpdatePacket = true;
                }
                break;

                case PACKET_Disconnect:
                {
                    cout << "received disconnect packet" << endl;
                    auto recv_disconnectPacket = static_pointer_cast<DisconnectPacket>( packet );
                    assert( *recv_disconnectPacket == *disconnectPacket );
                    receivedDisconnectPacket = true;
                }
                break;
            }
        }

        if ( receivedConnectPacket && receivedUpdatePacket && receivedDisconnectPacket )
            break;
    }
}

void test_connection()
{
    cout << "test_connection" << endl;

    Connection::Config config;

    config.packetType = PACKET_Connection;
    config.maxPacketSize = 4 * 1024;

    Connection connection( config );

    const int NumAcks = 100;

    for ( int i = 0; i < NumAcks*2; ++i )
    {
        auto packet = connection.WritePacket();

        connection.ReadPacket( packet );

        if ( connection.GetCounter( Connection::PacketsAcked ) == NumAcks )
            break;
    }

    assert( connection.GetCounter( Connection::PacketsAcked ) == NumAcks );
    assert( connection.GetCounter( Connection::PacketsWritten ) == NumAcks + 1 );
    assert( connection.GetCounter( Connection::PacketsRead ) == NumAcks + 1 );
    assert( connection.GetCounter( Connection::PacketsDiscarded ) == 0 );
    assert( connection.GetCounter( Connection::ReadPacketFailures ) == 0 );
}

void test_sequence()
{
    cout << "test_sequence" << endl;

    assert( sequence_greater_than( 0, 0 ) == false );
    assert( sequence_greater_than( 1, 0 ) == true );
    assert( sequence_greater_than( 0, -1 ) == true );

    assert( sequence_less_than( 0, 0 ) == false );
    assert( sequence_less_than( 0, 1 ) == true );
    assert( sequence_less_than( -1, 0 ) == true );
}

void test_sliding_window()
{
    cout << "test_sliding_window" << endl;

    const int size = 256;

    SlidingWindow<SentPacketData> slidingWindow( size );

    for ( int i = 0; i < size; ++i )
        assert( slidingWindow.Find(i) == nullptr );

    for ( int i = 0; i <= size*4; ++i )
    {
        slidingWindow.Insert( i );
        assert( slidingWindow.GetSequence() == i );
    }

    for ( int i = 0; i <= size; ++i )
    {
        // note: outside bounds of sliding window!
        bool insert_succeeded = slidingWindow.Insert( i );
        assert( !insert_succeeded );
    }    

    int index = size*4;
    for ( int i = 0; i < size; ++i )
    {
        auto entry = slidingWindow.Find( index );
        assert( entry );
        assert( entry->valid );
        assert( entry->sequence == index );
        index--;
    }

    slidingWindow.Reset();

    assert( slidingWindow.GetSequence() == 0 );

    for ( int i = 0; i < size; ++i )
        assert( slidingWindow.Find(i) == nullptr );
}

void test_generate_ack_bits()
{
    cout << "test_generate_ack_bits" << endl;

    const int size = 256;

    SlidingWindow<ReceivedPacketData> received_packets( size );

    uint16_t ack = -1;
    uint32_t ack_bits = -1;

    GenerateAckBits( received_packets, ack, ack_bits );
    assert( ack == 0 );
    assert( ack_bits == 0 );

    for ( int i = 0; i <= size; ++i )
        received_packets.Insert( i );

    GenerateAckBits( received_packets, ack, ack_bits );
    assert( ack == size );
    assert( ack_bits == 0xFFFFFFFF );

    received_packets.Reset();
    uint16_t input_acks[] = { 1, 5, 9, 11 };
    int input_num_acks = sizeof( input_acks ) / sizeof( uint16_t );
    for ( int i = 0; i < input_num_acks; ++i )
        received_packets.Insert( input_acks[i] );

    GenerateAckBits( received_packets, ack, ack_bits );

    assert( ack == 11 );
    assert( ack_bits == ( 1 | (1<<(11-9)) | (1<<(11-5)) | (1<<(11-1)) ) );
}
