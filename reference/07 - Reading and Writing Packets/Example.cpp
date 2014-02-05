/*
	Reading and Writing Packets Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "NetStream.h"

using namespace std;
using namespace net;

enum GameMode
{
	Client,
	Server
};

struct ExampleA
{
	bool booleanValue;
	unsigned char byteValue;
	unsigned short shortValue;
	unsigned int intValue;
	float floatValue;
	
	bool Serialize( Stream & stream, GameMode mode )
	{
		int bits_before = stream.GetBitsProcessed();
		stream.SerializeBoolean( booleanValue );
		stream.SerializeByte( byteValue );
		stream.SerializeShort( shortValue );
		stream.SerializeInteger( intValue );
		stream.SerializeFloat( floatValue );
		int bits_after = stream.GetBitsProcessed();
		int serialized_bits = bits_after - bits_before;
		assert( serialized_bits == 1 + 8 + 16 + 32 + 32 );
		return true;
	}
};

struct ExampleB
{
	float health;
	
	bool Serialize( Stream & stream, GameMode mode )
	{
		bool dead = stream.IsWriting() && health < 0.001f;
		stream.SerializeBoolean( dead );
		if ( !dead )
			stream.SerializeFloat( health );
		return true;
	}
};

struct ExampleC
{
	unsigned char count;
	unsigned int values[15];
	
	bool Serialize( Stream & stream, GameMode mode )
	{
		stream.SerializeByte( count, 0, 15 );
		for ( int i = 0; i < count; ++i )
			stream.SerializeInteger( values[i] );
		return true;
	}
};

struct ExampleD
{
	unsigned int clientToServerData;
	unsigned int serverToClientData;
	
	bool Serialize( Stream & stream, GameMode mode )
	{
		if ( mode == Client && stream.IsWriting() || mode == Server && stream.IsReading() )
			stream.SerializeInteger( clientToServerData );
		if ( mode == Server && stream.IsWriting() || mode == Client && stream.IsReading() )
			stream.SerializeInteger( serverToClientData );
		return true;
	}
};

struct ExampleE
{
	char stringOne[64];
	char stringTwo[64];
	
	bool Serialize( Stream & stream, GameMode mode )
	{
		for ( int i = 0; i < (int) sizeof(stringOne); ++i )
		{
			stream.SerializeByte( stringOne[i] );
			if ( stringOne[i] == '\0' )
				break;
		}
		
		unsigned int length = stream.IsWriting() ? strlen( (const char*)stringTwo ) : 0;
		stream.SerializeInteger( length, 0, 63 );
		for ( unsigned int i = 0; i < length; ++i )
			stream.SerializeByte( stringTwo[i] );
		stringTwo[length+1] = '\0'; 
		
		return true;
	}
};

const int MaxObjects = 15;						// max number of objects
const int MaxObjectBits = 100;					// max bits of object data to write

struct Object
{
	unsigned char id;
	unsigned char bits;
	
	Object()
	{
		id = 0;
		bits = 1;
	}
	
	Object( unsigned char id, unsigned char bits )
	{
		assert( id <= MaxObjects - 1 );
		this->id = id;
		assert( bits >= 1 );
		assert( bits <= 32 );
		this->bits = bits;
	}
	
	bool Serialize( Stream & stream )
	{
		stream.SerializeByte( id, 0, MaxObjects - 1 );
		unsigned int value = 0xFFFFFFFF;
		stream.SerializeByte( bits, 1, 32 );
		stream.SerializeBits( value, bits );
		return true;
	}
};

struct ExampleF
{
	unsigned int count;
	Object objects[MaxObjects];
	
	bool Serialize( Stream & stream, GameMode mode )
	{
		unsigned char tmp[256];

		if ( stream.IsWriting() )
		{
			bool send[MaxObjects];
			int send_bits = 0;
			unsigned int send_count = 0;
			for ( unsigned int i = 0; i < count; ++i )
			{
				Stream measure( Stream::Write, tmp, sizeof(tmp) );
				objects[i].Serialize( measure );
				int object_bits = measure.GetBitsProcessed();
				send[i] = send_bits + object_bits <= MaxObjectBits;
				if ( send[i] )
				{
					send_bits += object_bits;
					send_count++;
				}
			}
			stream.SerializeInteger( send_count, 0, MaxObjects );
			int bits_before = stream.GetBitsProcessed();
			for ( unsigned int i = 0; i < count; ++i )
			{
				if ( send[i] )
				{
					objects[i].Serialize( stream );
				}
			}
			int bits_after = stream.GetBitsProcessed();
			int bits_serialized = bits_after - bits_before;
			assert( send_bits = bits_serialized );
		}
		else
		{
			stream.SerializeInteger( count, 0, MaxObjects );
			for ( unsigned int i = 0; i < count; ++i )
			{
				if ( !objects[i].Serialize( stream ) )
					return false;
			}
		}
		
		return true;
	}
};

struct ExampleG
{
	struct X
	{
		float one,two,three;
		bool Serialize( Stream & stream, GameMode mode )
		{
			stream.SerializeFloat( one );
			stream.SerializeFloat( two );
			stream.SerializeFloat( three );
			return true;
		}
	};

	struct Y
	{
		int left;
		int right;
		bool Serialize( Stream & stream, GameMode mode )
		{
			stream.SerializeInteger( left );
			stream.SerializeInteger( right );
			return true;
		}
	};

	struct Z
	{
		bool testing;
		bool Serialize( Stream & stream, GameMode mode )
		{
			stream.SerializeBoolean( testing );
			return true;
		}
	};

	enum DataType
	{
		DataTypeX,
		DataTypeY,
		DataTypeZ,
		DataTypeMax
	};

	DataType dataType;

	union
	{
		X x;
		Y y;
		Z z;
	};

	ExampleG()
	{
		dataType = DataTypeX;
		x.one = 1.0f;
		x.two = 2.0f;
		x.three = 3.0f;
	}

	void SetX( float one, float two, float three )
	{
		dataType = DataTypeX;
		x.one = 1.0f;
		x.two = 2.0f;
		x.three = 3.0f;
	}

	void SetY( int left, int right )
	{
		dataType = DataTypeY;
		y.left = left;
		y.right = right;
	}

	void SetZ( bool testing )
	{
		dataType = DataTypeZ;
		z.testing = testing;
	}

	#define SERIALIZE_ENUM( stream, EnumValue, EnumType, EnumMax )							\
	{																						\
		unsigned int value = (unsigned int) EnumValue;										\
		if ( !stream.SerializeInteger( value, 0, EnumMax - 1 ) )							\
			return false;																	\
		EnumValue = (EnumType) value;														\
	}

	bool Serialize( Stream & stream, GameMode mode )
	{
		SERIALIZE_ENUM( stream, dataType, DataType, DataTypeMax );
		
		switch ( dataType )
		{
			case DataTypeX: return x.Serialize( stream, mode );
			case DataTypeY: return y.Serialize( stream, mode );
			case DataTypeZ: return z.Serialize( stream, mode );
			default: return false;
		}
		
		return true;
	}
};

class ExampleH
{
	unsigned int a : 3;
	unsigned int b : 2;
	unsigned int c : 1;
	unsigned int d : 10;
	unsigned int e : 5;
	
	#define SERIALIZE_BITFIELD( stream, bitfield, bits )			\
	{																\
		unsigned int value = (unsigned int) bitfield;				\
		if ( !stream.SerializeBits( value, bits ) )					\
			return false;											\
		bitfield = value;											\
	}

	bool Serialize( Stream & stream, GameMode mode )
	{
		SERIALIZE_BITFIELD( stream, a, 3 );
		SERIALIZE_BITFIELD( stream, b, 2 );
		SERIALIZE_BITFIELD( stream, c, 1 );
		SERIALIZE_BITFIELD( stream, d, 10 );
		SERIALIZE_BITFIELD( stream, e, 5 );
		return true;
	}
};

struct GameData
{
	GameData( GameMode mode )
	{
		this->mode = mode;
		
		if ( mode == Server )
		{
			a.booleanValue = true;
			a.byteValue = 100;
			a.shortValue = 30000;
			a.intValue = 120319341;
			a.floatValue = 7.0f;
			b.health = 100.0f;
			c.count = 3;
			c.values[0] = 1000;
			c.values[1] = 38000;
			c.values[2] = 173;
			d.serverToClientData = 0x11111;
			strcpy( (char*) e.stringOne, "hello from server! (1)" );
			strcpy( (char*) e.stringTwo, "hello from server! (2)" );
			f.count = 15;
			for ( unsigned int i = 0; i < f.count; ++i )
				f.objects[i] = Object( i, 1 + rand() % 32 );
			g.SetY( -1, +1 );
		}
		
		if ( mode == Client )
		{
			a.booleanValue = true;
			a.byteValue = 100;
			a.shortValue = 30000;
			a.intValue = 120319341;
			a.floatValue = 7.0f;
			b.health = 100.0f;
			c.count = 5;
			c.values[0] = 10;
			c.values[1] = 55;
			c.values[2] = 89;
			c.values[3] = 500;
			c.values[4] = 100008;
			d.serverToClientData = 0x22222;
			strcpy( (char*) e.stringOne, "hello from client! (1)" );
			strcpy( (char*) e.stringTwo, "hello from client! (2)" );
			g.SetZ( true );
		}
	}
	
	GameMode mode;
	
	ExampleA a;
	ExampleB b;
	ExampleC c;
	ExampleD d;
	ExampleE e;
	ExampleF f;
	ExampleG g;
	
	bool Serialize( Stream & stream )
	{
		printf( "serialize packet (%s %s)\n", mode == Server ? "server" : "client", stream.IsReading() ? "read" : "write" );
		
		stream.Checkpoint();

		if ( !a.Serialize( stream, mode ) )
			return false;

		if ( !b.Serialize( stream, mode ) )
			return false;
			
		if ( !c.Serialize( stream, mode ) )
			return false;
			
		if ( !d.Serialize( stream, mode ) )
			return false;
			
		if ( !e.Serialize( stream, mode ) )
			return false;

		if ( !f.Serialize( stream, mode ) )
			return false;

		if ( !g.Serialize( stream, mode ) )
			return false;

		stream.Checkpoint();
		
		return true;
	}
};

int main( int argc, char * argv[] )
{
	GameData server_game( Server );
	GameData client_game( Client );;
	
	printf( "--------------------------------------------\n" );

	// write packet (server)
	unsigned char server_buffer[256];
	unsigned char server_journal[512];
	memset( server_buffer, 0, sizeof( server_buffer ) );
	memset( server_journal, 0, sizeof( server_journal ) );
	int buffer_bytes_written = 0;
	int journal_bytes_written = 0;
	{
		Stream stream( Stream::Write, server_buffer, sizeof(server_buffer), server_journal, sizeof(server_journal) );
		server_game.Serialize( stream );
		buffer_bytes_written = stream.GetDataBytes();
		journal_bytes_written = stream.GetJournalBytes();
	}
	printf( " -> %d data bytes, %d journal bytes\n", buffer_bytes_written, journal_bytes_written );

	printf( "--------------------------------------------\n" );

	// "send" the packet over the network (server -> client)
	unsigned char client_buffer[256];
	unsigned char client_journal[512];
	memcpy( client_buffer, server_buffer, buffer_bytes_written );
	memcpy( client_journal, server_journal, journal_bytes_written );
	int buffer_bytes_read = buffer_bytes_written;
	int journal_bytes_read = journal_bytes_written;
	
	// read packet (client)
	int client_buffer_bytes_read = 0;
	int client_journal_bytes_read = 0;
	{
		Stream stream( Stream::Read, client_buffer, buffer_bytes_read, client_journal, journal_bytes_read );
		client_game.Serialize( stream );
		client_buffer_bytes_read = stream.GetDataBytes();
		client_journal_bytes_read = stream.GetJournalBytes();
	}
	printf( " -> %d data bytes, %d journal bytes\n", client_buffer_bytes_read, client_journal_bytes_read );
	
	printf( "--------------------------------------------\n" );

	// todo: send it back the other way ...

	return 0;
}
