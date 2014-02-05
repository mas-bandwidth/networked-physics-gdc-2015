/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#include "Cubes.h"

class AuthorityDemo : public Demo
{
protected:
	
	enum { MaxPlayers = 4 };
	enum { MaxObjectsInPacket = 256 };
	
	enum Output
	{
		OUTPUT_Fullscreen,
		OUTPUT_Splitscreen,
		OUTPUT_Quadscreen
	};
	
	enum Visualization
	{
		VIS_Normal,
		VIS_Activation,
		VIS_Ghosts,
		VIS_Merged,
		VIS_NumVisualizations
	};
	
	enum SyncMode
	{
		SYNC_Disabled,
		SYNC_Naive,
		SYNC_PlayerAuthority,
		SYNC_TieBreakAuthority,
		SYNC_InteractionAuthority
	};
	
	Output output;
	SyncMode syncMode;
	Visualization vis;
	game::Interface * gameInstance[MaxPlayers];
	GameWorkerThread workerThread[MaxPlayers];
	view::Packet viewPacket[MaxPlayers];
	view::ObjectManager viewObjectManager[MaxPlayers];
	render::Render * render;
	Camera camera[MaxPlayers];
	math::Vector origin[MaxPlayers];
	float t;
	int activePlayer;
	bool follow;
	bool enterDownLastFrame;
	bool tabDownLastFrame;
	float lag;

public:

	AuthorityDemo( int displayWidth, int displayHeight )
	{
		syncMode = SYNC_Disabled;
		output = OUTPUT_Fullscreen;
		vis = VIS_Normal;
		render = new render::Render( displayWidth, displayHeight );
		activePlayer = 0;
		follow = true;
		enterDownLastFrame = false;
		tabDownLastFrame = false;
		t = 0.0f;
		lag = 0.0f;
	}

	~AuthorityDemo()
	{
		for ( int i = 0; i < MaxPlayers; ++i )
			delete gameInstance[i];
		delete render;
	}
	
