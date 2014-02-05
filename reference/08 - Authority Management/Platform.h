/*
	Simple Network Library from "Networking for Game Programmers"
	http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#ifndef NET_PLATFORM_H
#define NET_PLATFORM_H

// platform detection

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

#ifndef PLATFORM
#error unknown platform!
#endif


#include <assert.h>
#include <stdio.h>
#include <unistd.h>

namespace net
{
	// platform independent wait for n seconds

#if PLATFORM == PLATFORM_WINDOWS

	inline void wait_seconds( float seconds )
	{
		Sleep( (int) ( seconds * 1000.0f ) );
	}

#else

	inline void wait_seconds( float seconds ) { usleep( (int) ( seconds * 1000000.0f ) ); }

#endif

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
}

#endif
