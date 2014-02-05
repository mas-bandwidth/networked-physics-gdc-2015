/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#include "Hypercube.h"

class CorrectionsDemo : public AuthorityDemo
{
public:
	
	enum { GridSize = 1024 };
	enum { MaxObjectsInPacket = 256 };
	enum { MaxResponsesInPacket = 16 };
	
	enum CorrectionMode
	{
		MODE_NoSync,
		MODE_Authority,
		MODE_Corrections,
		MODE_ReverseCorrections
	};
	
	struct Response
	{
		ObjectId id;
		bool correction;
	};
	
	struct PlayerData
	{
		ResponseQueue<Response> responseQueue[MaxPlayers];
	};
	
	PlayerData playerData[MaxPlayers];
	
	CorrectionMode correctionMode;

	CorrectionsDemo( int displayWidth, int displayHeight )
		: AuthorityDemo( displayWidth, displayHeight )
	{
		correctionMode = MODE_NoSync;
		syncMode = SYNC_InteractionAuthority;
	}

	~CorrectionsDemo()
	{
		// ...
	}

	void Initialize()
	{
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			game::Config config;
			config.maxObjects = GridSize * GridSize + MaxPlayers + 1;
			config.deactivationTime = 0.5f;
			config.cellSize = 4.0f;
			config.cellWidth = GridSize / config.cellSize + 2;
			config.cellHeight = config.cellWidth;
			config.activationDistance = 5.0f;
			config.simConfig.ERP = 0.1f;
			config.simConfig.CFM = 0.001f;
			config.simConfig.MaxIterations = 12;
			config.simConfig.MaximumCorrectingVelocity = 100.0f;
			config.simConfig.ContactSurfaceLayer = 0.05f;
			config.simConfig.Elasticity = 0.3f;
			config.simConfig.LinearDrag = 0.01f;
			config.simConfig.AngularDrag = 0.01f;
			config.simConfig.Friction = 200.0f;

			game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * instance = 
				new game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> ( config );

			gameInstance[i] = instance;

			origin[i] = math::Vector(0,0,0);

			instance->SetFlag( game::FLAG_Hover );
			instance->SetFlag( game::FLAG_Katamari );

			instance->InitializeBegin();
			
			instance->AddPlane( math::Vector(0,0,1), 0 );
			
			AddCube( instance, 1, math::Vector(-5,+5,10) );
			AddCube( instance, 1, math::Vector(+5,+5,10) );
			AddCube( instance, 1, math::Vector(-5,-5,10) );
			AddCube( instance, 1, math::Vector(+5,-5,10) );

			const int border = 10.0f;
			const float origin = -GridSize / 2 + border;
			const float z = hypercube::NonPlayerCubeSize / 2;
			const int count = GridSize - border * 2;
			for ( int y = 0; y < count; ++y )
				for ( int x = 0; x < count; ++x )
					AddCube( instance, 0, math::Vector(x+origin,y+origin,z) );
			
			instance->InitializeEnd();

			for ( int j = 0; j < MaxPlayers; ++j )
			{
				instance->OnPlayerJoined( j );
				instance->SetPlayerFocus( j, j + 1 );
			}

			instance->SetLocalPlayer( i );
		}
	}

	void AddCube( game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * gameInstance, int player, const math::Vector & position )
	{
		hypercube::DatabaseObject object;
		CompressPosition( position, object.position );
		CompressOrientation( math::Quaternion(1,0,0,0), object.orientation );
		object.enabled = player;
		object.activated = 0;
		object.confirmed = 0;
		object.corrected = 0;
		object.player = player;
		gameInstance->AddObject( object, position.x, position.y );
	}

	void Update( float deltaTime )
	{
		/*
			Much easier during R&D to just have everything in one process,
			so here we fake some networking between our multiple game "instances".
			Note that the instances are still logically separate and the only
			communication between them is via this fake networking, so it does
			actually prove that this shit works! :)
		*/

		struct TempObject
		{
			ObjectId id;
			uint32_t enabled : 1;
			uint32_t authority : 1;
			uint32_t confirmed : 1;
			uint32_t defaultAuthority : 1;
			math::Vector position;
			math::Quaternion orientation;
			math::Vector linearVelocity;
			math::Vector angularVelocity;
		};
		
		struct TempResponse
		{
			ObjectId id;
			bool correction;
			bool enabled;
			math::Vector position;
			math::Quaternion orientation;
		};
		
		struct TempPacket
		{
			uint32_t frame;
			game::Input input;
			int objectCount;
			int responseCount;
			TempObject object[MaxObjectsInPacket];
			TempResponse response[MaxResponsesInPacket];
		};
		
		const int sendRate = 1;

		static engine::PacketQueue packetQueue;
		packetQueue.SetDelay( lag );

		// -----------------------------------------------------------------------------------------------
		// send packets
		// -----------------------------------------------------------------------------------------------

		bool sync = correctionMode != MODE_NoSync;

		if ( sync )
		{
			static int accumulator = 0;
			accumulator++;

			if ( sendRate > 0 && accumulator >= sendRate )
			{
				for ( int from = 0; from < MaxPlayers; ++from )
				{
					for ( int to = 0; to < MaxPlayers; ++to )
					{
						/*
							Packets are sent peer-to-peer to everybody
							but ourselves. Total bandwidth is O(n^2).
						*/
						if ( from == to )
							continue;
							
						game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * instance = 
							static_cast< game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject>* > ( gameInstance[from] );

						TempPacket packet;
					
						/*
							Grab our player input and frame time.
							These are both necessary for quality extrapolation
							in the remote view. Input is required to drive forces,
							and the frame is required for player control stuff 
							which includes functions of time - eg. sin(t) wobble...
						*/
						packet.frame = instance->GetPlayerFrame( from );
						instance->GetPlayerInput( from, packet.input );
					
						/*
							Our goal is to fit as many active objects into our packet
							But we usually cannot fit them all. So we have a simple
							prioritization system where we increase accumulate priority
							and reset to zero each time we send an object. High priority
							objects bubble up quicker, while still distributing updates
							across all active objects somewhat.
						*/
						packet.objectCount = MaxObjectsInPacket;
						if ( packet.objectCount > instance->GetActiveObjectCount() )
							packet.objectCount = instance->GetActiveObjectCount();
					
						for ( int i = 0; i < packet.objectCount; ++i )
						{
							const hypercube::ActiveObject & activeObject = instance->GetPriorityObject( to, i );
							instance->ResetObjectPriority( 0, i );
							packet.object[i].id = activeObject.id;
							const int localAuthority = instance->GetObjectAuthority( packet.object[i].id );
							packet.object[i].authority = localAuthority == from;
							packet.object[i].confirmed = activeObject.IsConfirmed( from );
							packet.object[i].defaultAuthority = localAuthority == MaxPlayers;
							packet.object[i].enabled = activeObject.enabled;
							packet.object[i].position = activeObject.position;
							packet.object[i].orientation = activeObject.orientation;
							packet.object[i].linearVelocity = activeObject.linearVelocity;
							packet.object[i].angularVelocity = activeObject.angularVelocity;
						}

						/*
							If we have any responses queued up from when we last
							received packets the player we are sending a packet to,
							this is the place where we pop these responses, and
							include them in the outgoing packet.
						*/
						ResponseQueue<Response> & responseQueue = playerData[from].responseQueue[to];
						int i = 0;
						while ( true )
						{
							Response response;
							if ( !responseQueue.PopResponse( response ) )
								break;
							packet.response[i].id = response.id;
							packet.response[i].correction = response.correction;
							if ( response.correction )
							{
								hypercube::ActiveObject activeObject;
								instance->GetObjectState( response.id, activeObject );
								packet.response[i].enabled = activeObject.enabled;
								packet.response[i].position = activeObject.position;
								packet.response[i].orientation = activeObject.orientation;
							}
							if ( ++i == MaxResponsesInPacket )
								break;
						}
						packet.responseCount = i;
								
						/*
							Now we fake "sending" the packet.
							This queue has no packet loss, but it does simulate latency
							for us, so we can see the effects of latency on our "networking"
						*/
						packetQueue.QueuePacket( from, to, (unsigned char*)&packet, sizeof(TempPacket) );
					}
				}
				
				accumulator = 0;
			}
		}

		// -----------------------------------------------------------------------------------------------
		// receive packets
		// -----------------------------------------------------------------------------------------------
		{
			packetQueue.Update( deltaTime );

			while ( engine::PacketQueue::Packet * pkt = packetQueue.PacketReadyToSend() )
			{
				TempPacket & packet = * (TempPacket*) &pkt->data[0];
				int from = pkt->sourceNodeId;
				int to = pkt->destinationNodeId;
				
				game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * instance = 
					static_cast< game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject>* > ( gameInstance[to] );
				
				/*
					Here we push the player input and frame time
					from the player who sent us the packet, such that
					the simulation for that player's cube will extrapolate
					holding the player's input constant.
				*/
				
				instance->SetPlayerInput( from, packet.input );
				instance->SetPlayerFrame( from, packet.frame );
				
				/*
					Now we push the state of the objects sent to us
					from the player. To "push" means that we simply
					apply the state included in the packet on top
					of our simulation which we have running locally.
					This is the other half of the "authority" and
					"correction" logic.
				*/
				
				if ( packet.objectCount > 0 )
				{
					for ( int i = 0; i < packet.objectCount; ++i )
					{
						/*
						 	Grab the local state for the object and
							store it in "activeObject". Note that the object
							may not in fact be active locally, in which case
							the active object is a stub...
						*/
						
						const ObjectId id = packet.object[i].id;
						
						hypercube::ActiveObject activeObject;
						instance->GetObjectState( id, activeObject );

						/*
							Decompress the quantities in the packet
							so that we have them in vector and quat form.
						*/

						const math::Vector & remotePosition = packet.object[i].position;
						const math::Quaternion & remoteOrientation = packet.object[i].orientation;

						/*
							Player flag is true if the current object is the player
							controlled cube of the player who sent the packet (from).
							If this is another player's cube then the player has
							no authority to ever tell us where that is, so we must
							ignore it!
						*/

						const bool player = packet.object[i].id == (ObjectId) ( from + 1 );
						
						if ( packet.object[i].id <= MaxPlayers && !player )
							continue;

						/*
							Here is where we queue up responses to object
							state sent to us. In general, we respond to 
							unconfirmed object state, so that the sender
 							has their state corrected or confirmed by us.
						*/
						
						bool confirmed = true;
										
						if ( correctionMode >= MODE_Corrections && !player )
						{
							confirmed = packet.object[i].confirmed;
							
							/*
								We only send responses to higher player ids
								than us because lower player ids do not care
								what we think (they were in the game before us).

								Also, we do not send responses if the object is
								currently active on our machine, because normal
								state updates make this redundant.
							*/
							if ( from > to && !confirmed && !instance->IsObjectActive( id ) )
							{
								Response response;
								response.id = id;
								response.correction = activeObject.activated && ( activeObject.position - remotePosition ).lengthSquared() > 0.01f;
								ResponseQueue<Response> & responseQueue = playerData[to].responseQueue[from];
								if ( !responseQueue.AlreadyQueued( id ) )
									responseQueue.QueueResponse( response );
							}
						}

						/*
							We only set the state of the object if this is the 
							player controlled cube for the "from" player, or if
							the object update is confirmed.
						*/

						if ( player || confirmed )
						{
							activeObject.position = remotePosition;
							activeObject.orientation = remoteOrientation;
							activeObject.enabled = packet.object[i].enabled;
							activeObject.linearVelocity = packet.object[i].linearVelocity;
							activeObject.angularVelocity = packet.object[i].angularVelocity;
							activeObject.SetConfirmed( from );

							if ( player )
							{
								/*
									Player authority means that we *always*
									accept the state update for the from player's
									player controlled cube.
								*/
								instance->SetObjectState( id, activeObject );
								instance->SetObjectAuthority( id, from, true );		// note: this "forces" authority to be set
							}
							else
							{
								/*
									This is a non-player cube
									Now we need to be much more selective
									about whether or not we can apply the state...
								*/
								int remoteAuthority = from;
								int localAuthority = instance->GetObjectAuthority( id );

								if ( packet.object[i].authority )
								{
									if ( remoteAuthority <= localAuthority )
									{
										/*
											Interaction authority means that we accept
											updates for objects which the player is interacting
											with, provided that the remote authority is less than
											(takes priority over) our local authority. Note that
											local authority will be "MaxPlayers" for objects
											which are not currently being interacted with.
										*/
										instance->SetObjectState( id, activeObject );
										if ( packet.object[i].enabled )
											instance->SetObjectAuthority( id, remoteAuthority );
									}
								}
								else if ( packet.object[i].defaultAuthority )
								{
									bool active = instance->IsObjectActive( activeObject.id );
									if ( active )
									{
										/*
											Tie break authority means that if the from player
											is not interacting with this object, and we are
											not interacting with it - we accept their update,
											but only if the from player has a lower player id
											than we do. This avoids feedback.
										*/
										if ( localAuthority == MaxPlayers && from < to )
											instance->SetObjectState( id, activeObject );
									
										/*
											Special case, if the sender doesn't have authority
											but the receiver thinks they do, we need to clear
											the object authority back to default.
										*/
										if ( localAuthority == from )
											instance->ClearObjectAuthority( id );
									}
									else
									{
										/*
											The object is inactive on our machine.
											Just accept the update. Technically we really
											should have some mechanism to ensure that we take
											the lowest player id update in preference when
											there are two players overlapping an area, but
											this seems to work well enough.
										*/
										instance->SetObjectState( id, activeObject );
									}
								}
							}
						}
					}
				}
				
				/*
					Responses are the reverse side of state updates
					This is where the receiving side gets the chance
					to confirm or apply corrections to the sent state.
				*/
				if ( correctionMode >= MODE_Corrections && from < to )
				{
					for ( int i = 0; i < packet.responseCount; ++i )
					{
						const TempResponse & response = packet.response[i];
						ObjectId id = response.id;
						hypercube::ActiveObject activeObject;
						instance->GetObjectState( id, activeObject );
						/*
							We may only apply the first correction
							we receive from each player. Subsequent
							corrections are ignored.
						*/
						if ( response.correction && !activeObject.IsConfirmedBitSet( from ) )
						{
							/*
								We may only apply a correction when the other
								lower player ids than us have not already applied
								one, because their correction take priority over us.
							*/
							if ( activeObject.CanApplyCorrection( from ) )
							{
								/*
									Corrections almost always occur when objects
									are in an incorrect position on a higher player id
									machine, while they are inactive on the lower id machine.
									Therefore we can optimize by not sending linear and
									angular velocities - they are almost always zero.
								*/
								activeObject.enabled = response.enabled;
								activeObject.position = response.position;
								activeObject.orientation = response.orientation;
								activeObject.linearVelocity = math::Vector(0,0,0);
								activeObject.angularVelocity = math::Vector(0,0,0);
							}
							activeObject.SetCorrected( from );
						}
						activeObject.SetConfirmed( from );
						instance->SetObjectState( id, activeObject );
					}
				}
				
				/*
					Important: Missing functionality!
					
					In order to keep the state completely in sync via P2P
					it is necessary to add the concept of reverse corrections.

					The basic idea of reverse corrections is that you keep track 
					of the set of objects you *expect* to be activated for other
					players, then you pre-emptively send state updates for those 
					objects which you think they have active, but have not received
					state updates for yet.
					
					This solves the case where the red player goes katamari and 
					dumps a whole load of cubes in the blue players circle while
					sync is OFF, then moves away, then you turn sync ON.
					
					Without reverse corrections the extra cubes do not show up 
					in the blue player's circle until the red player rolls
					over and overlaps them.

					No time to code this before MIGS'09 so I have to release
					the demo without it. Sorry guys!
					
					Glenn Fiedler <gaffer@gaffer.org>
				*/
				
				// todo: reverse corrections

				delete pkt;
			}
		}
		
		// -----------------------------------------------------------------------------------------------
		// update simulation
		// -----------------------------------------------------------------------------------------------

		for ( int i = 0; i < MaxPlayers; ++i )
		{
			gameInstance[i]->GetViewPacket( viewPacket[i] );
			workerThread[i].Start( gameInstance[i] );
		}
		
		t += deltaTime;
	}

	void Render( float deltaTime, bool shadows )
	{
		AuthorityDemo::Render( deltaTime, shadows );
	}

	void PostRender()
	{
		AuthorityDemo::PostRender();
	}

	void ProcessInput( const platform::Input & input )
	{
		AuthorityDemo::ProcessInput( input );
		
		syncMode = SYNC_InteractionAuthority;

		// demo controls

		if ( input.f1 )
			correctionMode = MODE_NoSync;
			
		if ( input.f2 )
			correctionMode = MODE_Authority;
			
		if ( input.f3 )
			correctionMode = MODE_Corrections;
			
		if ( input.f4 )
			correctionMode = MODE_ReverseCorrections;
	}
};