	void Initialize()
	{
		const float MinScale = 0.2f;
		const float MaxScale = 0.8f;
		const float CubeDensity = 5.0;
		const float CellSize = 4.0f;
		const float GridSize = 200;

		for ( int i = 0; i < MaxPlayers; ++i )
		{
			game::Config config;
			config.maxObjects = GridSize * GridSize * CubeDensity + MaxPlayers + 1;
			config.deactivationTime = 0.5f;
			config.cellSize = CellSize;
			config.cellWidth = GridSize + 2;
			config.cellHeight = GridSize + 2;
			config.activationDistance = 5.0f;
			config.simConfig.QuickStep = false;
			config.simConfig.ERP = 0.1f;
			config.simConfig.CFM = 0.001f;
			config.simConfig.MaxIterations = 12;
			config.simConfig.MaximumCorrectingVelocity = 100.0f;
			config.simConfig.ContactSurfaceLayer = 0.05f;
			config.simConfig.Elasticity = 0.3f;
			config.simConfig.LinearDrag = 0.01f;
			config.simConfig.AngularDrag = 0.01f;
			config.simConfig.Friction = 200.0f;
			
			game::Instance<cubes::DatabaseObject, cubes::ActiveObject> * instance = 
				new game::Instance<cubes::DatabaseObject, cubes::ActiveObject> ( config );
			
			gameInstance[i] = instance;
			
			origin[i] = math::Vector(0,0,0);

			instance->InitializeBegin();

			instance->AddPlane( math::Vector(0,0,1), 0 );

			AddCube( instance, 1.5f, math::Vector(-5,+5,10), math::Vector(0,0,0), math::Vector(0,0,0) );
			AddCube( instance, 1.5f, math::Vector(+5,+5,10), math::Vector(0,0,0), math::Vector(0,0,0) );
			AddCube( instance, 1.5f, math::Vector(-5,-5,10), math::Vector(0,0,0), math::Vector(0,0,0) );
			AddCube( instance, 1.5f, math::Vector(+5,-5,10), math::Vector(0,0,0), math::Vector(0,0,0) );
		
			srand( 21 );
		
			float y = -GridSize / 2 * CellSize;
			for ( int iy = 0; iy < GridSize; ++iy )
			{
				float x = -GridSize / 2 * CellSize;
				for ( int ix = 0; ix < GridSize; ++ix )
				{
					for ( int j = 0; j < CubeDensity; ++j )
					{
						math::Vector position( math::random_float( x, x + CellSize ), math::random_float( y, y + CellSize ), math::random_float( 1.0f, 5.0f ) );
						math::Vector linearVelocity( math::random_float( -2.0f, +2.0f ), math::random_float( -2.0f, +2.0f ), math::random_float( -2.0f, +2.0f ) );
						math::Vector angularVelocity( math::random_float( -2.0f, +2.0f ), math::random_float( -2.0f, +2.0f ), math::random_float( -2.0f, +2.0f ) );
						const float scale = math::random_float( MinScale, MaxScale );

						AddCube( instance, scale, position, linearVelocity, angularVelocity );
					}
					x += CellSize;
				}
				y += CellSize;
			}
			
			instance->InitializeEnd();

			for ( int j = 0; j < MaxPlayers; ++j )
			{
				instance->OnPlayerJoined( j );
				instance->SetPlayerFocus( j, j + 1 );
			}

			instance->SetLocalPlayer( i );
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
		// special controls
		
		if ( input.control )
		{
			if ( input.tilde )
				lag = 0.0f;
				
			if ( input.one )
				lag = 0.05f;
				
			if ( input.two )
				lag = 0.1f;
				
			if ( input.three )
				lag = 0.25f;

			if ( input.four )
				lag = 0.5f;

			if ( input.five )
				lag = 1.0f;
			
			if ( input.enter && !enterDownLastFrame )
			{
				output = OUTPUT_Fullscreen;
				follow = true;
			}
			enterDownLastFrame = input.enter;

			return;
		}
		
		// demo controls

		if ( input.f1 )
			syncMode = SYNC_Disabled;
		
		if ( input.f2 )
			syncMode = SYNC_Naive;
		
		if ( input.f3 )
			syncMode = SYNC_PlayerAuthority;
		
		if ( input.f4 )
			syncMode = SYNC_TieBreakAuthority;
		
		if ( input.f5 )
			syncMode = SYNC_InteractionAuthority;

		if ( input.one )
			activePlayer = 0;

		if ( input.two )
			activePlayer = 1;

		if ( input.three )
			activePlayer = 2;

		if ( input.four )
			activePlayer = 3;

		if ( input.five )
			follow = true;

		if ( input.six )
			follow = false;

		if ( input.enter && !enterDownLastFrame )
		{
			// note: on the transition from fullscreen to splitscreen
			// we want to snap the first camera to view the red cube
			if ( output == OUTPUT_Fullscreen )
			{
				math::Vector position;
				math::Vector lookat;
				activePlayer = 0;
				DetermineCameraTarget( 0, lookat, position );
				camera[0].Snap( lookat, position );
			}
			
			// now cycle fullscreen -> splitscreen -> quadscreen
			output = (Output) ( output + 1 );
			if ( output > OUTPUT_Quadscreen )
				output = OUTPUT_Fullscreen;
		}
		enterDownLastFrame = input.enter;

		if ( input.tab && !tabDownLastFrame )
		{
			vis = (Visualization) ( vis + 1 );
			if ( vis >= VIS_NumVisualizations )
				vis = (Visualization) 0;
		}
		tabDownLastFrame = input.tab;
		
		// pass input to game instance

		game::Input gameInput;
		gameInput.left = input.left ? 1.0f : 0.0f;
		gameInput.right = input.right ? 1.0f : 0.0f;
		gameInput.up = input.up ? 1.0f : 0.0f;
		gameInput.down = input.down ? 1.0f : 0.0f;
		gameInput.push = input.space ? 1.0f : 0.0f;
		gameInput.pull = input.z ? 1.0f : 0.0f;

		for ( int i = 0; i < MaxPlayers; ++i )
		{
			for ( int j = 0; j < MaxPlayers; ++j )
			{
				if ( j == activePlayer )
					gameInstance[i]->SetPlayerInput( j, gameInput );
				else
					gameInstance[i]->SetPlayerInput( j, game::Input() );
			}
		}
	}

	void Update( float deltaTime )
	{
		// fake some networking

		struct TempObject
		{
			ObjectId id;
			uint32_t enabled : 1;
			uint32_t authority : 3;				// [0,MaxPlayers]
			math::Quaternion orientation;
			math::Vector position;
			math::Vector linearVelocity;
			math::Vector angularVelocity;
		};

		struct TempPacket
		{
			uint32_t frame;
			game::Input input;
			int objectCount;
			TempObject object[MaxObjectsInPacket];
		};
		
		const int sendRate = 1;

		static engine::PacketQueue packetQueue;
		packetQueue.SetDelay( lag );

		// update flags
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			if ( syncMode == SYNC_PlayerAuthority || syncMode == SYNC_TieBreakAuthority )
				gameInstance[i]->SetFlag( game::FLAG_DisableInteractionAuthority );
			else
				gameInstance[i]->ClearFlag( game::FLAG_DisableInteractionAuthority );
		}

		// send packets

		if ( syncMode != SYNC_Disabled )
		{
			static int accumulator = 0;
			accumulator++;

			if ( sendRate > 0 && accumulator >= sendRate )
			{
				for ( int from = 0; from < MaxPlayers; ++from )
				{
					game::Instance<cubes::DatabaseObject, cubes::ActiveObject> * instance = 
						static_cast< game::Instance<cubes::DatabaseObject, cubes::ActiveObject>* > ( gameInstance[from] );

					// hack: some authority tricks for exotic sync modes
					const int objectCount = instance->GetActiveObjectCount();
					for ( int i = 0; i < objectCount; ++i )
					{
						const cubes::ActiveObject & activeObject = instance->GetPriorityObject( 0, i );
						if ( syncMode == SYNC_Naive )
						{
							instance->SetObjectAuthority( activeObject.id, from, true );
						}
						else if ( syncMode == SYNC_PlayerAuthority || syncMode == SYNC_TieBreakAuthority )
						{
							if ( activeObject.id >= MaxPlayers )
								instance->ClearObjectAuthority( activeObject.id );
						}
					}

					// construct the packet: from -> to
					for ( int to = 0; to < MaxPlayers; ++to )
					{
						if ( to == from )
							continue;
							
						TempPacket packet;
					
						packet.frame = instance->GetPlayerFrame( from );
						instance->GetPlayerInput( from, packet.input );
					
						packet.objectCount = MaxObjectsInPacket;
						if ( packet.objectCount > instance->GetActiveObjectCount() )
							packet.objectCount = instance->GetActiveObjectCount();
					
						for ( int i = 0; i < packet.objectCount; ++i )
						{
							const cubes::ActiveObject & activeObject = instance->GetPriorityObject( to, i );
							instance->ResetObjectPriority( to, i );
							packet.object[i].id = activeObject.id;
							if ( syncMode == SYNC_Naive )
								packet.object[i].authority = from;
							else
								packet.object[i].authority = instance->GetObjectAuthority( packet.object[i].id );
							packet.object[i].enabled = activeObject.enabled;
							packet.object[i].position = activeObject.position;
							packet.object[i].orientation = activeObject.orientation;
							packet.object[i].linearVelocity = activeObject.linearVelocity;
							packet.object[i].angularVelocity = activeObject.angularVelocity;
						}

						packetQueue.QueuePacket( from, to, (unsigned char*)&packet, sizeof(TempPacket) );
					}
				}
				
				accumulator = 0;
			}
		}

		// receive packets
		{
			packetQueue.Update( deltaTime );

			while ( engine::PacketQueue::Packet * pkt = packetQueue.PacketReadyToSend() )
			{
				TempPacket * packet = (TempPacket*) &pkt->data[0];
				if ( syncMode != SYNC_Disabled )
				{
					int from = pkt->sourceNodeId;
					int to = pkt->destinationNodeId;
					
					game::Instance<cubes::DatabaseObject, cubes::ActiveObject> * instance = 
						static_cast< game::Instance<cubes::DatabaseObject, cubes::ActiveObject>* > ( gameInstance[to] );
					
					instance->SetPlayerInput( from, packet->input );
					instance->SetPlayerFrame( from, packet->frame );
					
					if ( packet->objectCount > 0 )
					{
						for ( int i = 0; i < packet->objectCount; ++i )
						{
							cubes::ActiveObject activeObject;
							instance->GetObjectState( packet->object[i].id, activeObject );
							activeObject.enabled = packet->object[i].enabled;
							activeObject.position = packet->object[i].position;
							activeObject.orientation = packet->object[i].orientation;
							activeObject.linearVelocity = packet->object[i].linearVelocity;
							activeObject.angularVelocity = packet->object[i].angularVelocity;
							
							if ( syncMode == SYNC_Naive )
							{
								instance->SetObjectState( activeObject.id, activeObject );
							}
							else if ( syncMode == SYNC_PlayerAuthority )
							{
								if ( activeObject.id == (ObjectId) (from + 1) )
								{
									instance->SetObjectState( activeObject.id, activeObject );
									instance->SetObjectAuthority( packet->object[i].id, from );
								}
								else if ( activeObject.id > MaxPlayers )
									instance->SetObjectState( activeObject.id, activeObject );
							}
							else if ( syncMode == SYNC_TieBreakAuthority || syncMode == SYNC_InteractionAuthority )
							{
								if ( activeObject.id == (ObjectId) (from + 1) )
								{
									// player authority
									instance->SetObjectState( activeObject.id, activeObject );
									instance->SetObjectAuthority( packet->object[i].id, from, true );
								}
								else
								{
									int remoteAuthority = packet->object[i].authority;
									int localAuthority = instance->GetObjectAuthority( activeObject.id );
									if ( remoteAuthority == from )
									{
										if ( remoteAuthority <= localAuthority )
										{
											// interaction authority
											instance->SetObjectState( activeObject.id, activeObject );
											instance->SetObjectAuthority( activeObject.id, remoteAuthority );
										}
									}
									else if ( remoteAuthority == MaxPlayers )
									{
										bool active = instance->IsObjectActive( activeObject.id );
										if ( !active )
										{
											// object is not active on this machine
											instance->SetObjectState( activeObject.id, activeObject );
										}
										else if ( active && localAuthority == MaxPlayers && from < to )
										{
											// object is active: tie break authority - lower player id wins
											instance->SetObjectState( activeObject.id, activeObject );
										}
									}
								}
							}
						}
					}
				}
				delete pkt;
			}
		}
		
		// grab the view packets & start the worker threads...
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			gameInstance[i]->GetViewPacket( viewPacket[i] );
			workerThread[i].Start( gameInstance[i] );
		}
		
