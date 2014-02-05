/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef NETWORK_H
#define NETWORK_H

#include "Mathematics.h"

// platform detection

#define NET_PLATFORM_WINDOWS  1
#define NET_PLATFORM_MAC      2
#define NET_PLATFORM_UNIX     3

#if defined(_WIN32)
#define NET_PLATFORM NET_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define NET_PLATFORM NET_PLATFORM_MAC
#else
#define NET_PLATFORM NET_PLATFORM_UNIX
#endif

#ifndef NET_PLATFORM
#error unknown platform!
#endif

#if PLATFORM == PLATFORM_WINDOWS

	#define WIN32_LEAN_AND_MEAN
	#define VC_EXTRALEAN
	#include <winsock2.h>
	#pragma comment( lib, "wsock32.lib" )
	#undef min
	#undef max
	#undef GetObject

#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <unistd.h>

#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>
#include <stack>
#include <list>
#include <algorithm>
#include <functional>

namespace net
{
	// internet address

	class Address
	{
	public:
	
		Address()
		{
			address = 0;
			port = 0;
		}
	
		Address( unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port )
		{
			this->address = ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | d;
			this->port = port;
		}
	
		Address( unsigned int address, unsigned short port )
		{
			this->address = address;
			this->port = port;
		}
	
		unsigned int GetAddress() const
		{
			return address;
		}
	
		unsigned char GetA() const
		{
			return ( unsigned char ) ( address >> 24 );
		}
	
		unsigned char GetB() const
		{
			return ( unsigned char ) ( address >> 16 );
		}
	
		unsigned char GetC() const
		{
			return ( unsigned char ) ( address >> 8 );
		}
	
		unsigned char GetD() const
		{
			return ( unsigned char ) ( address );
		}
	
		unsigned short GetPort() const
		{ 
			return port;
		}
	
		bool operator == ( const Address & other ) const
		{
			return address == other.address && port == other.port;
		}
	
		bool operator != ( const Address & other ) const
		{
			return ! ( *this == other );
		}
		
		bool operator < ( const Address & other ) const
		{
			// note: this is so we can use address as a key in std::map
			if ( address < other.address )
				return true;
			if ( address > other.address )
				return false;
			else
				return port < other.port;
		}
	
	private:
	
		unsigned int address;
		unsigned short port;
	};
	
	// socket functionality

	inline bool InitializeSockets()
	{
		#if NET_PLATFORM == NET_PLATFORM_WINDOWS
	    WSADATA WsaData;
		return WSAStartup( MAKEWORD(2,2), &WsaData ) == NO_ERROR;
		#else
		return true;
		#endif
	}

	inline void ShutdownSockets()
	{
		#if NET_PLATFORM == NET_PLATFORM_WINDOWS
		WSACleanup();
		#endif
	}

	class Socket
	{
	public:
	
		enum Options
		{
			NonBlocking = 1,
			Broadcast = 2
		};
	
		Socket( int options = NonBlocking )
		{
			this->options = options;
			#if NET_PLATFORM == NET_PLATFORM_WINDOWS
			socket = INVALID_SOCKET;
			#else
			socket = -1;
			#endif
		}
	
		~Socket()
		{
			Close();
		}
	
