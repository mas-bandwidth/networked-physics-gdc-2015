/*
	Client for "Drop-In COOP for Open World Games"
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include "Mathematics.h"
#include "Platform.h"
#include "Display.h"
#include "Net.h"
#include "Common.h"
#include "Render.h"
#include "GameObject.h"
#include <unistd.h>

const int sendRate = 60;
const int packetSize = 2048;

int main( int argc, char * argv[] )
{
	// initialize sockets

	if ( !InitializeSockets() )
	{
		printf( "failed to initialize sockets!\n" );
		return 1;
	}

	// create connection
	
 	Address address( 127,0,0,1, ServerPort );

	printf( "client connecting to %d.%d.%d.%d:%d\n", address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort() );

	AckConnection connection;
	if ( !connection.Connect( address ) )
	{
		printf( "failed to connect to server\n" );
		return 1;
	}

	// open display

	if ( !OpenDisplay( "Drop-In COOP for Open World Games", DisplayWidth, DisplayHeight ) )
	{
		printf( "failed to open display!\n" );
		return 1;
	}
	
	// create game object manager
	
	GameObjectManager gameObjectManager;

	// setup renderer

	Render render( DisplayWidth, DisplayHeight );

	// main loop

	float time = 0.0f;
	float deltaTime = 0.01f;
	float accumulator = 0.0f;

	math::Vector origin(0,0,0);

	while ( true )
	{
		Input input = Input::Sample();

		if ( input.escape )
			break;

		// send packet

		if ( connection.IsConnected() )
		{
			if ( accumulator >= 1.0f / sendRate )
			{
				Packet packet( packetSize );

				packet.BeginWrite();
				
				packet.WriteBoolean( input.left );
				packet.WriteBoolean( input.right );
				packet.WriteBoolean( input.up );
				packet.WriteBoolean( input.down );
				packet.WriteBoolean( input.space );

				connection.Send( packet, time );

				accumulator = 0.0f;
			}
		}
		else
		{
			accumulator = 0.0f;
		}

		// receive packets

		while ( true )
		{
			Packet packet( packetSize );

			if ( !connection.Receive( packet, time ) )
				break;

			packet.ReadFloat( origin.x );
			packet.ReadFloat( origin.y );
			packet.ReadFloat( origin.z );

			unsigned int updateCount = 0;
			packet.ReadInteger( updateCount );
			
			if ( updateCount > (unsigned int) MaxCubes )
				updateCount = MaxCubes;

			GameObjectUpdate updates[MaxCubes];
			
			for ( int i = 0; i < (int) updateCount; ++i )
			{
				packet.ReadInteger( updates[i].id );
				
				packet.ReadFloat( updates[i].position.x );
				packet.ReadFloat( updates[i].position.y );
				packet.ReadFloat( updates[i].position.z );
				packet.ReadFloat( updates[i].orientation.w );
				packet.ReadFloat( updates[i].orientation.x );
				packet.ReadFloat( updates[i].orientation.y );
				packet.ReadFloat( updates[i].orientation.z );
				packet.ReadFloat( updates[i].scale );
				
				bool pendingHibernation = false;
				packet.ReadBoolean( pendingHibernation );
				updates[i].visible = !pendingHibernation;
				
				updates[i].r = 1.0f;
				updates[i].g = 0.0f;
				updates[i].b = 0.0f;
			}
			
			gameObjectManager.UpdateObjects( updates, updateCount );
		}

		gameObjectManager.Update( deltaTime );

		RenderState renderState;
		gameObjectManager.GetRenderState( renderState );
		
		renderState.hibernationArea.show = true;
		renderState.hibernationArea.origin = origin;
		renderState.hibernationArea.radius = 5.0f;		// todo: get from packet?
		renderState.hibernationArea.startAngle = - time * 0.75f;
		renderState.hibernationArea.r = 1.0f;
		renderState.hibernationArea.g = 1.0f;
		renderState.hibernationArea.b = 0.0f;
		renderState.hibernationArea.a = 0.5f + pow( math::sin( time * 5 ) + 1.0f, 2 ) * 0.8f;
		if ( renderState.hibernationArea.a > 1.0f )
			renderState.hibernationArea.a = 1.0f;
		renderState.hibernationArea.a *= 0.5f;
		
		render.SetCamera( origin + math::Vector( 0.0f, -12.0f, 5.0f ), origin );
		render.ClearScreen();
		render.RenderScene( renderState, 0, 0, DisplayWidth, DisplayHeight );
		render.RenderShadows( renderState, 0, 0, DisplayWidth, DisplayHeight );

		UpdateDisplay( 1 );

		deltaTime = 1.0 / 60.0f;

		connection.Update( time, deltaTime );

		time += deltaTime;				// todo: remove - accumulating floating point time is evil!
		accumulator += deltaTime;
		
		if ( connection.IsTimedOut() )
			break;
	}

	// shutdown

	CloseDisplay();

	ShutdownSockets();

	return 0;
}
