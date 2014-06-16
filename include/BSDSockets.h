/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_BSD_SOCKETS_H
#define PROTOCOL_BSD_SOCKETS_H

#include "NetworkInterface.h"

namespace protocol 
{     
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

    struct BSDSocketsConfig
    {
        BSDSocketsConfig()
        {
            port = 10000;
            family = AF_INET6;                      // default to IPv6.
            maxPacketSize = 10*1024;
            packetFactory = nullptr;
        }

        uint16_t port;                              // port to bind UDP socket to
        int family;                                 // socket family: eg. AF_INET (IPv4 only), AF_INET6 (IPv6 only)
        int maxPacketSize;                          // maximum packet size
        Factory<Packet> * packetFactory;            // packet factory (required)
    };

    class BSDSockets : public NetworkInterface
    {
        const BSDSocketsConfig m_config;

        int m_socket;
        BSDSocketError m_error;
        PacketQueue m_send_queue;
        PacketQueue m_receive_queue;
        uint8_t * m_receiveBuffer;
        uint64_t m_counters[BSD_SOCKET_COUNTER_NUM_COUNTERS];

        BSDSockets( BSDSockets & other );
        BSDSockets & operator = ( BSDSockets & other );

    public:

        BSDSockets( const BSDSocketsConfig & config = BSDSocketsConfig() )
            : m_config( config )
        {
            assert( m_config.packetFactory );       // IMPORTANT: You must supply a packet factory!
            assert( m_config.maxPacketSize > 0 );

            m_receiveBuffer = new uint8_t[m_config.maxPacketSize];

            m_error = BSD_SOCKET_ERROR_NONE;

            // create socket

            assert( m_config.family == AF_INET || m_config.family == AF_INET6 );

            m_socket = socket( m_config.family, SOCK_DGRAM, IPPROTO_UDP );

            if ( m_socket <= 0 )
            {
                printf( "create socket failed: %s\n", strerror( errno ) );
                m_error = BSD_SOCKET_ERROR_CREATE_FAILED;
                return;
            }

            // force IPv6 only if necessary

            if ( m_config.family == AF_INET6 )
            {
                int yes = 1;
                if ( setsockopt( m_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&yes, sizeof(yes) ) != 0 )
                {
                    printf( "failed to set ipv6 only sockopt\n" );
                    m_error = BSD_SOCKET_ERROR_SOCKOPT_IPV6_ONLY_FAILED;
                    return;
                }
            }

            // bind to port

            if ( m_config.family == AF_INET6 )
            {
                sockaddr_in6 sock_address;
                sock_address.sin6_family = AF_INET6;
                sock_address.sin6_addr = in6addr_any;
                sock_address.sin6_port = htons( m_config.port );
            
//                printf( "bind ipv6 socket to port %d\n", m_config.port );

                if ( ::bind( m_socket, (const sockaddr*) &sock_address, sizeof(sock_address) ) < 0 )
                {
                    printf( "bind socket failed (ipv6): %s\n", strerror( errno ) );
                    m_socket = BSD_SOCKET_ERROR_BIND_IPV6_FAILED;
                    return;
                }
            }
            else if ( m_config.family == AF_INET )
            {
                sockaddr_in sock_address;
                sock_address.sin_family = AF_INET;
                sock_address.sin_addr.s_addr = INADDR_ANY;
                sock_address.sin_port = htons( m_config.port );
            
//                printf( "bind ipv4 socket to port %d\n", m_config.port );

                if ( ::bind( m_socket, (const sockaddr*) &sock_address, sizeof(sock_address) ) < 0 )
                {
                    printf( "bind socket failed (ipv4): %s\n", strerror( errno ) );
                    m_error = BSD_SOCKET_ERROR_BIND_IPV4_FAILED;
                    return;
                }
            }

            // set non-blocking io

            #if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
        
                int nonBlocking = 1;
                if ( fcntl( m_socket, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
                {
                    printf( "failed to make socket non-blocking\n" );
                    m_error = BSD_SOCKET_ERROR_SET_NON_BLOCKING_FAILED;
                    return;
                }
            
            #elif PLATFORM == PLATFORM_WINDOWS
        
                DWORD nonBlocking = 1;
                if ( ioctlsocket( m_socket, FIONBIO, &nonBlocking ) != 0 )
                {
                    printf( "failed to make socket non-blocking\n" );
                    m_error = BSD_SOCKET_ERROR_SET_NON_BLOCKING_FAILED;
                    return;
                }

            #else

                #error unsupported platform

            #endif
        }

        ~BSDSockets()
        {
            if ( m_receiveBuffer )
            {
                delete [] m_receiveBuffer;
                m_receiveBuffer = nullptr;
            }

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

        bool IsError() const
        {
            return m_error != BSD_SOCKET_ERROR_NONE;
        }

        bool GetError() const
        {
            return m_error;
        }

        void SendPacket( const Address & address, Packet * packet )
        {
            if ( m_error )
                return;
            assert( packet );
            assert( address.IsValid() );
            packet->SetAddress( address );
            m_send_queue.push( packet );
        }

        Packet * ReceivePacket()
        {
            if ( m_error )
                return nullptr;
            if ( m_receive_queue.empty() )
                return nullptr;
            auto packet = m_receive_queue.front();
            m_receive_queue.pop();
            return packet;
        }

        void Update( const TimeBase & timeBase )
        {
            if ( m_error )
                return;

            SendPackets();

            ReceivePackets();
        }

        uint32_t GetMaxPacketSize() const
        {
            return m_config.maxPacketSize;
        }

        uint64_t GetCounter( int index ) const
        {
            assert( index >= 0 );
            assert( index < BSD_SOCKET_COUNTER_NUM_COUNTERS );
            return m_counters[index];
        }

    private:

        void SendPackets()
        {
            while ( !m_send_queue.empty() )
            {
                auto packet = m_send_queue.front();
                m_send_queue.pop();

                uint8_t buffer[m_config.maxPacketSize];

                typedef WriteStream Stream;

                Stream stream( buffer, m_config.maxPacketSize );

                const int maxPacketType = m_config.packetFactory->GetMaxType();
                
                int packetType = packet->GetType();
                
                serialize_int( stream, packetType, 0, maxPacketType );
                
                stream.Align();

                packet->SerializeWrite( stream );                

                stream.Check( 0x51246234 );

                stream.Flush();

                if ( stream.IsOverflow() )
                {
//                    printf( "stream overflow on write\n" );
                    m_counters[BSD_SOCKET_COUNTER_SERIALIZE_WRITE_OVERFLOW]++;
                    delete packet;
                    continue;
                }

                const int bytes = stream.GetBytesWritten();
                const uint8_t * data = stream.GetData();

                assert( bytes <= m_config.maxPacketSize );
                if ( bytes > m_config.maxPacketSize )
                {
                    //printf( "packet is too large to send\n" );
                    m_counters[BSD_SOCKET_COUNTER_PACKET_TOO_LARGE_TO_SEND]++;
                    delete packet;
                    continue;
                }

                SendPacketInternal( packet->GetAddress(), data, bytes );

                delete packet;
            }
        }

        void ReceivePackets()
        {
            while ( true )
            {
                Address address;
                int received_bytes = ReceivePacketInternal( address, m_receiveBuffer, m_config.maxPacketSize );
                if ( !received_bytes )
                    break;

//                printf( "received bytes: %d\n", received_bytes );

                typedef ReadStream Stream;

                Stream stream( m_receiveBuffer, m_config.maxPacketSize );

                // todo: move the protocol id processing here!!!

                const int maxPacketType = m_config.packetFactory->GetMaxType();
                int packetType = 0;
                serialize_int( stream, packetType, 0, maxPacketType );

                stream.Align();

                auto packet = m_config.packetFactory->Create( packetType );
                if ( !packet )
                {
                    printf( "failed to create packet of type %d\n", packetType );
                    m_counters[BSD_SOCKET_COUNTER_CREATE_PACKET_FAILURES]++;
                    continue;
                }

                packet->SerializeRead( stream );

                if ( stream.IsOverflow() )
                {
                    printf( "packet overflow on read\n" );
                    m_counters[BSD_SOCKET_COUNTER_SERIALIZE_READ_OVERFLOW]++;
                    delete packet;
                    continue;
                }

                stream.Check( 0x51246234 );         // if this triggers then the packet was truncated

                packet->SetAddress( address );

                m_receive_queue.push( packet );
            }
        }

        bool SendPacketInternal( const Address & address, const uint8_t * data, size_t bytes )
        {
            assert( m_socket );
            assert( address.IsValid() );
            assert( bytes > 0 );
            assert( bytes <= m_config.maxPacketSize );

//            printf( "send packet internal: address = %s, bytes = %d\n", address.ToString().c_str(), bytes );

            bool result = false;

            m_counters[BSD_SOCKET_COUNTER_PACKETS_SENT]++;

            if ( address.GetType() == ADDRESS_IPV6 )
            {
//                printf( "ipv6 packet\n" );
                sockaddr_in6 s_addr;
                s_addr.sin6_family = AF_INET6;
                s_addr.sin6_port = htons( address.GetPort() );
                memcpy( &s_addr.sin6_addr, address.GetAddress6(), sizeof( s_addr.sin6_addr ) );
                const int sent_bytes = sendto( m_socket, (const char*)data, bytes, 0, (sockaddr*)&s_addr, sizeof(sockaddr_in6) );
                result = sent_bytes == bytes;
            }
            else if ( address.GetType() == ADDRESS_IPV4 )
            {
//                printf( "sent ipv4 packet\n" );
                sockaddr_in s_addr;
                s_addr.sin_family = AF_INET;
                s_addr.sin_addr.s_addr = address.GetAddress4();
                s_addr.sin_port = htons( (unsigned short) address.GetPort() );
                const int sent_bytes = sendto( m_socket, (const char*)data, bytes, 0, (sockaddr*)&s_addr, sizeof(sockaddr_in) );
                result = sent_bytes == bytes;
            }

            if ( !result )
            {
                printf( "sendto failed: %s\n", strerror( errno ) );
                m_counters[BSD_SOCKET_COUNTER_SEND_FAILURES]++;
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

//            printf( "recvfrom result = %d\n", result );

            if ( result <= 0 )
            {
                if ( errno == EAGAIN )
                    return 0;

                printf( "recvfrom failed: %s\n", strerror( errno ) );
                return 0;
            }

            sender = Address( from );

            assert( result >= 0 );

            m_counters[BSD_SOCKET_COUNTER_PACKETS_RECEIVED]++;

            return result;
        }
    };
}

#endif