		bool Open( unsigned short port )
		{
			assert( !IsOpen() );
		
			// create socket

			socket = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

			if ( !IsOpen() )
			{
				printf( "failed to create socket\n" );
				return false;
			}

			// bind to port

			sockaddr_in address;
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = INADDR_ANY;
			address.sin_port = htons( (unsigned short) port );
		
			if ( bind( socket, (const sockaddr*) &address, sizeof(sockaddr_in) ) < 0 )
			{
				printf( "failed to bind socket\n" );
				Close();
				return false;
			}

			// set non-blocking io

			if ( options & NonBlocking )
			{
				#if NET_PLATFORM == NET_PLATFORM_MAC || NET_PLATFORM == NET_PLATFORM_UNIX
		
					int nonBlocking = 1;
					if ( fcntl( socket, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
					{
						printf( "failed to set non-blocking socket\n" );
						Close();
						return false;
					}
			
				#elif NET_PLATFORM == NET_PLATFORM_WINDOWS
		
					DWORD nonBlocking = 1;
					if ( ioctlsocket( socket, FIONBIO, &nonBlocking ) != 0 )
					{
						printf( "failed to set non-blocking socket\n" );
						Close();
						return false;
					}

				#endif
			}
			
			// set broadcast socket
			
			if ( options & Broadcast )
			{
				int enable = 1;
				if ( setsockopt( socket, SOL_SOCKET, SO_BROADCAST, (const char*) &enable, sizeof( enable ) ) < 0 )
				{
					printf( "failed to set socket to broadcast\n" );
					Close();
					return false;
				}
			}
		
			return true;
		}
	
		void Close()
		{
			if ( IsOpen() )
			{
				#if NET_PLATFORM == NET_PLATFORM_MAC || NET_PLATFORM == NET_PLATFORM_UNIX
				close( socket );
				socket = -1;
				#elif NET_PLATFORM == NET_PLATFORM_WINDOWS
				closesocket( socket );
				socket = INVALID_SOCKET;
				#endif
			}
		}
	
		bool IsOpen() const
		{
			#if NET_PLATFORM == NET_PLATFORM_WINDOWS
			return socket != INVALID_SOCKET;
			#else
			return socket >= 0;
			#endif
		}
	
		bool Send( const Address & destination, const void * data, int size )
		{
			assert( data );
			assert( size > 0 );
		
			if ( !IsOpen() )
				return false;
		
			assert( destination.GetAddress() != 0 );
			assert( destination.GetPort() != 0 );
		
			sockaddr_in address;
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = htonl( destination.GetAddress() );
			address.sin_port = htons( (unsigned short) destination.GetPort() );

			int sent_bytes = sendto( socket, (const char*)data, size, 0, (sockaddr*)&address, sizeof(sockaddr_in) );

			return sent_bytes == size;
		}
	
		int Receive( Address & sender, void * data, int size )
		{
			assert( data );
			assert( size > 0 );
		
			if ( !IsOpen() )
				return false;
			
			#if NET_PLATFORM == NET_PLATFORM_WINDOWS
			typedef int socklen_t;
			#endif
			
			sockaddr_in from;
			socklen_t fromLength = sizeof( from );

			int received_bytes = recvfrom( socket, (char*)data, size, 0, (sockaddr*)&from, &fromLength );

			if ( received_bytes <= 0 )
				return 0;

			unsigned int address = ntohl( from.sin_addr.s_addr );
			unsigned short port = ntohs( from.sin_port );

			sender = Address( address, port );

			return received_bytes;
		}
		
	private:
	
		#if NET_PLATFORM == NET_PLATFORM_WINDOWS
		SOCKET socket;
		#else
		int socket;
		#endif
		int options;
	};
	
	// get host name helper
	
	inline bool GetHostName( char * hostname, int size )
	{
		gethostname( hostname, size );
		hostname[size-1] = '\0';
		return true;
	}

	// helper functions for reading and writing integer values to packets
	
	inline void WriteByte( unsigned char * data, unsigned char value )
	{
		*data = value;
	}

	inline void ReadByte( const unsigned char * data, unsigned char & value )
	{
		value = *data;
	}

	inline void WriteShort( unsigned char * data, unsigned short value )
	{
		data[0] = (unsigned char) ( ( value >> 8 ) & 0xFF );
		data[1] = (unsigned char) ( value & 0xFF );
	}

	inline void ReadShort( const unsigned char * data, unsigned short & value )
	{
		value = ( ( (unsigned int)data[0] << 8 )  | ( (unsigned int)data[1] ) );				
	}

	inline void WriteInteger( unsigned char * data, unsigned int value )
	{
		data[0] = (unsigned char) ( value >> 24 );
		data[1] = (unsigned char) ( ( value >> 16 ) & 0xFF );
		data[2] = (unsigned char) ( ( value >> 8 ) & 0xFF );
		data[3] = (unsigned char) ( value & 0xFF );
	}

	inline void ReadInteger( const unsigned char * data, unsigned int & value )
	{
		value = ( ( (unsigned int)data[0] << 24 ) | ( (unsigned int)data[1] << 16 ) | 
			      ( (unsigned int)data[2] << 8 )  | ( (unsigned int)data[3] ) );				
	}
	
	// packet queue to store information about sent and received packets sorted in sequence order
	//  + we define ordering using the "sequence_more_recent" function, this works provided there is a large gap when sequence wrap occurs
	
	struct PacketData
	{
		unsigned int sequence;			// packet sequence number
		float time;					    // time offset since packet was sent or received (depending on context)
		int size;						// packet size in bytes
	};

	inline bool sequence_more_recent( unsigned int s1, unsigned int s2, unsigned int max_sequence )
	{
		return ( s1 > s2 ) && ( s1 - s2 <= max_sequence/2 ) || ( s2 > s1 ) && ( s2 - s1 > max_sequence/2 );
	}		
	
	class PacketQueue : public std::list<PacketData>
	{
	public:
		
		bool exists( unsigned int sequence )
		{
			for ( iterator itor = begin(); itor != end(); ++itor )
				if ( itor->sequence == sequence )
					return true;
			return false;
		}
		
		void insert_sorted( const PacketData & p, unsigned int max_sequence )
		{
			if ( empty() )
			{
				push_back( p );
			}
			else
			{
				if ( !sequence_more_recent( p.sequence, front().sequence, max_sequence ) )
				{
					push_front( p );
				}
				else if ( sequence_more_recent( p.sequence, back().sequence, max_sequence ) )
				{
					push_back( p );
				}
				else
				{
					for ( PacketQueue::iterator itor = begin(); itor != end(); itor++ )
					{
						assert( itor->sequence != p.sequence );
						if ( sequence_more_recent( itor->sequence, p.sequence, max_sequence ) )
						{
							insert( itor, p );
							break;
						}
					}
				}
			}
		}
		
		void verify_sorted( unsigned int max_sequence )
		{
			PacketQueue::iterator prev = end();
			for ( PacketQueue::iterator itor = begin(); itor != end(); itor++ )
			{
				assert( itor->sequence <= max_sequence );
				if ( prev != end() )
				{
					assert( sequence_more_recent( itor->sequence, prev->sequence, max_sequence ) );
					prev = itor;
				}
			}
		}
	};

	// reliability system to support reliable connection
	//  + manages sent, received, pending ack and acked packet queues
	//  + separated out from reliable connection because it is quite complex and i want to unit test it!
	
	class ReliabilitySystem
	{
	public:
		
		ReliabilitySystem( unsigned int max_sequence = 0xFFFFFFFF )
		{
			this->rtt_maximum = rtt_maximum;
			this->max_sequence = max_sequence;
			Reset();
		}
		
		void Reset()
		{
			local_sequence = 0;
			remote_sequence = 0;
			sentQueue.clear();
			receivedQueue.clear();
			pendingAckQueue.clear();
			ackedQueue.clear();
			sent_packets = 0;
			recv_packets = 0;
			lost_packets = 0;
			acked_packets = 0;
			sent_bandwidth = 0.0f;
			acked_bandwidth = 0.0f;
			rtt = 0.0f;
			rtt_maximum = 1.0f;
		}
		
		void PacketSent( int size )
		{
			if ( sentQueue.exists( local_sequence ) )
			{
				printf( "local sequence %d exists\n", local_sequence );				
				for ( PacketQueue::iterator itor = sentQueue.begin(); itor != sentQueue.end(); ++itor )
					printf( " + %d\n", itor->sequence );
			}
			assert( !sentQueue.exists( local_sequence ) );
			assert( !pendingAckQueue.exists( local_sequence ) );
			PacketData data;
			data.sequence = local_sequence;
			data.time = 0.0f;
			data.size = size;
			sentQueue.push_back( data );
			pendingAckQueue.push_back( data );
			sent_packets++;
			local_sequence++;
			if ( local_sequence > max_sequence )
				local_sequence = 0;
		}
		
		void PacketReceived( unsigned int sequence, int size )
		{
			recv_packets++;
			if ( receivedQueue.exists( sequence ) )
				return;
			PacketData data;
			data.sequence = sequence;
			data.time = 0.0f;
			data.size = size;
			receivedQueue.push_back( data );
			if ( sequence_more_recent( sequence, remote_sequence, max_sequence ) )
				remote_sequence = sequence;
		}

		unsigned int GenerateAckBits()
		{
			return generate_ack_bits( GetRemoteSequence(), receivedQueue, max_sequence );
		}
		
		void ProcessAck( unsigned int ack, unsigned int ack_bits )
		{
			process_ack( ack, ack_bits, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, max_sequence );
		}
				
		void Update( float deltaTime )
		{
			acks.clear();
			AdvanceQueueTime( deltaTime );
			UpdateQueues();
			UpdateStats();
			#ifdef NET_UNIT_TEST
			Validate();
			#endif
		}
		
		void Validate()
		{
			sentQueue.verify_sorted( max_sequence );
			receivedQueue.verify_sorted( max_sequence );
			pendingAckQueue.verify_sorted( max_sequence );
			ackedQueue.verify_sorted( max_sequence );
		}

		// utility functions

		static bool sequence_more_recent( unsigned int s1, unsigned int s2, unsigned int max_sequence )
		{
			return ( s1 > s2 ) && ( s1 - s2 <= max_sequence/2 ) || ( s2 > s1 ) && ( s2 - s1 > max_sequence/2 );
		}
		
		static int bit_index_for_sequence( unsigned int sequence, unsigned int ack, unsigned int max_sequence )
		{
			assert( sequence != ack );
			assert( !sequence_more_recent( sequence, ack, max_sequence ) );
			if ( sequence > ack )
			{
				assert( ack < 33 );
				assert( max_sequence >= sequence );
 				return ack + ( max_sequence - sequence );
			}
			else
			{
				assert( ack >= 1 );
				assert( sequence <= ack - 1 );
 				return ack - 1 - sequence;
			}
		}
		
		static unsigned int generate_ack_bits( unsigned int ack, const PacketQueue & received_queue, unsigned int max_sequence )
		{
			unsigned int ack_bits = 0;
			for ( PacketQueue::const_iterator itor = received_queue.begin(); itor != received_queue.end(); itor++ )
			{
				if ( itor->sequence == ack || sequence_more_recent( itor->sequence, ack, max_sequence ) )
					break;
				int bit_index = bit_index_for_sequence( itor->sequence, ack, max_sequence );
				if ( bit_index <= 31 )
					ack_bits |= 1 << bit_index;
			}
			return ack_bits;
		}
		
		static void process_ack( unsigned int ack, unsigned int ack_bits, 
								 PacketQueue & pending_ack_queue, PacketQueue & acked_queue, 
								 std::vector<unsigned int> & acks, unsigned int & acked_packets, 
								 float & rtt, unsigned int max_sequence )
		{
			if ( pending_ack_queue.empty() )
				return;
				
			PacketQueue::iterator itor = pending_ack_queue.begin();
			while ( itor != pending_ack_queue.end() )
			{
				bool acked = false;
				
				if ( itor->sequence == ack )
				{
					acked = true;
				}
				else if ( !sequence_more_recent( itor->sequence, ack, max_sequence ) )
				{
					int bit_index = bit_index_for_sequence( itor->sequence, ack, max_sequence );
					if ( bit_index <= 31 )
						acked = ( ack_bits >> bit_index ) & 1;
				}
				
				if ( acked )
				{
					rtt += ( itor->time - rtt ) * 0.1f;

					acked_queue.insert_sorted( *itor, max_sequence );
					acks.push_back( itor->sequence );
					acked_packets++;
					itor = pending_ack_queue.erase( itor );
				}
				else
					++itor;
			}
		}
		
		// data accessors
				
		unsigned int GetLocalSequence() const
		{
			return local_sequence;
		}

		unsigned int GetRemoteSequence() const
		{
			return remote_sequence;
		}
		
		unsigned int GetMaxSequence() const
		{
			return max_sequence;
		}
				
 		void GetAcks( unsigned int ** acks, int & count )
		{
			*acks = &this->acks[0];
			count = (int) this->acks.size();
		}
		
		unsigned int GetSentPackets() const
		{
			return sent_packets;
		}
		
		unsigned int GetReceivedPackets() const
		{
			return recv_packets;
		}

		unsigned int GetLostPackets() const
		{
			return lost_packets;
		}

		unsigned int GetAckedPackets() const
		{
			return acked_packets;
		}

		float GetSentBandwidth() const
		{
			return sent_bandwidth;
		}

		float GetAckedBandwidth() const
		{
			return acked_bandwidth;
		}

		float GetRoundTripTime() const
		{
			return rtt;
		}
		
		int GetHeaderSize() const
		{
			return 12;
		}

	protected:
		
		void AdvanceQueueTime( float deltaTime )
		{
			for ( PacketQueue::iterator itor = sentQueue.begin(); itor != sentQueue.end(); itor++ )
				itor->time += deltaTime;

			for ( PacketQueue::iterator itor = receivedQueue.begin(); itor != receivedQueue.end(); itor++ )
				itor->time += deltaTime;

			for ( PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); itor++ )
				itor->time += deltaTime;

			for ( PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); itor++ )
				itor->time += deltaTime;
		}
		
