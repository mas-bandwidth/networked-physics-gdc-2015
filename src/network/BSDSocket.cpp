// Network Library - Copyright (c) 2008-2015, The Network Protocol Company, Inc.

#include "network/Network.h"
#include "network/BSDSocket.h"
#include "core/Memory.h"
#include "core/Queue.h"
#include <string.h>

#if CORE_PLATFORM == CORE_PLATFORM_WINDOWS

    #include <winsock2.h>
    #pragma comment( lib, "wsock32.lib" )

#elif CORE_PLATFORM == CORE_PLATFORM_MAC || CORE_PLATFORM == CORE_PLATFORM_UNIX

    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    
#else

    #error unknown platform!

#endif

namespace network
{     
    BSDSocket::BSDSocket( const BSDSocketConfig & config )
        : m_config( config ), 
          m_send_queue( config.allocator ? *config.allocator : core::memory::default_allocator() ),
          m_receive_queue( config.allocator ? *config.allocator : core::memory::default_allocator() )
    {
        CORE_ASSERT( IsNetworkInitialized() );

        CORE_ASSERT( m_config.packetFactory );       // IMPORTANT: You must supply a packet factory!
        CORE_ASSERT( m_config.maxPacketSize > 0 );

        m_allocator = m_config.allocator ? m_config.allocator : &core::memory::default_allocator();

        CORE_ASSERT( m_allocator );

        core::queue::reserve( m_send_queue, m_config.sendQueueSize );
        core::queue::reserve( m_receive_queue, m_config.receiveQueueSize );

        m_receiveBuffer = (uint8_t*) m_allocator->Allocate( m_config.maxPacketSize );

        m_error = BSD_SOCKET_ERROR_NONE;

        m_context = nullptr;

        // create socket

        m_socket = socket( m_config.ipv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP );

        if ( m_socket <= 0 )
        {
            printf( "create socket failed: %s\n", strerror( errno ) );
            m_error = BSD_SOCKET_ERROR_CREATE_FAILED;
            return;
        }

        // force IPv6 only if necessary

        if ( m_config.ipv6 )
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

        if ( m_config.ipv6 )
        {
            sockaddr_in6 sock_address;
            memset( &sock_address, 0, sizeof( sockaddr_in6 ) );
            sock_address.sin6_family = AF_INET6;
            sock_address.sin6_addr = in6addr_any;
            sock_address.sin6_port = htons( m_config.port );

            if ( ::bind( m_socket, (const sockaddr*) &sock_address, sizeof(sock_address) ) < 0 )
            {
                printf( "bind socket failed (ipv6) - %s\n", strerror( errno ) );
                m_socket = BSD_SOCKET_ERROR_BIND_IPV6_FAILED;
                return;
            }
        }
        else
        {
            sockaddr_in sock_address;
            sock_address.sin_family = AF_INET;
            sock_address.sin_addr.s_addr = INADDR_ANY;
            sock_address.sin_port = htons( m_config.port );

            if ( ::bind( m_socket, (const sockaddr*) &sock_address, sizeof(sock_address) ) < 0 )
            {
                printf( "bind socket failed (ipv4) - %s\n", strerror( errno ) );
                m_error = BSD_SOCKET_ERROR_BIND_IPV4_FAILED;
                return;
            }
        }

        // todo: get the actual port we bound to in the case of passing in port 0 we must ask the OS

        m_port = m_config.port;

        // set non-blocking io

        #if CORE_PLATFORM == CORE_PLATFORM_MAC || CORE_PLATFORM == CORE_PLATFORM_UNIX
    
            int nonBlocking = 1;
            if ( fcntl( m_socket, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
            {
                printf( "failed to make socket non-blocking\n" );
                m_error = BSD_SOCKET_ERROR_SET_NON_BLOCKING_FAILED;
                return;
            }
        
        #elif CORE_PLATFORM == CORE_PLATFORM_WINDOWS
    
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

    BSDSocket::~BSDSocket()
    {
        if ( m_receiveBuffer )
        {
            m_allocator->Free( m_receiveBuffer );
            m_receiveBuffer = nullptr;
        }

        if ( m_socket != 0 )
        {
            #if CORE_PLATFORM == CORE_PLATFORM_MAC || CORE_PLATFORM == CORE_PLATFORM_UNIX
            close( m_socket );
            #elif CORE_PLATFORM == CORE_PLATFORM_WINDOWS
            closesocket( m_socket );
            #else
            #error unsupported platform
            #endif
            m_socket = 0;
        }

        for ( int i = 0; i < core::queue::size( m_send_queue ); ++i )
        {
            auto packet = m_send_queue[i];
            CORE_ASSERT( packet );
            m_config.packetFactory->Destroy( packet );
        }

        for ( int i = 0; i < core::queue::size( m_receive_queue ); ++i )
        {
            auto packet = m_receive_queue[i];
            CORE_ASSERT( packet );
            m_config.packetFactory->Destroy( packet );
        }

        core::queue::clear( m_send_queue );
        core::queue::clear( m_receive_queue );
    }

    bool BSDSocket::IsError() const
    {
        return m_error != BSD_SOCKET_ERROR_NONE;
    }

    bool BSDSocket::GetError() const
    {
        return m_error;
    }

    void BSDSocket::SendPacket( const Address & address, protocol::Packet * packet )
    {
        if ( m_error )
        {
            m_config.packetFactory->Destroy( packet );
            return;
        }

        CORE_ASSERT( packet );
        CORE_ASSERT( address.IsValid() );
        
        packet->SetAddress( address );

        if ( core::queue::size( m_send_queue ) == m_config.sendQueueSize )
        {
            m_config.packetFactory->Destroy( packet );
            return;
        }

        core::queue::push_back( m_send_queue, packet );
    }

    protocol::Packet * BSDSocket::ReceivePacket()
    {
        if ( m_error )
            return nullptr;

        if ( core::queue::size( m_receive_queue ) == 0 )
            return nullptr;

        auto packet = m_receive_queue[0];

        core::queue::consume( m_receive_queue, 1 );

        return packet;
    }

    void BSDSocket::Update( const core::TimeBase & timeBase )
    {
        if ( m_error )
            return;

        SendPackets();

        ReceivePackets();
    }

    uint32_t BSDSocket::GetMaxPacketSize() const
    {
        return m_config.maxPacketSize;
    }

    protocol::PacketFactory & BSDSocket::GetPacketFactory() const
    {
        CORE_ASSERT( m_config.packetFactory );
        return *m_config.packetFactory;
    }

    void BSDSocket::SetContext( const void ** context )
    {
        m_context = context;
    }

    uint64_t BSDSocket::GetCounter( int index ) const
    {
        CORE_ASSERT( index >= 0 );
        CORE_ASSERT( index < BSD_SOCKET_COUNTER_NUM_COUNTERS );
        return m_counters[index];
    }

    uint16_t BSDSocket::GetPort() const
    {
        return m_port;
    }

    void BSDSocket::SendPackets()
    {
        while ( core::queue::size( m_send_queue ) )
        {
            auto packet = m_send_queue[0];

            core::queue::consume( m_send_queue, 1 );

            uint8_t buffer[m_config.maxPacketSize];

            typedef protocol::WriteStream Stream;

            Stream stream( buffer, m_config.maxPacketSize );

            stream.SetContext( m_context );

            uint64_t protocolId = m_config.protocolId;
            serialize_uint64( stream, protocolId );

            const int maxPacketType = m_config.packetFactory->GetNumTypes() - 1;
            
            int packetType = packet->GetType();
            
            serialize_int( stream, packetType, 0, maxPacketType );
            
            stream.Align();

            packet->SerializeWrite( stream );

            stream.Check( 0x51246234 );

            stream.Flush();

            CORE_ASSERT( !stream.IsOverflow() );

            if ( stream.IsOverflow() )
            {
                m_counters[BSD_SOCKET_COUNTER_SERIALIZE_WRITE_OVERFLOW]++;
                m_config.packetFactory->Destroy( packet );
                continue;
            }

            const int bytes = stream.GetBytesWritten();
            const uint8_t * data = stream.GetData();

            CORE_ASSERT( bytes <= m_config.maxPacketSize );
            if ( bytes > m_config.maxPacketSize )
            {
                m_counters[BSD_SOCKET_COUNTER_PACKET_TOO_LARGE_TO_SEND]++;
                m_config.packetFactory->Destroy( packet );
                continue;
            }

            SendPacketInternal( packet->GetAddress(), data, bytes );

            m_config.packetFactory->Destroy( packet );
        }
    }

    void BSDSocket::ReceivePackets()
    {
        while ( true )
        {
            if ( core::queue::size( m_receive_queue ) == m_config.receiveQueueSize )
                break;

            Address address;
            int received_bytes = ReceivePacketInternal( address, m_receiveBuffer, m_config.maxPacketSize );
            if ( !received_bytes )
                break;

            typedef protocol::ReadStream Stream;

            Stream stream( m_receiveBuffer, m_config.maxPacketSize );

            stream.SetContext( m_context );

            uint64_t protocolId;
            serialize_uint64( stream, protocolId );
            if ( protocolId != m_config.protocolId )
            {
                m_counters[BSD_SOCKET_COUNTER_PROTOCOL_ID_MISMATCH]++;
                continue;
            }

            const int maxPacketType = m_config.packetFactory->GetNumTypes() - 1;
            int packetType = 0;
            serialize_int( stream, packetType, 0, maxPacketType );

            stream.Align();

            auto packet = m_config.packetFactory->Create( packetType );
            CORE_ASSERT( packet );
            CORE_ASSERT( packet->GetType() == packetType );
            if ( !packet )
            {
//                printf( "failed to create packet of type %d\n", packetType );
                m_counters[BSD_SOCKET_COUNTER_CREATE_PACKET_FAILURES]++;
                continue;
            }

            packet->SerializeRead( stream );

            // IMPORTANT: packet read was aborted. intentionally ignore this packet
            if ( stream.Aborted() )
            {
                m_counters[BSD_SOCKET_COUNTER_ABORTED_PACKET_READS]++;
                m_config.packetFactory->Destroy( packet );
                continue;
            }

            CORE_ASSERT( !stream.IsOverflow() );
            if ( stream.IsOverflow() )
            {
                m_counters[BSD_SOCKET_COUNTER_SERIALIZE_READ_OVERFLOW]++;
                m_config.packetFactory->Destroy( packet );
                continue;
            }

            if ( !stream.Check( 0x51246234 ) )
            {
                // todo: counter for truncated packets
                m_config.packetFactory->Destroy( packet );
                continue;
            }

            packet->SetAddress( address );

            core::queue::push_back( m_receive_queue, packet );
        }
    }

    bool BSDSocket::SendPacketInternal( const Address & address, const uint8_t * data, size_t bytes )
    {
        CORE_ASSERT( m_socket );
        CORE_ASSERT( address.IsValid() );
        CORE_ASSERT( bytes > 0 );
        CORE_ASSERT( bytes <= m_config.maxPacketSize );

        bool result = false;

        m_counters[BSD_SOCKET_COUNTER_PACKETS_SENT]++;

        if ( address.GetType() == ADDRESS_IPV6 )
        {
            sockaddr_in6 s_addr;
            memset( &s_addr, 0, sizeof( s_addr ) );
            s_addr.sin6_family = AF_INET6;
            s_addr.sin6_port = htons( address.GetPort() );
            memcpy( &s_addr.sin6_addr, address.GetAddress6(), sizeof( s_addr.sin6_addr ) );
            const int sent_bytes = sendto( m_socket, (const char*)data, bytes, 0, (sockaddr*)&s_addr, sizeof(sockaddr_in6) );
            result = sent_bytes == bytes;
        }
        else if ( address.GetType() == ADDRESS_IPV4 )
        {
            sockaddr_in s_addr;
            memset( &s_addr, 0, sizeof( s_addr ) );
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

    int BSDSocket::ReceivePacketInternal( Address & sender, void * data, int size )
    {
        CORE_ASSERT( data );
        CORE_ASSERT( size > 0 );
        CORE_ASSERT( m_socket );

        #if CORE_PLATFORM == CORE_PLATFORM_WINDOWS
        typedef int socklen_t;
        #endif
        
        sockaddr_storage from;
        socklen_t fromLength = sizeof( from );

        int result = recvfrom( m_socket, (char*)data, size, 0, (sockaddr*)&from, &fromLength );

        if ( result <= 0 )
        {
            if ( errno == EAGAIN )
                return 0;

            printf( "recvfrom failed: %s\n", strerror( errno ) );

            return 0;
        }

        sender = Address( from );

        CORE_ASSERT( result >= 0 );

        m_counters[BSD_SOCKET_COUNTER_PACKETS_RECEIVED]++;

        return result;
    }
}
