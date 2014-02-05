// *very* basic network library

// note: this is all legacy, todo: replace with new net transport layer

#ifndef NET_H
#define NET_H

#include "Platform.h"

#if PLATFORM == PLATFORM_PC

	#include <objbase.h>
	#pragma comment( lib, "wsock32.lib" )
	#pragma comment( lib, "ole32.lib" )

#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <errno.h>
	#include <stdio.h>
	#include <unistd.h>

#else

	#error unknown platform!

#endif

inline bool InitializeSockets()
{
	#if PLATFORM == PLATFORM_PC
    WSADATA WsaData;
	return WSAStartup( MAKEWORD(2,2), &WsaData ) != NO_ERROR;
	#else
	return true;
	#endif
}

inline void ShutdownSockets()
{
	#if PLATFORM == PLATFORM_PC
	WSACleanup();
	#endif
}

// simple network address wrapper

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
	
private:
	
	unsigned int address;
	unsigned short port;
};

// socket class

class Socket
{
public:
	
	Socket()
	{
		socket = 0;
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

		if ( socket <= 0 )
		{
			printf( "failed to create socket\n" );
			socket = 0;
			return false;
		}

		// bind to address / port

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

		#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
		
			int nonBlocking = 1;
			if ( fcntl( socket, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
			{
				printf( "failed to set non-blocking socket\n" );
				Close();
				return false;
			}
			
		#elif PLATFORM == PLATFORM_PC
		
			DWORD nonBlocking = 1;
			if ( ioctlsocket( socket, FIONBIO, &nonBlocking ) != 0 )
			{
				printf( "failed to set non-blocking socket\n" );
				Close();
				return false;
			}

		#endif
		
		return true;
	}
	
	void Close()
	{
		if ( socket != 0 )
		{
			#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
			close( socket );
			#elif PLATFORM == PLATFORM_PC
			socket_close( socket );
			#endif
			socket = 0;
		}
	}
	
	bool IsOpen() const
	{
		return socket != 0;
	}
	
	bool Send( const Address & destination, const void * data, int size )
	{
		assert( data );
		assert( size > 0 );
		
		if ( socket == 0 )
			return false;
		
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
		
		if ( socket == 0 )
			return false;
			
		#if PLATFORM == PLATFORM_PC
		typedef int socklen_t;
		#endif
			
		sockaddr_in from;
		socklen_t fromLength = sizeof( from );

		int received_bytes = recvfrom( socket, (char*)data, size, 0, (sockaddr*)&from, &fromLength );

		if ( received_bytes <= 0 )
			return 0;

		unsigned int address = ntohl( from.sin_addr.s_addr );
		unsigned int port = ntohs( from.sin_port );

		sender = Address( address, port );

		return received_bytes;
	}
		
private:
	
	int socket;
};

// packet class

class Packet
{
public:
	
	Packet( int maximum )
	{
		assert( maximum > 0 );
		this->size = 0;
		this->index = 0;
		this->maximum = maximum;
		data = new unsigned char[maximum];
	}
	
	~Packet()
	{
		delete[] data;
		data = NULL;
	}

	// mode
	
	void BeginWrite()
	{
		index = 0;
		size = 0;
	}
	
	void BeginRead( void * data, int bytes )
	{
		assert( bytes > 0 );
		assert( bytes <= maximum );
		size = bytes;
		index = 0;
		memcpy( this->data, data, bytes );
	}

	// read/write
	
 	bool WriteBoolean( bool value )
	{
		if ( size + 1 >= maximum )
			return false;
		if ( value )
			data[size] = 1;
		else
			data[size] = 0;
		size += 1;
		return true;
	}
	
	bool WriteByte( unsigned char value )
	{
		if ( size + 1 >= maximum )
			return false;
		data[size] = value;
		size ++;
		return true;
	}
	
 	bool WriteInteger( unsigned int value )
	{
		if ( size + 4 >= maximum )
			return false;
		data[size]   = ( value >> 24 );
		data[size+1] = ( value >> 16 ) & 0xFF;
		data[size+2] = ( value >> 8 ) & 0xFF;
		data[size+3] = ( value ) & 0xFF;
		size += 4;
		return true;
	}
	
	bool WriteFloat( float value )
	{
		union FloatInteger
		{
			float floatValue;
			unsigned int intValue;
		};
		FloatInteger tmp;
		tmp.floatValue = value;
		return WriteInteger( tmp.intValue );
	}
	
	bool ReadBoolean( bool & value )
	{
		if ( index + 1 >= maximum )
			return false;
		value = data[index] == 1;
		index += 1;
		return true;
	}
	
	bool ReadByte( unsigned char & value )
	{
		if ( index + 1 >= maximum )
			return false;
		value = data[index];
		index ++;
		return true;
	}
	
	bool ReadInteger( unsigned int & value )
	{
		if ( index + 4 >= maximum )
			return false;
		value = ( (unsigned int) ( data[index]   ) << 24 ) | 
		        ( (unsigned int) ( data[index+1] ) << 16 ) | 
		        ( (unsigned int) ( data[index+2] ) << 8 )  | 
				( (unsigned int) ( data[index+3] ) ); 
		index += 4;
		return true;
	}
	
	bool ReadFloat( float & value )
	{
		union FloatInteger
		{
			float floatValue;
			unsigned int intValue;
		};
		FloatInteger tmp;
		if ( !ReadInteger( tmp.intValue ) )
			return false;
		value = tmp.floatValue;
		return true;
	}
	
	// raw data access

	void * GetData()
	{
		return data;
	}
	
	int GetSize() const
	{
		return size;
	}
	
	int GetMaximum() const
	{
		return maximum;
	}
	
private:
	
	int size;
	int index;
	int maximum;
	unsigned char * data;
};

const int MaximumPacketSize = 4096;

// simple connection class (UDP)
//  + ridiculously minimal for client/server connections with a single client
//  + server must have a dedicated IP, client may be behind a NAT

class Connection
{
public:

	static const int magic = 0xFFFFFFFF;			// todo: turn magic number into "protocol id"

	enum Mode
	{
		Disconnected,
		Client,
		Server
	};
	
	Connection()
	{
		mode = Disconnected;
		timeout = 0.0f;
		timedOut = false;
	}
	
	virtual ~Connection()
	{
		// ...
	}
	
 	bool Listen( int port )
	{
		Stop();
		mode = Server;
		timedOut = false;
		return socket.Open( port );
	}
	
	bool Connect( const Address & address )
	{
		Stop();
		mode = Client;
		timedOut = false;
		this->address = address;
		return socket.Open( 0 );
	}

	bool IsConnected() const
	{
		return socket.IsOpen() && mode != Disconnected && address != Address();
	}
	
	bool IsTimedOut() const
	{
		return timedOut;
	}
	
	const Address & GetAddress() const
	{
		return address;
	}
	
	void Stop()
	{
		socket.Close();
		mode = Disconnected;
		address = Address();
	}
	
 	bool Send( Packet & packet )
	{
		if ( mode == Disconnected )
			return false;
		
		if ( mode == Server && address == Address() )
			return false;
					
		if ( packet.GetSize() + 4 >= MaximumPacketSize )
			return false;
		
		unsigned char buffer[MaximumPacketSize];
		
		buffer[0] = 0xFF;
		buffer[1] = 0xFF;
		buffer[2] = 0xFF;
		buffer[3] = 0xFF;
		
		memcpy( buffer + 4, packet.GetData(), packet.GetSize() );

		return socket.Send( address, buffer, packet.GetSize() + 4 );
	}
	
	bool Receive( Packet & packet )
	{
		unsigned char buffer[MaximumPacketSize];

		Address sender;
		int receivedBytes = socket.Receive( sender, buffer, MaximumPacketSize );

		if ( receivedBytes <= 4 )
			return false;

		if ( buffer[0] != 0xFF || buffer[1] != 0xFF || buffer[2] != 0xFF || buffer[3] != 0xFF )
			return false;

		if ( mode == Server && address == Address() )
			address = sender;

		packet.BeginRead( buffer + 4, receivedBytes - 4 );
		
		timeout = 0.0f;
		
		return true;
	}
	
	void Update( float deltaTime )
	{
		if ( mode != Disconnected )
		{
			timeout += deltaTime;
			if ( timeout > 5.0f )
				OnTimeOut();
		}
	}
	
protected:
	
	virtual void OnTimeOut()
	{
		if ( mode == Server && address != Address() )
		{
			printf( "timeout\n" );
			address = Address();
			timedOut = true;
		}
		else if ( mode == Client )
		{
			printf( "timeout\n" );
			Stop();
			timedOut = true;
		}
	}
	
private:
	
	Socket socket;
	Address address;
	float timeout;
	bool timedOut;
	Mode mode;
};

#include <unistd.h>
#include <list>
#include <algorithm>
#include <functional>

class AckConnection : public Connection
{
public:
	
	AckConnection()
	{
		Defaults();
	}
	
 	bool Send( Packet & userPacket, float time )
	{
		Packet packet( MaximumPacketSize );

		unsigned int ackbits = 0;
		int bit_index = 0;
		for ( int i = ack - 1; i > (int) ( ack - 1 - window_size ); --i )
		{
			bool found = false;
			for ( PacketQueue::iterator itor = receivedQueue.begin(); itor != receivedQueue.end(); ++itor )
			{
				if ( itor->sequence == (unsigned int) i )
				{
					ackbits |= 1 << bit_index;
					found = true;
					break;
				}
			}
			bit_index++;
		}

		packet.BeginWrite();
		packet.WriteInteger( sequence );
		packet.WriteInteger( ack );
		packet.WriteInteger( ackbits );
		
		unsigned char * userData = (unsigned char*) userPacket.GetData();
		for ( int i = 0; i < userPacket.GetSize(); ++i )
			packet.WriteByte( userData[i] );

		if ( !Connection::Send( packet ) )
			return false;
	
		PacketData data;
		data.sequence = sequence;
		data.time = time;
		data.size = packet.GetSize();
		sentQueue.push_back( data );
		pendingAckQueue.push_back( data );
	
		sent_packets++;
		sequence++;
		
		return true;
	}
	
	bool Receive( Packet & userPacket, float time )
	{
		Packet packet( MaximumPacketSize );
	
		if ( !Connection::Receive( packet ) )
			return false;
		
		unsigned int packet_sequence = 0;
		unsigned int packet_ack = 0;
		unsigned int packet_ackbits = 0;
		
		packet.ReadInteger( packet_sequence );
		packet.ReadInteger( packet_ack );
		packet.ReadInteger( packet_ackbits );

		ack = packet_sequence;

		for ( PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); ++itor )
		{
			if ( itor->sequence == packet_ack )
			{
				const float packet_rtt = time - itor->time;
				rtt += ( packet_rtt - rtt ) * rtt_tightness;
				
				PacketData data;
				data.sequence = itor->sequence;
				data.time = itor->time;
				data.size = packet.GetSize();
				ackedQueue.push_back( data );
				
				acked_packets++;
				
				pendingAckQueue.erase( itor );

				break;
			}
		}
		
		int bit_index = 0;
		for ( int i = packet_ack - 1; i > (int) ( packet_ack - 1 - window_size ); --i )
		{
			if ( ( packet_ackbits >> bit_index ) & 1 )
			{
				for ( PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); ++itor )
				{
					if ( itor->sequence == (unsigned int) i )
					{
						const float packet_rtt = time - itor->time;
						rtt += ( packet_rtt - rtt ) * rtt_tightness;

						PacketData data;
						data.sequence = i;
						data.time = itor->time;
						data.size = packet.GetSize();
						ackedQueue.push_back( data );

						acked_packets++;

						pendingAckQueue.erase( itor );

						break;
					}
				}
			}
			bit_index++;
		}

		PacketData data;
		data.sequence = packet_sequence;
		data.time = time;
		data.size = packet.GetSize();
		receivedQueue.push_back( data );
		
		userPacket.BeginRead( ( (unsigned char*) packet.GetData() ) + headerSize, packet.GetSize() - headerSize );
		
		return true;
	}
	
