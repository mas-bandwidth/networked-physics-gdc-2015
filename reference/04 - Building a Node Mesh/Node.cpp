/*
 	Peer-to-Peer Example using Node Mesh
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
const int MeshPort = 40000;
const int MasterNodePort = 40001;
const int ProtocolId = 0x80808080;
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
	
	const bool masterNode = argc == 2 && strcmp( argv[1], "master" ) == 0; 
	
	Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
	
	if ( masterNode )
	{
		if ( !mesh.Start( MeshPort ) )
		{
			printf( "failed to start mesh on port %d\n", MeshPort );
			return 1;
		}
		mesh.Reserve( 0, Address(127,0,0,1,MasterNodePort) );
	}
		
	Node node( ProtocolId, SendRate, TimeOut );
	const int port = masterNode ? MasterNodePort : 0;
 	if ( !node.Start( port ) )
	{
		printf( "failed to start node on port %d\n", port );
		return 1;
	}
	
	node.Join( Address(127,0,0,1,MeshPort) );
	
	bool connected = false;
	
	while ( true )
	{
		if ( node.IsConnected() )
		{
			connected = true;
			
			const int localNodeId = node.GetLocalNodeId();
			
			for ( int i = 0; i < node.GetMaxAllowedNodes(); ++i )
			{
				if ( i != localNodeId && node.IsNodeConnected(i) )
				{
					unsigned char packet[] = "peer to peer";
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
				printf( "received packet from node %d\n", nodeId );
			}
		}
		
		if ( !masterNode && ( !connected && node.JoinFailed() || connected && !node.IsConnected() ) )
			break;		
		
		node.Update( DeltaTime );
		
		if ( masterNode )
			mesh.Update( DeltaTime );

		wait_seconds( DeltaTime );
	}

	if ( masterNode )
		mesh.Stop();	
	
	ShutdownSockets();

	return 0;
}
