/*
	Beacon Example for LAN Matchmaking
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Net.h"

using namespace std;
using namespace net;

const int ServerPort = 30000;
const int ClientPort = 30001;
const int BeaconPort = 40000;
const int ListenerPort = 40001;
const int ProtocolId = 0x31337;
const float DeltaTime = 0.25f;
const float TimeOut = 10.0f;

int main( int argc, char * argv[] )
{
	// initialize sockets
	
	if ( !InitializeSockets() )
	{
		printf( "failed to initialize sockets\n" );
		return 1;
	}

	// process command line
	
	char hostname[64+1] = "hostname";
	gethostname( hostname, 64 );
	hostname[64] = '\0';
	
	if ( argc == 2 )
	{
		strncpy( hostname, argv[1], 64 );
		hostname[64] = '\0';
	}
	
	// create beacon (sends broadcast packets to the LAN...)
	
	Beacon beacon( hostname, ProtocolId, ListenerPort, ServerPort );
	
	if ( !beacon.Start( BeaconPort ) )
	{
		printf( "could not start beacon\n" );
		return 1;
	}
	
	// create listener (listens for packets sent from beacons on the LAN...)

	Listener listener( ProtocolId, TimeOut );

	if ( !listener.Start( ListenerPort ) )
	{
		printf( "could not start listener\n" );
		return 1;
	}
	
	// main loop

	float accumulator = 0.0f;

	while ( true )
	{
		accumulator += DeltaTime;
		while ( accumulator >= 1.5f )
		{
			printf( "---------------------------------------------\n" );
			const int entryCount = listener.GetEntryCount();
			for ( int i = 0; i < entryCount; ++i )
			{
				const ListenerEntry & entry = listener.GetEntry( i );
				printf( "%d.%d.%d.%d:%d -> %s\n", 
					entry.address.GetA(), entry.address.GetB(), 
					entry.address.GetC(), entry.address.GetD(), 
					entry.address.GetPort(), entry.name );
			}
			printf( "---------------------------------------------\n" );
			accumulator -= 1.5f;
		}
		
		beacon.Update( DeltaTime );
		listener.Update( DeltaTime );
		
		wait_seconds( DeltaTime );
	}
	
	ShutdownSockets();

	return 0;
}