	void Update( float time, float deltaTime )
	{
		// update base connection
		
		Connection::Update( deltaTime );
		
		// update sent queue

		while ( sentQueue.size() && sentQueue.front().time < time - rtt_timeout )
			sentQueue.pop_front();
		
		// update pending ack queue

		while ( pendingAckQueue.size() && pendingAckQueue.front().time < time - rtt_timeout )
		{
			pendingAckQueue.pop_front();
			lost_packets++;
		}
		
		// update received queue

		receivedQueue.sort();

		while ( receivedQueue.size() > window_size )
			receivedQueue.pop_front();

		// update acked queue

		ackedQueue.sort();
		
		while ( ackedQueue.size() && ackedQueue.front().time < time - rtt_timeout * 2 )
			ackedQueue.pop_front();
			
		// calculate sent bandwidth
		
		assert( rtt_timeout == 1.0f );

		int sent_bytes_per_second = 0;
		
		for ( PacketQueue::iterator itor = sentQueue.begin(); itor != sentQueue.end(); ++itor )
			sent_bytes_per_second += itor->size;

		// calculate acked packets per second

		assert( rtt_timeout == 1.0f );

		int acked_packets_per_second = 0;
		int acked_bytes_per_second = 0;
		
		for ( PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor )
		{
			if ( itor->time <= time - rtt_timeout )
			{
				acked_packets_per_second++;
				acked_bytes_per_second += itor->size;
			}
		}

		// update stats
		
		sent_bandwidth = sent_bytes_per_second * ( 8 / 1000.0f );
		acked_bandwidth = acked_bytes_per_second * ( 8 / 1000.0f );
	}
	