		void UpdateQueues()
		{
			const float epsilon = 0.001f;

			while ( sentQueue.size() && sentQueue.front().time > rtt_maximum + epsilon )
				sentQueue.pop_front();

			if ( receivedQueue.size() )
			{
				const unsigned int latest_sequence = receivedQueue.back().sequence;
				const unsigned int minimum_sequence = latest_sequence >= 34 ? ( latest_sequence - 34 ) : max_sequence - ( 34 - latest_sequence );
				while ( receivedQueue.size() && !sequence_more_recent( receivedQueue.front().sequence, minimum_sequence, max_sequence ) )
					receivedQueue.pop_front();
			}

			while ( ackedQueue.size() && ackedQueue.front().time > rtt_maximum * 2 - epsilon )
				ackedQueue.pop_front();

			while ( pendingAckQueue.size() && pendingAckQueue.front().time > rtt_maximum + epsilon )
			{
				pendingAckQueue.pop_front();
				lost_packets++;
			}
		}
		
		void UpdateStats()
		{
			int sent_bytes_per_second = 0;
			for ( PacketQueue::iterator itor = sentQueue.begin(); itor != sentQueue.end(); ++itor )
				sent_bytes_per_second += itor->size;
			int acked_packets_per_second = 0;
			int acked_bytes_per_second = 0;
			for ( PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor )
			{
				if ( itor->time >= rtt_maximum )
				{
					acked_packets_per_second++;
					acked_bytes_per_second += itor->size;
				}
			}
			sent_bytes_per_second /= rtt_maximum;
			acked_bytes_per_second /= rtt_maximum;
			sent_bandwidth = sent_bytes_per_second * ( 8 / 1000.0f );
			acked_bandwidth = acked_bytes_per_second * ( 8 / 1000.0f );
		}
		
	private:
		
		unsigned int max_sequence;			// maximum sequence value before wrap around (used to test sequence wrap at low # values)
		unsigned int local_sequence;		// local sequence number for most recently sent packet
		unsigned int remote_sequence;		// remote sequence number for most recently received packet
		
		unsigned int sent_packets;			// total number of packets sent
		unsigned int recv_packets;			// total number of packets received
		unsigned int lost_packets;			// total number of packets lost
		unsigned int acked_packets;			// total number of packets acked

		float sent_bandwidth;				// approximate sent bandwidth over the last second
		float acked_bandwidth;				// approximate acked bandwidth over the last second
		float rtt;							// estimated round trip time
		float rtt_maximum;					// maximum expected round trip time (hard coded to one second for the moment)

		std::vector<unsigned int> acks;		// acked packets from last set of packet receives. cleared each update!

		PacketQueue sentQueue;				// sent packets used to calculate sent bandwidth (kept until rtt_maximum)
		PacketQueue pendingAckQueue;		// sent packets which have not been acked yet (kept until rtt_maximum * 2 )
		PacketQueue receivedQueue;			// received packets for determining acks to send (kept up to most recent recv sequence - 32)
		PacketQueue ackedQueue;				// acked packets (kept until rtt_maximum * 2)
	};

	// virtual connection over UDP
	//  + very simple, just for learning purposes do not use in production code
	
	class Connection
	{
	public:
		
		enum Mode
		{
			None,	
			Client,
			Server
		};
		
		Connection( unsigned int protocolId, float timeout )
		{
			this->protocolId = protocolId;
			this->timeout = timeout;
			mode = None;
			running = false;
			ClearData();
		}
		
		virtual ~Connection()
		{
			if ( IsRunning() )
				Stop();
		}
		
		bool Start( int port )
		{
			assert( !running );
			printf( "start connection on port %d\n", port );
			if ( !socket.Open( port ) )
				return false;
			running = true;
			OnStart();
			return true;
		}
		
		void Stop()
		{
			assert( running );
			printf( "stop connection\n" );
			bool connected = IsConnected();
			ClearData();
			socket.Close();
			running = false;
			if ( connected )
				OnDisconnect();
			OnStop();
		}
		
		bool IsRunning() const
		{
			return running;
		}
		
		void Listen()
		{
			printf( "server listening for connection\n" );
			bool connected = IsConnected();
			ClearData();
			if ( connected )
				OnDisconnect();
			mode = Server;
			state = Listening;
		}
		
		void Connect( const Address & address )
		{
			printf( "client connecting to %d.%d.%d.%d:%d\n", 
				address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort() );
			bool connected = IsConnected();
			ClearData();
			if ( connected )
				OnDisconnect();
			mode = Client;
			state = Connecting;
			this->address = address;
		}
		
		bool IsConnecting() const
		{
			return state == Connecting;
		}
		
		bool ConnectFailed() const
		{
			return state == ConnectFail;
		}
		
		bool IsConnected() const
		{
			return state == Connected;
		}
		
		bool IsListening() const
		{
			return state == Listening;
		}
		
		Mode GetMode() const
		{
			return mode;
		}
		
		virtual void Update( float deltaTime )
		{
			assert( running );
			timeoutAccumulator += deltaTime;
			if ( timeoutAccumulator > timeout )
			{
				if ( state == Connecting )
				{
					printf( "connect timed out\n" );
					ClearData();
					state = ConnectFail;
					OnDisconnect();
				}
				else if ( state == Connected )
				{
					printf( "connection timed out\n" );
					ClearData();
					if ( state == Connecting )
						state = ConnectFail;
					OnDisconnect();
				}
			}
		}
		
		virtual bool SendPacket( const unsigned char data[], int size )
		{
			assert( running );
			if ( address.GetAddress() == 0 )
				return false;
//			unsigned char packet[size+4];
			unsigned char* packet = reinterpret_cast<unsigned char*>(alloca(size+4));
			packet[0] = (unsigned char) ( protocolId >> 24 );
			packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
			packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
			packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
			memcpy( &packet[4], data, size );
			return socket.Send( address, packet, size + 4 );
		}
		
		virtual int ReceivePacket( unsigned char data[], int size )
		{
			assert( running );
//			unsigned char packet[size+4];
			unsigned char* packet = reinterpret_cast<unsigned char*>(alloca(size+4));
			Address sender;
			int bytes_read = socket.Receive( sender, packet, size + 4 );
			if ( bytes_read == 0 )
				return 0;
			if ( bytes_read <= 4 )
				return 0;
			unsigned int packetProtocolId;
			ReadInteger( packet, packetProtocolId );
			if ( packetProtocolId != protocolId )
			{
				printf( "incorrect protocol id: %x vs. %x\n", packetProtocolId, protocolId );
				return 0;
			}
			if ( mode == Server && !IsConnected() )
			{
				printf( "server accepts connection from client %d.%d.%d.%d:%d\n", 
					sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), sender.GetPort() );
				state = Connected;
				address = sender;
				OnConnect();
			}
			if ( sender == address )
			{
				if ( mode == Client && state == Connecting )
				{
					printf( "client completes connection with server\n" );
					state = Connected;
					OnConnect();
				}
				timeoutAccumulator = 0.0f;
				memcpy( data, &packet[4], bytes_read - 4 );
				return bytes_read - 4;
			}
			return 0;
		}
		
		int GetHeaderSize() const
		{
			return 4;
		}
		
	protected:
		
		virtual void OnStart()		{}
		virtual void OnStop()		{}
		virtual void OnConnect()    {}
		virtual void OnDisconnect() {}
			
	private:
		
		void ClearData()
		{
			state = Disconnected;
			timeoutAccumulator = 0.0f;
			address = Address();
		}
	
		enum State
		{
			Disconnected,
			Listening,
			Connecting,
			ConnectFail,
			Connected
		};

		unsigned int protocolId;
		float timeout;
		
