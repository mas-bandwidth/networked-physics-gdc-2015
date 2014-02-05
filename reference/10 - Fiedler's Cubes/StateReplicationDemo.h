/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#include "Cubes.h"

class StateReplicationDemo : public Demo
{
private:

	enum { MaxPlayers = 2 };
	enum { MaxObjectsInPacket = 256 };

	enum SyncMode
	{
		SYNC_Nothing,
		SYNC_InputOnly,
		SYNC_InputAndState,
	};

	game::Instance<cubes::DatabaseObject, cubes::ActiveObject> * gameInstance[MaxPlayers];
	GameWorkerThread workerThread[MaxPlayers];
	view::Packet viewPacket[MaxPlayers];
	view::ObjectManager viewObjectManager[MaxPlayers];
	render::Render * render;
	Camera camera[MaxPlayers];
	math::Vector origin[MaxPlayers];
	float t;
	int accumulator;
	int sendRate;
	float lag;
	bool follow;
	bool strobe;
	bool tildeDownLastFrame;
	SyncMode syncMode;

public:

	StateReplicationDemo( int displayWidth, int displayHeight )
	{
		render = new render::Render( displayWidth, displayHeight );
		t = 0.0f;
		accumulator = 0;
		sendRate = 1;
		lag = 0.0f;
		strobe = false;
		follow = true;
		tildeDownLastFrame = false;
		syncMode = SYNC_Nothing;
	}

	~StateReplicationDemo()
	{
		for ( int i = 0; i < MaxPlayers; ++i )
			delete gameInstance[i];
		delete render;
	}