	// queries

	int GetHeaderSize() const
	{
		return headerSize;
	}
	
	unsigned int GetSentPackets() const
	{
		return sent_packets;
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
	
protected:
	
	virtual void OnTimeOut()
	{
		Defaults();
		Connection::OnTimeOut();
	}

private:

	void Defaults()
	{
		headerSize = 12;
		
		rtt = 0.0f;
		rtt_timeout = 1.0f;
		rtt_tightness = 0.01f;
		
		sequence = 0;
		ack = 0;
		
		sent_packets = 0;
		lost_packets = 0;
		acked_packets = 0;
		
		sent_bandwidth = 0.0f;
		acked_bandwidth = 0.0f;
		
		window_size = 32;
	}
	
	struct PacketData
	{
		unsigned int sequence;			// packet sequence # (waiting for ack)
		float time;					    // time packet was sent, received or acked (depending on context)
		int size;						// packet size in bytes
		bool operator < ( const PacketData & other ) const { return sequence < other.sequence; }
	};

	typedef std::list<PacketData> PacketQueue;

	int headerSize;

	float rtt;
	float rtt_timeout;
	float rtt_tightness;

	unsigned int sequence;
	unsigned int ack;

	PacketQueue sentQueue;				// sent packets (kept until rtt_timeout)
	PacketQueue receivedQueue;			// received packets for determining acks (32 entries kept only)
	PacketQueue ackedQueue;				// acked packets (kept until rtt_timeout * 2)
	PacketQueue pendingAckQueue;		// sent packets which have not been acked yet
	
	unsigned int sent_packets;
	unsigned int lost_packets;
	unsigned int acked_packets;
	
	float sent_bandwidth;
	float acked_bandwidth;
	
	unsigned int window_size;
};

#endif