		bool running;
		Mode mode;
		State state;
		Socket socket;
		float timeoutAccumulator;
		Address address;
	};
	
	// connection with reliability (seq/ack)
	//  + just for demonstrating the reliability system

	class ReliableConnection : public Connection
	{
	public:
		
		ReliableConnection( unsigned int protocolId, float timeout, unsigned int max_sequence = 0xFFFFFFFF )
			: Connection( protocolId, timeout ), reliabilitySystem( max_sequence )
		{
			ClearData();
			#ifdef NET_UNIT_TEST
			packet_loss_mask = 0;
			#endif
		}
	
		~ReliableConnection()
		{
			if ( IsRunning() )
				Stop();
		}
		
		// overriden functions from "Connection"
				
		bool SendPacket( const unsigned char data[], int size )
		{
			#ifdef NET_UNIT_TEST
			if ( reliabilitySystem.GetLocalSequence() & packet_loss_mask )
			{
				reliabilitySystem.PacketSent( size );
				return true;
			}
			#endif
			const int header = 12;
//			unsigned char packet[header+size];
			unsigned char* packet = reinterpret_cast<unsigned char*>(alloca(header+size));
			unsigned int seq = reliabilitySystem.GetLocalSequence();
			unsigned int ack = reliabilitySystem.GetRemoteSequence();
			unsigned int ack_bits = reliabilitySystem.GenerateAckBits();
			WriteHeader( packet, seq, ack, ack_bits );
			memcpy( packet + header, data, size );
 			if ( !Connection::SendPacket( packet, size + header ) )
				return false;
			reliabilitySystem.PacketSent( size );
			return true;
		}	
		
		int ReceivePacket( unsigned char data[], int size )
		{
			const int header = 12;
			if ( size <= header )
				return false;
//			unsigned char packet[header+size];
			unsigned char* packet = reinterpret_cast<unsigned char*>(alloca(header+size));
			int received_bytes = Connection::ReceivePacket( packet, size + header );
			if ( received_bytes == 0 )
				return false;
			if ( received_bytes <= header )
				return false;
			unsigned int packet_sequence = 0;
			unsigned int packet_ack = 0;
			unsigned int packet_ack_bits = 0;
			ReadHeader( packet, packet_sequence, packet_ack, packet_ack_bits );
			reliabilitySystem.PacketReceived( packet_sequence, received_bytes - header );
			reliabilitySystem.ProcessAck( packet_ack, packet_ack_bits );
			memcpy( data, packet + header, received_bytes - header );
			return received_bytes - header;
		}
		
		void Update( float deltaTime )
		{
			Connection::Update( deltaTime );
			reliabilitySystem.Update( deltaTime );
		}
		
		int GetHeaderSize() const
		{
			return Connection::GetHeaderSize() + reliabilitySystem.GetHeaderSize();
		}
		
		ReliabilitySystem & GetReliabilitySystem()
		{
			return reliabilitySystem;
		}

		// unit test controls
		
		#ifdef NET_UNIT_TEST
		void SetPacketLossMask( unsigned int mask )
		{
			packet_loss_mask = mask;
		}
		#endif
		
	protected:		
		
		void WriteHeader( unsigned char * header, unsigned int sequence, unsigned int ack, unsigned int ack_bits )
		{
			WriteInteger( header, sequence );
			WriteInteger( header + 4, ack );
			WriteInteger( header + 8, ack_bits );
		}

		void ReadHeader( const unsigned char * header, unsigned int & sequence, unsigned int & ack, unsigned int & ack_bits )
		{
			ReadInteger( header, sequence );
			ReadInteger( header + 4, ack );
			ReadInteger( header + 8, ack_bits );
		}

		virtual void OnStop()
		{
			ClearData();
		}
		
		virtual void OnDisconnect()
		{
			ClearData();
		}
		
	private:

		void ClearData()
		{
			reliabilitySystem.Reset();
		}

		#ifdef NET_UNIT_TEST
		unsigned int packet_loss_mask;			// mask sequence number, if non-zero, drop packet - for unit test only
		#endif
		
		ReliabilitySystem reliabilitySystem;	// reliability system: manages sequence numbers and acks, tracks network stats etc.
	};

	// node mesh
	//  + manages node connect and disconnect
	//  + updates each node with set of currently connected nodes
	
	class Mesh
	{
		struct NodeState
		{
			enum Mode { Disconnected, ConnectionAccept, Connected };
			Mode mode;
			float timeoutAccumulator;
			Address address;
			int nodeId;
			bool reserved;
			NodeState()
			{
				mode = Disconnected;
				address = Address();
				nodeId = -1;
				timeoutAccumulator = 0.0f;
				reserved = false;
			}
		};
		
		unsigned int protocolId;
		float sendRate;
		float timeout;

		Socket socket;
		std::vector<NodeState> nodes;
		typedef std::map<int,NodeState*> IdToNode;
		typedef std::map<Address,NodeState*> AddrToNode;
		IdToNode id2node;
		AddrToNode addr2node;
		bool running;
		float sendAccumulator;
				
	public:

		Mesh( unsigned int protocolId, int maxNodes = 255, float sendRate = 0.25f, float timeout = 10.0f )
		{
			assert( maxNodes >= 1 );
			assert( maxNodes <= 255 );
			this->protocolId = protocolId;
			this->sendRate = sendRate;
			this->timeout = timeout;
			nodes.resize( maxNodes );
			running = false;
			sendAccumulator = 0.0f;
		}
		
		~Mesh()
		{
			if ( running )
				Stop();
		}
		
		bool Start( int port )
		{
			assert( !running );
			printf( "start mesh on port %d\n", port );
			if ( !socket.Open( port ) )
				return false;
			running = true;
			return true;
		}
		
		void Stop()
		{
			assert( running );
			printf( "stop mesh\n" );
			socket.Close();
			id2node.clear();
			addr2node.clear();
			for ( unsigned int i = 0; i < nodes.size(); ++i )
				nodes[i] = NodeState();
			running = false;
			sendAccumulator = 0.0f;
		}	
		
		void Update( float deltaTime )
		{
			assert( running );
			ReceivePackets();
			SendPackets( deltaTime );
			CheckForTimeouts( deltaTime );
		}
		
	    bool IsNodeConnected( int nodeId )
		{
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			return nodes[nodeId].mode == NodeState::Connected;
		}
		
		Address GetNodeAddress( int nodeId )
		{
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			return nodes[nodeId].address;
		}

	    int GetMaxNodes() const
		{
			assert( nodes.size() <= 255 );
			return (int) nodes.size();
		}
		
		void Reserve( int nodeId, const Address & address )
		{
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			printf( "mesh reserves node id %d for %d.%d.%d.%d:%d\n", 
				nodeId, address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort() );
			nodes[nodeId].mode = NodeState::ConnectionAccept;
			nodes[nodeId].nodeId = nodeId;
			nodes[nodeId].address = address;
			nodes[nodeId].reserved = true;
			addr2node.insert( std::make_pair( address, &nodes[nodeId] ) );
		}
		
	protected:
		
		void ReceivePackets()
		{
			while ( true )
			{
				Address sender;
				unsigned char data[256];
				int size = socket.Receive( sender, data, sizeof(data) );
				if ( !size )
					break;
				ProcessPacket( sender, data, size );
			}
		}

		void ProcessPacket( const Address & sender, unsigned char data[], int size )
		{
			assert( sender != Address() );
			assert( size > 0 );
			assert( data );
			// ignore packets that dont have the correct protocol id
			unsigned int firstIntegerInPacket = ( unsigned(data[0]) << 24 ) | ( unsigned(data[1]) << 16 ) |
			                                    ( unsigned(data[2]) << 8 )  | unsigned(data[3]);
			if ( firstIntegerInPacket != protocolId )
				return;
			// determine packet type
			enum PacketType { ConnectRequest, KeepAlive };
			PacketType packetType;
			if ( data[4] == 0 )
				packetType = ConnectRequest;
			else if ( data[4] == 1 )
				packetType = KeepAlive;
			else
				return;
			// process packet type
			switch ( packetType )
			{
				case ConnectRequest:
				{
					// is address already connecting or connected?
					AddrToNode::iterator itor = addr2node.find( sender );
					if ( itor == addr2node.end() )
					{
						// no entry for address, start connect process...
						int freeSlot = -1;
						for ( unsigned int i = 0; i < nodes.size(); ++i )
						{
							if ( nodes[i].mode == NodeState::Disconnected )
							{
								freeSlot = (int) i;
								break;
							}
						}
						if ( freeSlot >= 0 )
						{
							printf( "mesh accepts %d.%d.%d.%d:%d as node %d\n", 
								sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), sender.GetPort(), freeSlot );
							assert( nodes[freeSlot].mode == NodeState::Disconnected );
							nodes[freeSlot].mode = NodeState::ConnectionAccept;
							nodes[freeSlot].nodeId = freeSlot;
							nodes[freeSlot].address = sender;
							addr2node.insert( std::make_pair( sender, &nodes[freeSlot] ) );
						}
					}
					else if ( itor->second->mode == NodeState::ConnectionAccept )
					{
						// reset timeout accumulator, but only while connecting
						itor->second->timeoutAccumulator = 0.0f;
					}
				}
				break;
				case KeepAlive:
				{
					AddrToNode::iterator itor = addr2node.find( sender );
					if ( itor != addr2node.end() )
					{
						// progress from "connection accept" to "connected"
						if ( itor->second->mode == NodeState::ConnectionAccept )
						{
							itor->second->mode = NodeState::Connected;
							itor->second->reserved = false;
							printf( "mesh completes connection of node %d\n", itor->second->nodeId );
						}
						// reset timeout accumulator for node
						itor->second->timeoutAccumulator = 0.0f;
					}
				}
				break;
			}
		}
		
		void SendPackets( float deltaTime )
		{
			sendAccumulator += deltaTime;
			while ( sendAccumulator > sendRate )
			{
				for ( unsigned int i = 0; i < nodes.size(); ++i )
				{
					if ( nodes[i].mode == NodeState::ConnectionAccept )
					{
						// node is negotiating connect: send "connection accepted" packets
						unsigned char packet[7];
						packet[0] = (unsigned char) ( ( protocolId >> 24 ) & 0xFF );
						packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
						packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
						packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
						packet[4] = 0;
						packet[5] = (unsigned char) i;
						packet[6] = (unsigned char) nodes.size();
						socket.Send( nodes[i].address, packet, sizeof(packet) );
					}
					else if ( nodes[i].mode == NodeState::Connected )
					{
						// node is connected: send "update" packets
						const size_t packetSize = 5+6*nodes.size();
//						unsigned char packet[packetSize];
						unsigned char* packet = reinterpret_cast<unsigned char*>(alloca(packetSize));
						packet[0] = (unsigned char) ( ( protocolId >> 24 ) & 0xFF );
						packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
						packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
						packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
						packet[4] = 1;
						unsigned char * ptr = &packet[5];
						for ( unsigned int j = 0; j < nodes.size(); ++j )
						{
							ptr[0] = (unsigned char) nodes[j].address.GetA();
							ptr[1] = (unsigned char) nodes[j].address.GetB();
							ptr[2] = (unsigned char) nodes[j].address.GetC();
							ptr[3] = (unsigned char) nodes[j].address.GetD();
							ptr[4] = (unsigned char) ( ( nodes[j].address.GetPort() >> 8 ) & 0xFF );
							ptr[5] = (unsigned char) ( ( nodes[j].address.GetPort() ) & 0xFF );
							ptr += 6;
						}
						socket.Send( nodes[i].address, packet, packetSize );
					}
				}
				sendAccumulator -= sendRate;
			}
		}
		
		void CheckForTimeouts( float deltaTime )
		{
			for ( unsigned int i = 0; i < nodes.size(); ++i )
			{
				if ( nodes[i].mode != NodeState::Disconnected )
				{
					nodes[i].timeoutAccumulator += deltaTime;
					if ( nodes[i].timeoutAccumulator > timeout && !nodes[i].reserved )
					{
						printf( "mesh timed out node %d\n", i );
						AddrToNode::iterator addr_itor = addr2node.find( nodes[i].address );
						assert( addr_itor != addr2node.end() );
						addr2node.erase( addr_itor );
						nodes[i] = NodeState();
					}
				}
			}
		}
	};

	// node
	
	class Node
	{
		struct NodeState
		{
			bool connected;
			Address address;
			NodeState()
			{
				connected = false;
				address = Address();
			}
		};
		
		struct BufferedPacket
		{
			int nodeId;
			std::vector<unsigned char> data;
		};
		
		typedef std::stack<BufferedPacket*> PacketBuffer;
		PacketBuffer receivedPackets;

		unsigned int protocolId;
		float sendRate;
		float timeout;
		int maxPacketSize;

		Socket socket;
		std::vector<NodeState> nodes;
		typedef std::map<int,NodeState*> IdToNode;
		typedef std::map<Address,NodeState*> AddrToNode;
		AddrToNode addr2node;
		bool running;
		float sendAccumulator;
		float timeoutAccumulator;
		enum State
		{
			Disconnected,
			Connecting,
			Connected,
			ConnectFail
		};
		State state;
		Address meshAddress;
		int localNodeId;

	public:

		Node( unsigned int protocolId, float sendRate = 0.25f, float timeout = 2.0f, int maxPacketSize = 1024 )
		{
			this->protocolId = protocolId;
			this->sendRate = sendRate;
			this->timeout = timeout;
			this->maxPacketSize = maxPacketSize;
			state = Disconnected;
			running = false;
			ClearData();
		}

		~Node()
		{
			if ( running )
				Stop();
		}

		bool Start( int port )
		{
			assert( !running );
			printf( "start node on port %d\n", port );
			if ( !socket.Open( port ) )
				return false;
			running = true;
			return true;
		}

		void Stop()
		{
			assert( running );
			printf( "stop node\n" );
			ClearData();
			socket.Close();
			running = false;
		}	
		
		void Connect( const Address & address )
		{
			printf( "node connect to %d.%d.%d.%d:%d\n", 
				address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort() );
			ClearData();
			state = Connecting;
			meshAddress = address;
		}
		
		bool IsConnecting() const
		{
			return state == Connecting;
		}
		
		bool ConnectFailed() const
		{
			return state == ConnectFail;
		}
		
		bool IsConnected() const
		{
			return state == Connected;
		}
		
		bool IsDisconnected() const
		{
			return state == Disconnected;
		}
		
		int GetLocalNodeId() const
		{
			return localNodeId;
		}

		void Update( float deltaTime )
		{
			assert( running );
			ReceivePackets();
			SendPackets( deltaTime );
			CheckForTimeout( deltaTime );
		}

	    bool IsNodeConnected( int nodeId )
		{
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			return nodes[nodeId].connected;
		}

		Address GetNodeAddress( int nodeId )
		{
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			return nodes[nodeId].address;
		}

	    int GetMaxNodes() const
		{
			assert( nodes.size() <= 255 );
			return (int) nodes.size();
		}
		
		bool SendPacket( int nodeId, const unsigned char data[], int size )
		{
			assert( running );
			if ( nodes.size() == 0 )
				return false;	// not connected yet
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			if ( nodeId < 0 || nodeId >= (int) nodes.size() )
				return false;
			if ( !nodes[nodeId].connected )
				return false;
			assert( size <= maxPacketSize );
			if ( size > maxPacketSize )
				return false;
			return socket.Send( nodes[nodeId].address, data, size );
		}
		
		int ReceivePacket( int & nodeId, unsigned char data[], int size )
		{
			assert( running );
			if ( !receivedPackets.empty() )
			{
				BufferedPacket * packet = receivedPackets.top();
				assert( packet );
				if ( (int) packet->data.size() <= size )
				{
					nodeId = packet->nodeId;
					size = packet->data.size();
					memcpy( data, &packet->data[0], size );
					delete packet;
					receivedPackets.pop();
					return size;
				}
				else
				{
					delete packet;
					receivedPackets.pop();
				}
			}
			return 0;
		}
		
		unsigned int GetProtocolId() const
		{
			return protocolId;
		}

	protected:

		void ReceivePackets()
		{
			while ( true )
			{
				Address sender;
//				unsigned char data[maxPacketSize];
				unsigned char* data = reinterpret_cast<unsigned char*>(alloca(maxPacketSize));
				int size = socket.Receive( sender, data, maxPacketSize );
				if ( !size )
					break;
				ProcessPacket( sender, data, size );
			}
		}

		void ProcessPacket( const Address & sender, unsigned char data[], int size )
		{
			assert( sender != Address() );
			assert( size > 0 );
			assert( data );
			// is packet from the mesh?
			if ( sender == meshAddress )
			{
				// *** packet sent from the mesh ***
				// ignore packets that dont have the correct protocol id
				unsigned int firstIntegerInPacket = ( unsigned(data[0]) << 24 ) | ( unsigned(data[1]) << 16 ) |
				                                    ( unsigned(data[2]) << 8 )  | unsigned(data[3]);
				if ( firstIntegerInPacket != protocolId )
					return;
				// determine packet type
				enum PacketType { ConnectionAccepted, Update };
				PacketType packetType;
				if ( data[4] == 0 )
					packetType = ConnectionAccepted;
				else if ( data[4] == 1 )
					packetType = Update;
				else
					return;
				// handle packet type
				switch ( packetType )
				{
					case ConnectionAccepted:
					{
						if ( size != 7 )
							return;
						if ( state == Connecting )
						{
							localNodeId = data[5];
							nodes.resize( data[6] );
							printf( "node connects as node %d of %d\n", localNodeId, (int) nodes.size() );
							state = Connected;
						}
						timeoutAccumulator = 0.0f;
					}
					break;
					case Update:
					{
						if ( size != (int) ( 5 + nodes.size() * 6 ) )
							return;
						if ( state == Connected )
						{
							// process update packet
							unsigned char * ptr = &data[5];
							for ( unsigned int i = 0; i < nodes.size(); ++i )
							{
								unsigned char a = ptr[0];
								unsigned char b = ptr[1];
								unsigned char c = ptr[2];
								unsigned char d = ptr[3];
								unsigned short port = (unsigned short)ptr[4] << 8 | (unsigned short)ptr[5];
								Address address( a, b, c, d, port );
								if ( address.GetAddress() != 0 )
								{
									// node is connected
									if ( address != nodes[i].address )
									{
										printf( "node %d: node %d connected\n", localNodeId, i );
										nodes[i].connected = true;
										nodes[i].address = address;
										addr2node[address] = &nodes[i];
									}
								}
								else
								{
									// node is not connected
									if ( nodes[i].connected )
									{
										printf( "node %d: node %d disconnected\n", localNodeId, i );
										AddrToNode::iterator itor = addr2node.find( nodes[i].address );
										assert( itor != addr2node.end() );
										addr2node.erase( itor );
										nodes[i].connected = false;
										nodes[i].address = Address();
									}
								}
								ptr += 6;
							}
						}
						timeoutAccumulator = 0.0f;
					}
					break;
				}
			}
			else
			{
				AddrToNode::iterator itor = addr2node.find( sender );
				if ( itor != addr2node.end() )
				{
					// *** packet sent from another node ***
					NodeState * node = itor->second;
					assert( node );
					int nodeId = (int) ( node - &nodes[0] );
					assert( nodeId >= 0 );
					assert( nodeId < (int) nodes.size() );
					BufferedPacket * packet = new BufferedPacket;
					packet->nodeId = nodeId;
					packet->data.resize( size );
					memcpy( &packet->data[0], data, size );
					receivedPackets.push( packet );
				}
			}
		}

		void SendPackets( float deltaTime )
		{
			sendAccumulator += deltaTime;
			while ( sendAccumulator > sendRate )
			{
				if ( state == Connecting )
				{
					// node is connecting: send "connect request" packets
					unsigned char packet[5];
					packet[0] = (unsigned char) ( ( protocolId >> 24 ) & 0xFF );
					packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
					packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
					packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
					packet[4] = 0;
					socket.Send( meshAddress, packet, sizeof(packet) );
				}
				else if ( state == Connected )
				{
					// node is connected: send "keep alive" packets
					unsigned char packet[5];
					packet[0] = (unsigned char) ( ( protocolId >> 24 ) & 0xFF );
					packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
					packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
					packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
					packet[4] = 1;
					socket.Send( meshAddress, packet, sizeof(packet) );
				}
				sendAccumulator -= sendRate;
			}
		}

		void CheckForTimeout( float deltaTime )
		{
			if ( state == Connecting || state == Connected )
			{
				timeoutAccumulator += deltaTime;
				if ( timeoutAccumulator > timeout )
				{
					if ( state == Connecting )
					{
						printf( "node connect failed\n" );
						state = ConnectFail;
					}
					else
					{
						printf( "node connection timed out\n" );
						state = Disconnected;
					}
					ClearData();
				}
			}
		}
		
		void ClearData()
		{
			nodes.clear();
			addr2node.clear();
			while ( !receivedPackets.empty() )
			{
				BufferedPacket * packet = receivedPackets.top();
				delete packet;
				receivedPackets.pop();
			}
			sendAccumulator = 0.0f;
			timeoutAccumulator = 0.0f;
			localNodeId = -1;
			meshAddress = Address();
		}
	};

	// beacon
	//  + sends broadcast UDP packets to the LAN
	//  + use a beacon to advertise the existence of a server on the LAN
	
	class Beacon
	{
	public:
		
		Beacon( const char name[], unsigned int protocolId, unsigned int listenerPort, unsigned int serverPort )
			: socket( Socket::Broadcast | Socket::NonBlocking )
		{
			strncpy( this->name, name, 64 );
			this->name[64] = '\0';
			this->protocolId = protocolId;
			this->listenerPort = listenerPort;
			this->serverPort = serverPort;
			running = false;
		}
		
		~Beacon()
		{
			if ( running )
				Stop();
		}
		
		bool Start( int port )
		{
			assert( !running );
			printf( "start beacon on port %d\n", port );
			if ( !socket.Open( port ) )
				return false;
			running = true;
			return true;
		}
		
		void Stop()
		{
			assert( running );
			printf( "stop beacon\n" );
			socket.Close();
			running = false;
		}
		
		void Update( float deltaTime )
		{
			assert( running );
			unsigned char packet[12+1+64];
			WriteInteger( packet, 0 );
			WriteInteger( packet + 4, protocolId );
			WriteInteger( packet + 8, serverPort );
			packet[12] = (unsigned char) strlen( name );
			assert( packet[12] < 63);
			memcpy( packet + 13, name, strlen( name ) );
			if ( !socket.Send( Address(255,255,255,255,listenerPort), packet, 12 + 1 + packet[12] ) )
				printf( "failed to send broadcast packet\n" );
			Address sender;
			while ( socket.Receive( sender, packet, 256 ) );
		}
		
	private:
		
		char name[64+1];
		unsigned int protocolId;
		unsigned int listenerPort;
		unsigned int serverPort;
		bool running;
		Socket socket;
	};
	
	// listener entry
	//  + an entry in the list of servers on the LAN
	
	struct ListenerEntry
	{
		char name[64+1];
		Address address;
		float timeoutAccumulator;
	};

	// listener
	//  + listens for broadcast packets sent over the LAN
	//  + use a listener to get a list of all the servers on the LAN
	
	class Listener
	{
	public:
		
		Listener( unsigned int protocolId, float timeout = 10.0f )
		{
			this->protocolId = protocolId;
			this->timeout = timeout;
			running = false;
			ClearData();
		}
		
		~Listener()
		{
			if ( running )
				Stop();
		}
		
		bool Start( int port )
		{
			assert( !running );
			printf( "start listener on port %d\n", port );
			if ( !socket.Open( port ) )
				return false;
			running = true;
			return true;
		}
		
		void Stop()
		{
			assert( running );
			printf( "stop listener\n" );
			socket.Close();
			running = false;
			ClearData();
		}
		
		void Update( float deltaTime )
		{
			assert( running );
			unsigned char packet[256];
			while ( true )
			{
				Address sender;
				int bytes_read = socket.Receive( sender, packet, 256 );
				if ( bytes_read == 0 )
					break;
				if ( bytes_read < 13 )
					continue;
				unsigned int packet_zero;
				unsigned int packet_protocolId;
				unsigned int packet_serverPort;
				unsigned char packet_stringLength;
				ReadInteger( packet, packet_zero );
				ReadInteger( packet + 4, packet_protocolId );
				ReadInteger( packet + 8, packet_serverPort );
				packet_stringLength = packet[12];
				if ( packet_zero != 0 )
					continue;
				if ( packet_protocolId != protocolId )
					continue;
				if ( packet_stringLength > 63 )
					continue;
				if ( packet_stringLength + 12 + 1 > bytes_read )
					continue;
				ListenerEntry entry;
				memcpy( entry.name, packet + 13, packet_stringLength );
				entry.name[packet_stringLength] = '\0';
				entry.address = Address( sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), packet_serverPort );
				entry.timeoutAccumulator = 0.0f;
				ListenerEntry * existingEntry = FindEntry( entry );
				if ( existingEntry )
					existingEntry->timeoutAccumulator = 0.0f;
				else
					entries.push_back( entry );
			}
			std::vector<ListenerEntry>::iterator itor = entries.begin();
			while ( itor != entries.end() )
			{
				itor->timeoutAccumulator += deltaTime;
				if ( itor->timeoutAccumulator > timeout )
					itor = entries.erase( itor );
				else
					++itor;
			}
		}
		
		int GetEntryCount() const
		{
			return entries.size();
		}
		
		const ListenerEntry & GetEntry( int index ) const
		{
			assert( index >= 0 );
			assert( index < (int) entries.size() );
			return entries[index];
		}
		
	protected:
		
		ListenerEntry * FindEntry( const ListenerEntry & entry )
		{
			for ( int i = 0; i < (int) entries.size(); ++i )
			{
				if ( entries[i].address == entry.address && strcmp( entries[i].name, entry.name ) == 0 )
					return &entries[i];
			}
			return NULL;
		}
		
	private:
		
		void ClearData()
		{
			entries.clear();
		}
		
		std::vector<ListenerEntry> entries;
		unsigned int protocolId;
		float timeout;
		bool running;
		Socket socket;
	};

	// bitpacker class
	//  + read and write non-8 multiples of bits efficiently
	
	class BitPacker
	{
	public:
		
		enum Mode
		{
			Read,
			Write
		};
		
		BitPacker( Mode mode, void * buffer, int bytes )
		{
			assert( bytes >= 0 );
			this->mode = mode;
			this->buffer = (unsigned char*) buffer;
			this->ptr = (unsigned char*) buffer;
			this->bytes = bytes;
			bit_index = 0;
			if ( mode == Write )
				memset( buffer, 0, bytes );
		}
		
		void WriteBits( unsigned int value, int bits = 32 )
		{
			assert( ptr );
			assert( buffer );
			assert( bits > 0 );
			assert( bits <= 32 );
			assert( mode == Write );
			if ( bits < 32 )
			{
				const unsigned int mask = ( 1 << bits ) - 1;
				value &= mask;
			}
			do
			{
				assert( ptr - buffer < bytes );
				*ptr |= (unsigned char) ( value << bit_index );
				assert( bit_index < 8 );
				const int bits_written = std::min( bits, 8 - bit_index );
				assert( bits_written > 0 );
				assert( bits_written <= 8 );
				bit_index += bits_written;
				if ( bit_index >= 8 )
				{
					ptr++;
					bit_index = 0;
					value >>= bits_written;
				}
				bits -= bits_written;
				assert( bits >= 0 );
				assert( bits <= 32 );
			}
			while ( bits > 0 );
		}
		
 		void ReadBits( unsigned int & value, int bits = 32 )
		{
			assert( ptr );
			assert( buffer );
			assert( bits > 0 );
			assert( bits <= 32 );
			assert( mode == Read );
			int original_bits = bits;
			int value_index = 0;
			value = 0;
			do
			{
				assert( ptr - buffer < bytes );
				assert( bits >= 0 );
				assert( bits <= 32 );
				int bits_to_read = std::min( 8 - bit_index, bits );
				assert( bits_to_read > 0 );
				assert( bits_to_read <= 8 );
				value |= ( *ptr >> bit_index ) << value_index;
				bits -= bits_to_read;
				bit_index += bits_to_read;
				value_index += bits_to_read;
				assert( value_index >= 0 );
				assert( value_index <= 32 );
				if ( bit_index >= 8 )
				{
					ptr++;
					bit_index = 0;
				}
			}
			while ( bits > 0 );
			if ( original_bits < 32 )
			{
				const unsigned int mask = ( 1 << original_bits ) - 1;
				value &= mask;
			}
		}
		
		void * GetData()
		{
			return buffer;
		}
		
		int GetBits() const
		{
			return ( ptr - buffer ) * 8 + bit_index;
		}
		
		int GetBytes() const
		{
			return (int) ( ptr - buffer ) + ( bit_index > 0 ? 1 : 0 );
		}
		
		int BitsRemaining() const
		{
			return bytes * 8 - ( ( ptr - buffer ) * 8 + bit_index );
		}
		
		Mode GetMode() const
		{
			return mode;
		}
		
		bool IsValid() const
		{
			return buffer != NULL;
		}
		
	private:
		
		int bit_index;
		unsigned char * ptr;
		unsigned char * buffer;
		int bytes;
		Mode mode;
	};
	
	// arithmetic coder (unimplemented)
	
	class ArithmeticCoder
	{
	public:
		
		enum Mode
		{
			Read,
			Write
		};
		
		ArithmeticCoder( Mode mode, void * buffer, unsigned int size )
		{
			assert( buffer );
			assert( size >= 0 );
			this->mode = mode;
			this->buffer = (unsigned char*) buffer;
			this->size = size;
		}

		bool WriteInteger( unsigned int value, unsigned int minimum = 0, unsigned int maximum = 0xFFFFFFFF )
		{
			// ...
			return false;
		}
		
		bool ReadInteger( unsigned int value, unsigned int minimum = 0, unsigned int maximum = 0xFFFFFFFF )
		{
			// ...
			return false;
		}
		
	private:
		
		unsigned char * buffer;
		int size;
		Mode mode;
	};
	
	// stream class
	//  + unifies read and write into a serialize operation
	//  + provides attribution of stream for debugging purposes
	
	class Stream
	{
	public:
		
		enum Mode
		{
			Uninitialized,
			Read,
			Write
		};
		
		Stream( Mode mode, void * buffer, int bytes, void * journal_buffer = NULL, int journal_bytes = 0 )
			: bitpacker( mode == Write ? BitPacker::Write : BitPacker::Read, buffer, bytes ), 
			  journal( mode == Write ? BitPacker::Write : BitPacker::Read, journal_buffer, journal_bytes )
		{
		}
		
		bool SerializeBoolean( bool & value )
		{
			unsigned int tmp = (unsigned int) value;
			bool result = SerializeBits( tmp, 1 );
			value = (bool) tmp;
			return result;
		}

		bool SerializeByte( char & value, char min = -127, char max = +128 )
		{
			// wtf: why do I have to do this!?
			unsigned int tmp = (unsigned int) ( value + 127 );
			bool result = SerializeInteger( tmp, (unsigned int ) ( min + 127 ), ( max + 127 ) );
			value = ( (char) tmp ) - 127;
			return result;
		}
		
		bool SerializeByte( signed char & value, signed char min = -127, signed char max = +128 )
		{
			unsigned int tmp = (unsigned int) ( value + 127 );
			bool result = SerializeInteger( tmp, (unsigned int ) ( min + 127 ), ( max + 127 ) );
			value = ( (signed char) tmp ) - 127;
			return result;
		}

		bool SerializeByte( unsigned char & value, unsigned char min = 0, unsigned char max = 0xFF )
		{
			unsigned int tmp = (unsigned int) value;
			bool result = SerializeInteger( tmp, min, max );
			value = (unsigned char) tmp;
			return result;
		}

		bool SerializeShort( signed short & value, signed short min = -32767, signed short max = +32768 )
		{
			unsigned int tmp = (unsigned int) ( value + 32767 );
			bool result = SerializeInteger( tmp, (unsigned int ) ( min + 32767 ), ( max + 32767 ) );
			value = ( (signed short) tmp ) - 32767;
			return result;
		}

		bool SerializeShort( unsigned short & value, unsigned short min = 0, unsigned short max = 0xFFFF )
		{
			unsigned int tmp = (unsigned int) value;
			bool result = SerializeInteger( tmp, min, max );
			value = (unsigned short) tmp;
			return result;
		}
		
		bool SerializeInteger( signed int & value, signed int min = -2147483646, signed int max = +2147483647 )
		{
			unsigned int tmp = (unsigned int) ( value + 2147483646 );
			bool result = SerializeInteger( tmp, (unsigned int ) ( min + 2147483646 ), ( max + 2147483646 ) );
			value = ( (signed int) tmp ) - 2147483646;
			return result;
		}

		bool SerializeInteger( unsigned int & value, unsigned int min = 0, unsigned int max = 0xFFFFFFFF )
		{
			assert( min < max );
			if ( IsWriting() )
			{
				assert( value >= min );
				assert( value <= max );
			}
			const int bits_required = BitsRequired( min, max );
			unsigned int bits = value - min;
			bool result = SerializeBits( bits, bits_required );
			if ( IsReading() )
			{
				value = bits + min;
				if ( value < min || value > max )
				{
					printf( "serialize integer out of range (read)\n" );
					return false;
				}
			}
			return result;
		}
		
		bool SerializeFloat( float & value )
		{
			union FloatInt
			{
				unsigned int i;
				float f;
			};
			if ( IsReading() )
			{
				FloatInt floatInt;
				if ( !SerializeBits( floatInt.i, 32 ) )
					return false;
				value = floatInt.f;
				return true;
			}
			else
			{
				FloatInt floatInt;
				floatInt.f = value;
				return SerializeBits( floatInt.i, 32 );
			}
		}
		
		bool SerializeCompressedFloat( float & value, float minimum, float maximum, float resolution )
		{
			// determine number of discrete values required for resolution
			
			assert( minimum < maximum );
			
			const float delta = maximum - minimum;
			
			float values = delta / resolution;
			
			assert( values < UINT_MAX );
			
			unsigned int maxIntegerValue = (unsigned int) math::ceiling( values );
			
			// compress if writing

			unsigned int integerValue = 0;
			
			if ( IsWriting() )
			{
				float normalizedValue = math::clamp( ( value - minimum ) / delta, 0.0f, 1.0f );
			
				assert( normalizedValue >= 0.0f );
				assert( normalizedValue <= 1.0f );
			
				integerValue = math::floor( normalizedValue * maxIntegerValue + 0.5f );
			}
			
			// serialize integer value
			
			if ( !SerializeInteger( integerValue, 0, maxIntegerValue ) )
				return false;
			
			// decompress if reading

			if ( IsReading() )
			{
				float normalizedValue = integerValue / float( maxIntegerValue );
			
				value = normalizedValue * delta + minimum;
				
				assert( value >= minimum );
				assert( value <= maximum );
			}
			
			return true;
		}

		bool SerializeCompressedVector( float & x, float & y, float & z, 
										float min, float max, float resolution )
		{
			if ( !SerializeCompressedFloat( x, min, max, resolution ) )
				return false;
			if ( !SerializeCompressedFloat( y, min, max, resolution ) )
				return false;
			if ( !SerializeCompressedFloat( z, min, max, resolution ) )
				return false;
			return true;
		}

		bool SerializeCompressedVector( float & x, float & y, float & z, 
										float x_min, float x_max, float x_resolution,
										float y_min, float y_max, float y_resolution,
										float z_min, float z_max, float z_resolution )
		{
			if ( !SerializeCompressedFloat( x, x_min, x_max, x_resolution ) )
				return false;
			if ( !SerializeCompressedFloat( y, y_min, y_max, y_resolution ) )
				return false;
			if ( !SerializeCompressedFloat( z, z_min, z_max, z_resolution ) )
				return false;
			return true;
		}

		bool SerializeCompressedQuaternion( float & w, float & x, float & y, float & z, float resolution )
		{
			// compress quaternion
			
			unsigned int largest = 0;
			float a,b,c;
			a = 0;
			b = 0;
			c = 0;
			
			if ( IsWriting() )
			{
				#ifdef DEBUG
				const float epsilon = 0.0001f;
				const float length_squared = w*w + x*x + y*y + z*z;
				assert( length_squared >= 1.0f - epsilon && length_squared <= 1.0f + epsilon );
				#endif

				const float abs_w = math::abs( w );
				const float abs_x = math::abs( x );
				const float abs_y = math::abs( y );
				const float abs_z = math::abs( z );

				float largest_value = abs_x;

				if ( abs_y > largest_value )
				{
					largest = 1;
					largest_value = abs_y;
				}

				if ( abs_z > largest_value )
				{
					largest = 2;
					largest_value = abs_z;
				}

				if ( abs_w > largest_value )
				{
					largest = 3;
					largest_value = abs_w;
				}

				switch ( largest )
				{
					case 0:
						if ( x >= 0 )
						{
							a = y;
							b = z;
							c = w;
						}
						else
						{
							a = -y;
							b = -z;
							c = -w;
						}
						break;

					case 1:
						if ( y >= 0 )
						{
							a = x;
							b = z;
							c = w;
						}
						else
						{
							a = -x;
							b = -z;
							c = -w;
						}
						break;

					case 2:
						if ( z >= 0 )
						{
							a = x;
							b = y;
							c = w;
						}
						else
						{
							a = -x;
							b = -y;
							c = -w;
						}
						break;

					case 3:
						if ( w >= 0 )
						{
							a = x;
							b = y;
							c = z;
						}
						else
						{
							a = -x;
							b = -y;
							c = -z;
						}
						break;

					default:
						assert( false );
				}
			}

			// serialize
			
			const float minimum = - 1.0f / 1.414214f;		// note: 1.0f / sqrt(2)
			const float maximum = + 1.0f / 1.414214f;

			if ( !SerializeBits( largest, 2 ) )
				return false;
				
			if ( !SerializeCompressedFloat( a, minimum, maximum, resolution ) )
				return false;
				
			if ( !SerializeCompressedFloat( b, minimum, maximum, resolution ) )
				return false;
				
			if ( !SerializeCompressedFloat( c, minimum, maximum, resolution ) )
				return false;
			
			// decompress quaternion
			
			if ( IsReading() )
			{
				switch ( largest )
				{
					case 0:
					{
						// (?) y z w

						y = a;
						z = b;
						w = c;
						x = math::sqrt( 1 - y*y - z*z - w*w );
					}
					break;

					case 1:
					{
						// x (?) z w

						x = a;
						z = b;
						w = c;
						y = math::sqrt( 1 - x*x - z*z - w*w );
					}
					break;

					case 2:
					{
						// x y (?) w

						x = a;
 						y = b;
						w = c;
						z = math::sqrt( 1 - x*x - y*y - w*w );
					}
					break;

					case 3:
					{
						// x y z (?)

						x = a;
						y = b;
 						z = c;
						w = math::sqrt( 1 - x*x - y*y - z*z );
					}
					break;

					default:
					{
						assert( false );
						w = 0.0f;
						x = 0.0f;
						y = 0.0f;
						z = 0.0f;
					}
				}
			}

			// is this really necessary?!

			const float length_squared = x*x + y*y + z*z + w*w;
			const float length = math::sqrt( length_squared );
			const float inverse_length = 1.0f / length;

			w *= inverse_length;
			x *= inverse_length;
			y *= inverse_length;
			z *= inverse_length;
			
			return true;
		}
		
		bool SerializeBits( unsigned int & value, int bits )
		{
			assert( bits >= 1 );
			assert( bits <= 32 );
			if ( bitpacker.BitsRemaining() < bits )
				return false;
			if ( journal.IsValid() )
			{
				unsigned int token = 2 + bits;		// note: 0 = end, 1 = checkpoint, [2,34] = n - 2 bits written
				if ( IsWriting() )
				{
					journal.WriteBits( token, 6 );
				}
				else
				{
					journal.ReadBits( token, 6 );
					int bits_written = token - 2;
					if ( bits != bits_written )
					{
						printf( "desync read/write: attempting to read %d bits when %d bits were written\n", bits, bits_written );
						return false;
					}
				}
			}
			if ( IsReading() )
				bitpacker.ReadBits( value, bits );
			else
				bitpacker.WriteBits( value, bits );
			return true;
		}
		
		bool Checkpoint()
		{
			if ( journal.IsValid() )
			{
				unsigned int token = 1;		// note: 0 = end, 1 = checkpoint, [2,34] = n - 2 bits written
				if ( IsWriting() )
				{
					journal.WriteBits( token, 6 );
				}
				else
				{
					journal.ReadBits( token, 6 );
					if ( token != 1 )
					{
						printf( "desync read/write: checkpoint not present in journal\n" );
						return false;
					}
				}
			}
			unsigned int magic = 0x12345678;
			unsigned int value = magic;
			if ( bitpacker.BitsRemaining() < 32 )
			{
				printf( "not enough bits remaining for checkpoint\n" );
				return false;
			}
			if ( IsWriting() )
				bitpacker.WriteBits( value, 32 );
			else
				bitpacker.ReadBits( value, 32 );
			if ( value != magic )
			{
				printf( "checkpoint failed!\n" );
				return false;
			}
			return true;
		}
		
		bool IsReading() const
		{
			return bitpacker.GetMode() == BitPacker::Read;
		}
		
		bool IsWriting() const
		{
			return bitpacker.GetMode() == BitPacker::Write;
		}
		
		int GetBitsProcessed() const
		{
			return bitpacker.GetBits();
		}
		
		int GetBitsRemaining() const
		{
			return bitpacker.BitsRemaining();
		}
		
		static int BitsRequired( unsigned int minimum, unsigned int maximum )
		{
			assert( maximum > minimum );
			assert( maximum >= 1 );
			if ( maximum - minimum >= 0x7FFFFFF )
				return 32;
			return BitsRequired( maximum - minimum + 1 );
		}
		
		static int BitsRequired( unsigned int distinctValues )
		{
			assert( distinctValues > 1 );
			unsigned int maximumValue = distinctValues - 1;
			for ( int index = 0; index < 32; ++index )
			{
				if ( ( maximumValue & ~1 ) == 0 )
					return index + 1;
				maximumValue >>= 1;
			}
			return 32;
		}
		
		unsigned char * GetData()
		{
			return (unsigned char*) bitpacker.GetData();
		}
		
		unsigned char * GetJournal()
		{
			return (unsigned char*) journal.GetData();
		}
		
		int GetDataBytes() const
		{
			return bitpacker.GetBytes();
		}
		
		int GetJournalBytes() const
		{
			return journal.GetBytes();
		}
		
		void DumpJournal()
		{
			if ( journal.IsValid() )
			{
				printf( "-----------------------------\n" );
				printf( "dump journal:\n" );
				BitPacker reader( BitPacker::Read, journal.GetData(), journal.GetBytes() );
				while ( reader.BitsRemaining() > 6 )
				{
					unsigned int token = 0;
					reader.ReadBits( token, 6 );
					if ( token == 0 )
						break;
					if ( token == 1 )
						printf( " (checkpoint)\n" );
					else
						printf( " + %d bits\n", token - 2 );
				}
				printf( "-----------------------------\n" );
			}
			else
				printf( "no journal exists!\n" );
		}
		
	private:
		
		BitPacker bitpacker;
		BitPacker journal;
	};
	
	// stream packet
	
	void BuildStreamPacket( Stream & stream, unsigned char * packet, int & packetSize, unsigned int protocolId )
	{
		const int dataBytes = stream.GetDataBytes();
		const int journalBytes = stream.GetJournalBytes();
		const int packetBytes = 8 + dataBytes + journalBytes;

		assert( packetBytes <= packetSize );
		
		WriteInteger( packet, protocolId );
		WriteShort( packet + 4, dataBytes );
		WriteShort( packet + 6, journalBytes );
		
		memcpy( packet + 8, stream.GetData(), dataBytes );
		memcpy( packet + 8 + dataBytes, stream.GetJournal(), journalBytes );
		
		packetSize = packetBytes;
	}
	
 	bool ReadStreamPacket( Stream & stream, unsigned char * packet, int packetSize, unsigned int protocolId )
	{
		unsigned short dataBytes = 0;
		unsigned short journalBytes = 0;
		net::ReadInteger( packet, protocolId );
		net::ReadShort( packet + 4, dataBytes );
		net::ReadShort( packet + 6, journalBytes );
		if ( 8 + dataBytes + journalBytes != packetSize )
		{
			printf( "invalid packet data & journal bytes\n" );
			return false;
		}
		stream = Stream( Stream::Read, packet + 8, dataBytes, packet + 8 + dataBytes, journalBytes );
		return true;
	}
}

#endif
