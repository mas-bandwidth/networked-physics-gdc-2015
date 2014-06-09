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
        }

        uint16_t port;                              // port to bind UDP socket to
        int family;                                 // socket family: eg. AF_INET (IPv4 only), AF_INET6 (IPv6 only)
        int maxPacketSize;                          // maximum packet size
        shared_ptr<Factory<Packet>> packetFactory;  // packet factory (required)
    };

    class BSDSockets : public NetworkInterface
    {
        const BSDSocketsConfig m_config;

        int m_socket;
        PacketQueue m_send_queue;
        PacketQueue m_receive_queue;
        vector<uint64_t> m_counters;
        vector<uint8_t> m_receiveBuffer;

    public:

        enum Counters
        {
            PacketsSent,                            // number of packets sent (eg. added to send queue)
            PacketsReceived,                        // number of packets received (eg. added to recv queue)
            SendFailures,                           // number of packets lost due to sendto failure
            SerializeWriteFailures,                 // number of serialize write failures
            SerializeReadFailures,                  // number of serialize read failures
            NumCounters
        };

        BSDSockets( const BSDSocketsConfig & config = BSDSocketsConfig() )
            : m_config( config )
        {
            assert( m_config.packetFactory );       // IMPORTANT: You must supply a packet factory!
            assert( m_config.maxPacketSize > 0 );

            m_receiveBuffer.resize( m_config.maxPacketSize );

//            cout << "creating bsd sockets interface on port " << m_config.port << endl;

            m_counters.resize( NumCounters, 0 );

            // create socket

            assert( m_config.family == AF_INET || m_config.family == AF_INET6 );

            m_socket = socket( m_config.family, SOCK_DGRAM, IPPROTO_UDP );

            if ( m_socket <= 0 )
            {
                cout << "create socket failed: " << strerror( errno ) << endl;
                throw runtime_error( "bsd sockets interface failed to create socket" );
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
            
//                cout << "bind ipv6 socket to port " << m_config.port << endl;

                if ( ::bind( m_socket, (const sockaddr*) &sock_address, sizeof(sock_address) ) < 0 )
                {
                    cout << "bind socket failed: " << strerror( errno ) << endl;
                    throw runtime_error( "bsd sockets interface failed to bind socket (ipv6)" );
                }
            }
            else if ( m_config.family == AF_INET )
            {
                sockaddr_in sock_address;
                sock_address.sin_family = AF_INET;
                sock_address.sin_addr.s_addr = INADDR_ANY;
                sock_address.sin_port = htons( m_config.port );
            
//                cout << "bind ipv4 socket to port " << m_config.port << endl;

                if ( ::bind( m_socket, (const sockaddr*) &sock_address, sizeof(sock_address) ) < 0 )
                {
                    cout << "bind socket failed: " << strerror( errno ) << endl;
                    throw runtime_error( "bsd sockets interface failed to bind socket (ipv4)" );
                }
            }

            // set non-blocking io

            #if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
        
                int nonBlocking = 1;
                if ( fcntl( m_socket, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
                    throw runtime_error( "bsd sockets interface failed to set non-blocking on socket" );
            
            #elif PLATFORM == PLATFORM_WINDOWS
        
                DWORD nonBlocking = 1;
                if ( ioctlsocket( m_socket, FIONBIO, &nonBlocking ) != 0 )
                    throw runtime_error( "bsd sockets interface failed to set non-blocking on socket" );

            #else

                #error unsupported platform

            #endif
        }

        ~BSDSockets()
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

        shared_ptr<Packet> ReceivePacket()
        {
            if ( m_receive_queue.empty() )
                return nullptr;
            auto packet = m_receive_queue.front();
            m_receive_queue.pop();
            return packet;
        }

        void Update( const TimeBase & timeBase )
        {
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
                    uint8_t buffer[m_config.maxPacketSize];

                    Stream stream( STREAM_Write, buffer, m_config.maxPacketSize );

                    const int maxPacketType = m_config.packetFactory->GetMaxType();
                    
                    int packetType = packet->GetType();
                    
                    serialize_int( stream, packetType, 0, maxPacketType );
                    
                    stream.Align();

                    packet->Serialize( stream );                

                    stream.Flush();

                    const int bytes = stream.GetBytes();
                    const uint8_t * data = stream.GetData();

                    if ( bytes > m_config.maxPacketSize )
                        throw runtime_error( format_string( "packet is larger than max size %llu", m_config.maxPacketSize ) );

                    if ( !SendPacketInternal( packet->GetAddress(), data, bytes ) )
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

//                cout << "received bytes: " << received_bytes << endl;

                try
                {
                    Stream stream( STREAM_Read, &m_receiveBuffer[0], m_receiveBuffer.size() );

                    const int maxPacketType = m_config.packetFactory->GetMaxType();
                    int packetType = 0;
                    serialize_int( stream, packetType, 0, maxPacketType );

                    stream.Align();

                    auto packet = m_config.packetFactory->Create( packetType );
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

//            cout << "send packet internal: address = " << address.ToString() << ", bytes = " << bytes << endl;

            bool result = false;

            m_counters[PacketsSent]++;

            if ( address.GetType() == AddressType::IPv6 )
            {
//                cout << "ipv6 packet" << endl;
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
//                cout << "sent ipv4 packet" << endl;
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
                m_counters[SendFailures]++;
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

//            cout << "recvfrom result = " << result;

            if ( result <= 0 )
            {
                if ( errno == EAGAIN )
                    return 0;

                cout << "recvfrom failed: " << strerror( errno ) << endl;
                return 0;
            }

            sender = Address( from );

            assert( result >= 0 );

            m_counters[PacketsReceived]++;

            return result;
        }
    };
}

#endif
