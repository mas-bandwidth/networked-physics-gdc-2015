/*
	Client Example using Node Mesh
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

const int MaxNodes = 4;
const int MeshPort = 30000;
const int ProtocolId = 0x12341234;
const float DeltaTime = 0.25f;
const float SendRate = 0.25f;
const float TimeOut = 10.0f;

int main( int argc, char * argv[] )
{
	if ( !InitializeSockets() )
	{
		printf( "failed to initialize sockets\n" );
		return 1;
	}
		
	Node node( ProtocolId, SendRate, TimeOut );
 	if ( !node.Start( 0 ) )
	{
		printf( "failed to start node\n" );
		return 1;
	}
	
	node.Join( Address(127,0,0,1,MeshPort) );
	
	bool connected = false;
	
	while ( true )
	{
		if ( node.IsConnected() )
		{
			if ( !connected )
			{
				printf( "connected as client %d\n", node.GetLocalNodeId() );
				connected = true;
			}
			
			unsigned char packet[] = "client to server";
			node.SendPacket( 0, packet, sizeof(packet) );
			
			while ( true )
			{
				int nodeId = -1;
				unsigned char packet[256];
				int bytes_read = node.ReceivePacket( nodeId, packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				assert( nodeId == 0 );
				printf( "client received packet from server\n" );
			}
		}
		
		if ( !connected && node.JoinFailed() || connected && !node.IsConnected() )
			break;
		
		node.Update( DeltaTime );

		wait_seconds( DeltaTime );
	}
	
	ShutdownSockets();

	return 0;
}