	void Initialize()
	{
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			game::Config config;
			config.deactivationTime = 0.5f;
			config.cellSize = 4.0f;
			config.cellWidth = 16;
			config.cellHeight = 16;
			config.activationDistance = 5.0f;
			config.simConfig.ERP = 0.1f;
			config.simConfig.CFM = 0.001f;
			config.simConfig.MaxIterations = 32;
			config.simConfig.MaximumCorrectingVelocity = 100.0f;
			config.simConfig.ContactSurfaceLayer = 0.05f;
			config.simConfig.Elasticity = 0.3f;
			config.simConfig.LinearDrag = 0.01f;
			config.simConfig.AngularDrag = 0.01f;
			config.simConfig.Friction = 200.0f;
			
			gameInstance[i] = new game::Instance<cubes::DatabaseObject, cubes::ActiveObject> ( config );
			
			gameInstance[i]->InitializeBegin();
			
			gameInstance[i]->AddPlane( math::Vector(0,0,1), 0 );
			
			AddCube( gameInstance[i], 1.5f, math::Vector(0,-5,10) );
			{
				const int steps = 20;
				const int border = 1.0f;
				const float origin = -steps / 2 + border;
				const float z = 0.2f;
				const int count = steps - border * 2;
				for ( int y = 0; y < count; ++y )
					for ( int x = 0; x < count; ++x )
						AddCube( gameInstance[i], 0.4f, math::Vector(x+origin,y+origin,z) );
			}
			
			gameInstance[i]->InitializeEnd();

			gameInstance[i]->OnPlayerJoined( 0 );
			gameInstance[i]->SetPlayerFocus( 0, 1 );
			gameInstance[i]->SetLocalPlayer( 0 );
			
			gameInstance[i]->SetFlag( game::FLAG_Hover );
			gameInstance[i]->SetFlag( game::FLAG_Katamari );

			origin[i] = math::Vector(0,0,0);
		}
	}

	void AddCube( game::Instance<cubes::DatabaseObject, cubes::ActiveObject> * gameInstance, float scale, const math::Vector & position, const math::Vector & linearVelocity = math::Vector(0,0,0), const math::Vector & angularVelocity = math::Vector(0,0,0) )
	{
		cubes::DatabaseObject object;
		object.position = position;
		object.orientation = math::Quaternion(1,0,0,0);
		object.scale = scale;
		object.linearVelocity = linearVelocity;
		object.angularVelocity = angularVelocity;
		object.enabled = 1;
		object.activated = 0;
		gameInstance->AddObject( object, position.x, position.y );
	}

	void ProcessInput( const platform::Input & input )
	{
		// demo controls

		if ( input.one )
			sendRate = 1;

		if ( input.two )
			sendRate = 2;

		if ( input.three )
			sendRate = 3;

		if ( input.four )
			sendRate = 6;

		if ( input.five )
			sendRate = 10;

		if ( input.six )
			sendRate = 15;

		if ( input.seven )
			sendRate = 20;

		if ( input.eight )
			sendRate = 30;

		if ( input.nine )
			sendRate = 60;

		if ( input.zero )
			sendRate = 0;

		if ( input.tilde && !tildeDownLastFrame )
		{
			follow = !follow;
		}
		tildeDownLastFrame = input.tilde;

		if ( input.f1 )
			syncMode = SYNC_Nothing;

		if ( input.f2 )
			syncMode = SYNC_InputOnly;

		if ( input.f3 )
			syncMode = SYNC_InputAndState;

		// pass input to game instance

		game::Input gameInput;
		gameInput.left = input.left ? 1.0f : 0.0f;
		gameInput.right = input.right ? 1.0f : 0.0f;
		gameInput.up = input.up ? 1.0f : 0.0f;
		gameInput.down = input.down ? 1.0f : 0.0f;
		gameInput.push = input.space ? 1.0f : 0.0f;
		gameInput.pull = input.z ? 1.0f : 0.0f;

		gameInstance[0]->SetPlayerInput( 0, gameInput );
	}

	void Update( float deltaTime )
	{
		// fake some networking

		struct TempObject
		{
			ObjectId id;
			uint32_t authority : 1;
			uint32_t enabled : 1;
			uint32_t framesAtRest : 7;
			math::Vector position;
			math::Quaternion orientation;
			math::Vector linearVelocity;
			math::Vector angularVelocity;
		};

		struct TempPacket
		{
			uint32_t frame;
			game::Input input;
			int objectCount;
			TempObject object[MaxObjectsInPacket];
			bool authority[MaxObjectsInPacket];
		};

		static engine::PacketQueue packetQueue;
		packetQueue.SetDelay( lag );

		static int accumulator = 0;
		accumulator++;
		TempPacket packet;
		packet.frame = gameInstance[0]->GetPlayerFrame( 0 );
		gameInstance[0]->GetPlayerInput( 0, packet.input );
		if ( sendRate > 0 && accumulator >= sendRate )
		{
			packet.objectCount = MaxObjectsInPacket;
			if ( packet.objectCount > gameInstance[0]->GetActiveObjectCount() )
				packet.objectCount = gameInstance[0]->GetActiveObjectCount();
			for ( int i = 0; i < packet.objectCount; ++i )
			{
				const cubes::ActiveObject & activeObject = gameInstance[0]->GetPriorityObject( 0, i );
				gameInstance[0]->ResetObjectPriority( 0, i );
				packet.object[i].id = activeObject.id;
				packet.object[i].authority = gameInstance[0]->GetObjectAuthority( packet.object[i].id ) == 0;
				packet.object[i].enabled = activeObject.enabled;
				packet.object[i].framesAtRest = 0;
				packet.object[i].position = activeObject.position;
				packet.object[i].orientation = activeObject.orientation;
				packet.object[i].linearVelocity = activeObject.linearVelocity;
				packet.object[i].angularVelocity = activeObject.angularVelocity;
			}
			accumulator = 0;
		}
		else
		{
			packet.objectCount = 0;
		}
		
		const int from = 0;
		const int to = 1;
		
		packetQueue.QueuePacket( from, to, (unsigned char*)&packet, sizeof(TempPacket) );

		packetQueue.Update( deltaTime );

		while ( engine::PacketQueue::Packet * pkt = packetQueue.PacketReadyToSend() )
		{
			TempPacket * packet = (TempPacket*) &pkt->data[0];
			if ( syncMode != SYNC_Nothing )
			{
				gameInstance[1]->SetPlayerInput( 0, packet->input );
				if ( syncMode == SYNC_InputAndState )
				{
					gameInstance[1]->SetPlayerFrame( 0, packet->frame );
					if ( packet->objectCount > 0 )
					{
						for ( int i = 0; i < packet->objectCount; ++i )
						{
							cubes::ActiveObject activeObject;
							gameInstance[1]->GetObjectState( packet->object[i].id, activeObject );
							activeObject.enabled = packet->object[i].enabled;
							activeObject.position = packet->object[i].position;
							activeObject.orientation = packet->object[i].orientation;
							activeObject.linearVelocity = packet->object[i].linearVelocity;
							activeObject.angularVelocity = packet->object[i].angularVelocity;
							gameInstance[1]->SetObjectState( activeObject.id, activeObject );
							if ( packet->object[i].authority )
								gameInstance[1]->SetObjectAuthority( packet->object[i].id, 0 );
							else
								gameInstance[1]->SetObjectAuthority( packet->object[i].id, ::MaxPlayers, true );
						}
					}
				}
			}
			delete pkt;
		}

		// grab the view packet & start the worker thread...
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			gameInstance[i]->GetViewPacket( viewPacket[i] );
			workerThread[i].Start( gameInstance[i] );
		}

		// advance time
		t += deltaTime;
	}

	void Render( float deltaTime, bool shadows )
	{
		// update the scene to be rendered

		for ( int i = 0; i < MaxPlayers; ++i )
		{
			if ( viewPacket[i].objectCount >= 1 )
			{
				view::ObjectUpdate updates[MaxViewObjects];
				getViewObjectUpdates( updates, viewPacket[i] );
				for ( int j = 0; j < viewPacket[i].objectCount; ++j )
					getAuthorityColor( updates[j].authority, updates[j].r, updates[j].g, updates[j].b );
				viewObjectManager[i].UpdateObjects( updates, viewPacket[i].objectCount );
			}
			viewObjectManager[i].ExtrapolateObjects( deltaTime );
			viewObjectManager[i].Update( deltaTime );
		}

		// update cameras

		for ( int i = 0; i < MaxPlayers; ++i )
		{
			view::Object * playerCube = viewObjectManager[i].GetObject( 1 );
			if ( playerCube )
				origin[i] = playerCube->position + playerCube->positionError;

			math::Vector lookat = origin[i];
			math::Vector position = lookat + math::Vector(0,-10,5);

			if ( !follow )
			{
				lookat = math::Vector( 0.0f, 0.0f, 0.0f );
				position = math::Vector( 0.0f, -15.0f, 4.0f );
			}

			camera[i].EaseIn( lookat, position ); 
		}

		// render the scenes

		render->ClearScreen();

		int width = render->GetDisplayWidth();
		int height = render->GetDisplayHeight();

		Cubes cubes;
		viewObjectManager[0].GetRenderState( cubes );
		setCameraAndLight( render, camera[0] );
		render->BeginScene( 0, 0, width/2, height );
		ActivationArea activationArea;
		setupActivationArea( activationArea, origin[0], 5.0f, t );
		render->RenderActivationArea( activationArea, 1.0f );
		render->RenderCubes( cubes );
		if ( shadows )
			render->RenderCubeShadows( cubes );

		viewObjectManager[1].GetRenderState( cubes );
		setCameraAndLight( render, camera[1] );
		render->BeginScene( width/2, 0, width, height );
		setupActivationArea( activationArea, origin[1], 5.0f, t );
		render->RenderActivationArea( activationArea, 1.0f );
		render->RenderCubes( cubes );
		if ( shadows )
			render->RenderCubeShadows( cubes );

		if ( shadows )
		{
			render->EnterScreenSpace();
			render->RenderShadowQuad();
		}
	}

	void PostRender()
	{
		// join worker threads
		for ( int i = 0; i < MaxPlayers; ++i )
			workerThread[i].Join();
	}
};
