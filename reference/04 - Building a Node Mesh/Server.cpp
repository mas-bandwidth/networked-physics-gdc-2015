/*
	Server Example using Node Mesh
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
const int NodePort = 30001;
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
	
	Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
	
	if ( !mesh.Start( MeshPort ) )
	{
		printf( "failed to start mesh on port %d\n", MeshPort );
		return 1;
	}
	
	Node node( ProtocolId, SendRate, TimeOut );
 	if ( !node.Start( NodePort ) )
	{
		printf( "failed to start node on port %d\n", NodePort );
		return 1;
	}
	
	mesh.Reserve( 0, Address(127,0,0,1,NodePort) );
	node.Join( Address(127,0,0,1,MeshPort) );
	
	while ( true )
	{
		if ( node.IsConnected() )
		{
			assert( node.GetLocalNodeId() == 0 );
			for ( int i = 1; i < node.GetMaxAllowedNodes(); ++i )
			{
				if ( node.IsNodeConnected(i) )
				{
					unsigned char packet[] = "server to client";
					node.SendPacket( i, packet, sizeof(packet) );
				}
			}
			
			while ( true )
			{
				int nodeId = -1;
				unsigned char packet[256];
				int bytes_read = node.ReceivePacket( nodeId, packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				assert( nodeId > 0 );
				printf( "server received packet from client %d\n", nodeId - 1 );
			}
		}
		
		node.Update( DeltaTime );
		mesh.Update( DeltaTime );

		wait_seconds( DeltaTime );
	}

	mesh.Stop();	
	
	ShutdownSockets();

	return 0;
}