		// advance time
		t += deltaTime;
	}

	void DetermineCameraTarget( int i, math::Vector & lookat, math::Vector & position )
	{
		int followPlayerId = ( i == 0 && output == OUTPUT_Fullscreen ) ? activePlayer : i;
		lookat = origin[followPlayerId] - math::Vector(0,0,1);
		
		// note: camera for widescreen
		if ( output == OUTPUT_Fullscreen )
			position = lookat + math::Vector(0,-11,5);
		else if ( output == OUTPUT_Splitscreen )
			position = lookat + math::Vector(0,-13,14);
		else
			position = lookat + math::Vector(0,-4,17);

		/*
		if ( output == OUTPUT_Fullscreen )
			position = lookat + math::Vector(0,-12,6);
		else if ( output == OUTPUT_Splitscreen )
			position = lookat + math::Vector(0,-13,14);
		else
			position = lookat + math::Vector(0,-4,17);
			*/

		if ( !follow )
		{
			position = math::Vector(0,0,35.0f);
			lookat = math::Vector(0,0,0);
		}
	}

	void Render( float deltaTime, bool shadows )
	{
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			// update the scene to be rendered

			if ( viewPacket[i].objectCount >= 1 )
			{
				view::ObjectUpdate updates[MaxViewObjects];
				getViewObjectUpdates( updates, viewPacket[i], ( syncMode == SYNC_Disabled ) ? i : -1 );
				viewObjectManager[i].UpdateObjects( updates, viewPacket[i].objectCount );
			}
			viewObjectManager[i].ExtrapolateObjects( deltaTime );
			viewObjectManager[i].Update( deltaTime );

			// track player origin

			view::Object * playerCube = viewObjectManager[i].GetObject( i + 1 );
			if ( playerCube )
				origin[i] = playerCube->position + playerCube->positionError;

			// update camera

			math::Vector lookat;
			math::Vector position;
			DetermineCameraTarget( i, lookat, position );
			camera[i].EaseIn( lookat, position );
		}

		// render the scene

		render->ClearScreen();

		if ( output == OUTPUT_Fullscreen )
		{			
			RenderView( VIEW_Fullscreen, activePlayer, shadows );
		}
		else if ( output == OUTPUT_Splitscreen )
		{
			RenderView( VIEW_Left, 0, shadows );
			RenderView( VIEW_Right, 1, shadows );
			render->DivideSplitscreen();
		}
		else if ( output == OUTPUT_Quadscreen )
		{
			RenderView( VIEW_TopLeft, 0, shadows );
			RenderView( VIEW_TopRight, 1, shadows );
			RenderView( VIEW_BottomLeft, 2, shadows );
			RenderView( VIEW_BottomRight, 3, shadows );
			render->DivideQuadscreen();
		}

		if ( shadows )
		{
			render->EnterScreenSpace();
			render->RenderShadowQuad();
		}
	}

	enum View
	{
		VIEW_Fullscreen,
		VIEW_Left,
		VIEW_Right,
		VIEW_TopLeft,
		VIEW_TopRight,
		VIEW_BottomLeft,
		VIEW_BottomRight
	};

	void RenderView( View view, int activePlayer, bool shadows )
	{
		// setup for rendering...

		int width = render->GetDisplayWidth();
		int height = render->GetDisplayHeight();

		int cameraIndex = view == VIEW_Fullscreen ? 0 : activePlayer;

		setCameraAndLight( render, camera[cameraIndex] );

		float x1 = 0.0f;
		float y1 = 0.0f;
		float x2 = width;
		float y2 = height;

		const float s = height / 2;
		const float w = ( width - s*2 ) / 2;

		switch ( view )
		{
			case VIEW_Left:
				x1 = 0.0f;
				y1 = ( height - width/2 ) / 2;
				x2 = width/2;
				y2 = height - ( height - width/2 ) / 2;
				break;

			case VIEW_Right:
				x1 = width/2;
				y1 = ( height - width/2 ) / 2;
				x2 = width;
				y2 = height - ( height - width/2 ) / 2;
				break;

			case VIEW_BottomLeft:
				x1 = w;
				y1 = 0.0f; 
				x2 = w + s;
				y2 = s;
				break;

			case VIEW_BottomRight:
				x1 = w + s;
				y1 = 0.0f;
				x2 = w + s*2;
				y2 = s;
				break;

			case VIEW_TopLeft:
				x1 = w;
				y1 = s;
				x2 = w + s;
				y2 = s*2;
				break;

			case VIEW_TopRight:
				x1 = w + s;
				y1 = s;
				x2 = w + s*2;
				y2 = s*2;
				break;

			default:
				break;
		}

		render->BeginScene( x1, y1, x2, y2 );

		// render activation circles

		for ( int i = 0; i < MaxPlayers; ++i )
		{
			if ( vis == VIS_Normal && i != activePlayer )
				continue;
			ActivationArea activationArea;
			setupActivationArea( activationArea, origin[i], 5.0f, t );
			render->RenderActivationArea( activationArea, i == activePlayer ? 1.0f : 0.4f );
		}

		// show active player cubes (required for most views...)

		if ( vis != VIS_Merged )
		{
			Cubes cubes;
			viewObjectManager[activePlayer].GetRenderState( cubes );
			render->RenderCubes( cubes, 1.0f );
			if ( shadows )
				render->RenderCubeShadows( cubes );
		}

		// render ghost view, show other players views with alpha

		if ( vis == VIS_Ghosts )
		{
			view::ObjectManager * playerObjects[MaxPlayers];
			for ( int i = 0; i < MaxPlayers; ++i )
				playerObjects[i] = &viewObjectManager[i];
			view::ObjectManager mergedObjects;
			MergeViewObjectSets( playerObjects, MaxPlayers, mergedObjects, activePlayer, syncMode == SYNC_Naive );
				Cubes cubes;
			mergedObjects.GetRenderState( cubes );
			render->RenderCubes( cubes, 0.4f );
			if ( shadows )
				render->RenderCubeShadows( cubes );
		}

		// merge the view into one cohesive view, all cubes shown without alpha

		if ( vis == VIS_Merged )
		{
			bool blendColors = syncMode == SYNC_Naive;
			view::ObjectManager * playerObjects[MaxPlayers];
			for ( int i = 0; i < MaxPlayers; ++i )
				playerObjects[i] = &viewObjectManager[i];
			view::ObjectManager mergedObjects;
			MergeViewObjectSets( playerObjects, MaxPlayers, mergedObjects, activePlayer, blendColors );
				Cubes cubes;
			mergedObjects.GetRenderState( cubes );
			render->RenderCubes( cubes, 1.0f );
			if ( shadows )
				render->RenderCubeShadows( cubes );
		}
	}

	void PostRender()
	{
		// join worker threads
		for ( int i = 0; i < MaxPlayers; ++i )
			workerThread[i].Join();
	}
};
