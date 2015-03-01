/*
	Networked Physics Demo
	Copyright © 2008-2015 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef CUBES_GAME_H
#define CUBES_GAME_H

#include "Config.h"
#include "Mathematics.h"
#include "Activation.h"
#include "Engine.h"
#include "ViewObject.h"
#include "vectorial/vec3f.h"

namespace game
{
	using namespace cubes;

	struct Input
	{
		bool left;
		bool right;
		bool up;
		bool down;
		bool push;
		bool pull;
	
		Input()
		{
			left = 0;
			right = 0;
			up = 0;
			down = 0;
			push = 0;
			pull = 0;
		}

		bool operator == ( const Input & other )
		{
			return left == other.left    &&
				  right == other.right   &&
				     up == other.up      &&
				   down == other.down    &&
				   push == other.push    &&
				   pull == other.pull;
		}
		
		bool operator != ( const Input & other )
		{
			return ! ( (*this) == other );
		}
	};

	/*	
		Configure game instance.
		Allows us to have multiple game worlds in one application.
	*/

	struct Config
	{
		SimulationConfig simConfig;
		float activationDistance;
		float deactivationTime;
		float cellSize;
 		int cellWidth;
		int cellHeight;
		int maxObjects;
		int initialObjectsPerCell;
		int initialActiveObjects;
		float z_min;
		float z_max;

		Config()
		{
			activationDistance = 5.0f;
			deactivationTime = 0.0f;
			cellSize = 4.0f;
			cellWidth = 16;
			cellHeight = 16;
			maxObjects = 1024;
			initialObjectsPerCell = 64;
			initialActiveObjects = 1024;
			z_min = 0.0f;
			z_max = 100.0f;
		}
	};
	
	/*
		Flags enable/disable various bits of game code that we may not
		always want enabled in different demos or test programs.
	*/

	enum Flag
	{
		FLAG_Pause,
		FLAG_Push,
		FLAG_Pull,
		FLAG_DisableInteractionAuthority
	};
	
	/*
		Game listener lets us follow when objects are activated/deactivated
		This is necessary to keep track of priority sets of objects.
	*/
	
	class Listener
	{
	public:
		
		virtual ~Listener() {}
		virtual void OnObjectActivated( ObjectId id, int activeIndex ) {}
		virtual void OnObjectDeactivated( ObjectId id, int activeIndex ) {}
	};

	/*
		Generic interface to maniplate game world instances.
		(Required because we currently use lots of template fuckery...)
	*/

	class Interface
	{
	public:
		
		virtual ~Interface() {}
		virtual void Update( float deltaTime ) = 0;
		virtual void SetPlayerInput( int playerId, const Input & input ) = 0;
		virtual void GetViewPacket( view::Packet & viewPacket ) = 0;
		virtual void SetFlag( Flag flag ) = 0;
		virtual void ClearFlag( Flag flag ) = 0;
		virtual bool GetFlag( Flag flag ) const = 0;
	};

	/*
		Game instance.
		Lets us have multiple game world instances running in the same app.
		(Templates let us have different database and active object structs.)
	*/

	template <typename DatabaseObject, typename ActiveObject> class Instance : public Interface
	{
	public:
		
		void SetListener( Listener * listener )
		{
			if ( listener )
				this->listener = listener;
			else
				this->listener = &dummyListener;
		}
		
		void SetFlag( Flag flag )
		{
			flags |= ( 1 << flag );
		}
		
		void ClearFlag( Flag flag )
		{
			flags &= ~( 1 << flag );			
		}
		
		bool GetFlag( Flag flag ) const
		{
			return ( flags & ( 1 << flag ) ) != 0;
		}
		
		const Config & GetConfig() const
		{
			return config;
		}
		
		Instance( const Config & config = Config() )
		{
			this->listener = &dummyListener;
			this->config = config;
			initialized = false;
			initializing = false;
			flags = 0;
			activationSystem = new ActivationSystem( config.maxObjects, config.activationDistance, config.cellWidth, config.cellHeight, config.cellSize, config.initialObjectsPerCell, config.initialActiveObjects, config.deactivationTime );
			simulation = new Simulation();
			simulation->Initialize( config.simConfig );
			objects = new DatabaseObject[config.maxObjects];
			objectCount = 0;
			localPlayerId = -1;
			origin = math::Vector(0,0,0);
			for ( int i = 0; i < MaxPlayers; ++i )
			{
				joined[i] = false;
				force[i] = math::Vector(0,0,0);
				frame[i] = 0;
				playerFocus[i] = 0;
			}
			activeObjects.Allocate( config.initialActiveObjects );
		}
		
		~Instance()
		{
			if ( initialized )
				Shutdown();
			delete [] objects;
			delete simulation;
			delete activationSystem;
		}
		
		void InitializeBegin()
		{
			assert( !initialized );
			initializing = true;
//			printf( "initializing game world\n" );
		}
				
		ObjectId AddObject( DatabaseObject & object, float x, float y )
		{
			int id = objectCount + 1;
			assert( id < config.maxObjects );
			objects[id] = object;
			activationSystem->InsertObject( id, x, y );
			objectCount++;
			return id;
		}

		void AddPlane( const math::Vector & normal, float d )
		{
			assert( initializing );
			simulation->AddPlane( normal, d );
		}

		void InitializeEnd()
		{
			/*
			if ( objectCount > 0 )
			{
				if ( objectCount > 1 )
					printf( "created %d objects\n", objectCount );
				else
					printf( "created 1 object\n" );
				
				printf( "active object: %d bytes\n", (int) sizeof( ActiveObject ) );
				printf( "database object: %d bytes\n", (int) sizeof( DatabaseObject ) );
				printf( "activation cell: %d bytes\n", (int) sizeof( activation::Cell ) );
				
				printf( "active set is %.1fKB\n", activeObjects.GetBytes() / ( 1000.0f ) );

				const int objectDatabaseBytes = sizeof(DatabaseObject) * objectCount;
				if ( objectDatabaseBytes >= 1000000 )
					printf( "object database is %.1fMB\n", objectDatabaseBytes / ( 1000.0f*1000.0f ) );
				else
					printf( "object database is %.1fKB\n", objectDatabaseBytes / ( 1000.0f ) );

				const int activationSystemBytes = activationSystem->GetBytes();
				if ( activationSystemBytes >= 1000000 )
					printf( "activation system is %.1fMB\n", activationSystemBytes / ( 1000.0f*1000.0f ) );
				else
					printf( "activation system is %.1fKB\n", activationSystemBytes / ( 1000.0f ) );
			}
			*/
			
			initialized = true;
			initializing = false;
		}
		
		void Shutdown()
		{
			assert( initialized );
			objectCount = 0;
			activeObjects.Clear();
			simulation->Reset();
			initialized = false;
			for ( int i = 0; i < MaxPlayers; ++i )
			{
				force[i] = math::Vector(0,0,0);
				joined[i] = false;
				playerFocus[i] = 0;
			}
		}
		
		void EnableObject( ObjectId objectId )
		{
			activationSystem->EnableObject( objectId );
		}
		
		void DisableObject( ObjectId objectId )
		{
			activationSystem->DisableObject( objectId );
		}
		
		void OnPlayerJoined( int playerId )
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			assert( !joined[playerId] );
			
			joined[playerId] = true;

			activationSystem->EnableObject( (ObjectId) playerId + 1 );
		}
		
		void OnPlayerLeft( int playerId )
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			assert( joined[playerId] );
			
			if ( localPlayerId == playerId )
				localPlayerId = -1;
			
			joined[playerId] = false;

			activationSystem->DisableObject( (ObjectId) playerId + 1 );
		}
		
		bool IsPlayerJoined( int playerId ) const
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			return joined[playerId];
		}
		
		void SetLocalPlayer( int playerId )
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			localPlayerId = playerId;
		}
		
		void SetPlayerFocus( int playerId, ObjectId objectId )
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			assert( objectId > 0 );
			assert( objectId <= (ObjectId) objectCount );
			playerFocus[playerId] = objectId;
		}
		
 		ObjectId GetPlayerFocus( int playerId ) const
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			return playerFocus[playerId];
		}
		
		uint32_t GetPlayerFrame( int playerId ) const
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			return frame[playerId];
		}

		void SetPlayerFrame( int playerId, uint32_t frame )
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			this->frame[playerId] = frame;
		}
		
		bool InGame() const
		{
			return ( localPlayerId >= 0 && IsPlayerJoined( localPlayerId ) );
		}
		
		int GetLocalPlayer() const
		{
			return localPlayerId;
		}
	
		void SetPlayerInput( int playerId, const Input & input )
		{
			this->input[playerId] = input;
		}
			
		void GetPlayerInput( int playerId, Input & input )
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			input = this->input[playerId];
		}
		
		const math::Vector & GetOrigin() const
		{
			return origin;
		}

		void Update( float deltaTime = 0.1f )
		{
			for ( int i = 0; i < MaxPlayers; ++i )
				ProcessPlayerInput( i, deltaTime );

			Validate();

			MoveOriginPoint();

			UpdateActivation( deltaTime );
			
			UpdateSimulation( deltaTime );
			
			UpdateAuthority( deltaTime );

			ConstructViewPacket();

			Validate();

			for ( int i = 0; i < MaxPlayers; ++i )
				frame[i]++;
		}
		
		void GetViewPacket( view::Packet & viewPacket )
		{
			viewPacket = this->viewPacket;
		}
		
		void CopyActiveObjects( ActiveObject * objects, int & count )
		{
			// IMPORTANT: slow copy of all active objects (for testing only)
			count = activeObjects.GetCount();
			for ( int i = 0; i < count; ++i )
			{
				ActiveObject & activeObject = activeObjects.GetObject( i );
				*objects = activeObject;
				objects++;
			}
		}
		
		void MoveActiveObject( ActiveObject * activeObject )
		{
			assert( activeObject );
			int activeIndex = activeObject - &activeObjects.GetObject( 0 );
			float x,y;
			activeObject->GetPositionXY( x, y );
			activationSystem->MoveActiveObject( activeIndex, x, y );
		}
		
		void MoveDatabaseObject( ObjectId id, float x, float y )
		{
			assert( id > 0 );
			assert( id < (ObjectId) config.maxObjects );
			activationSystem->MoveDatabaseObject( id, x, y );
		}
		
		ActiveObject * FindActiveObject( ObjectId id )
		{
			assert( id > 0 );
			assert( id < (ObjectId) config.maxObjects );
			return activeObjects.FindObject( id );
		}
		
		DatabaseObject & GetDatabaseObject( ObjectId id )
		{
			assert( id > 0 );
			assert( id < (ObjectId) config.maxObjects );
			return objects[id];
		}
		
		ActiveObject & GetActiveObject( int activeIndex )
		{
			assert( activeIndex >= 0 );
			assert( activeIndex < activeObjects.GetCount() );
			return activeObjects.GetObject( activeIndex );
		}
		
		ActiveObject * GetActiveObjects()
		{
			return &activeObjects.GetObject(0);
		}
		
		int GetNumActiveObjects() const
		{
			return activeObjects.GetCount();
		}
		
		bool IsObjectActive( ObjectId id )
		{
			assert( activationSystem );
			assert( id > 0 );
			assert( id <= (ObjectId) objectCount );
			return activationSystem->IsActive( id );
		}
		
		void GetObjectState( ObjectId id, ActiveObject & object )
		{
			assert( id > 0 );
			assert( id <= (ObjectId) objectCount );
			ActiveObject * activeObject = activeObjects.FindObject( id );
			if ( activeObject )
			{
				// active object
				object = *activeObject;
			}
			else
			{
				// inactive object
				objects[id].DatabaseToActive( object );
				object.activeId = 0;
				object.id = id;
			}
		}

		void SetObjectState( ObjectId id, const ActiveObject & object )
		{
			assert( id > 0 );
			assert( id <= (ObjectId) objectCount );
			ActiveObject * activeObject = activeObjects.FindObject( id );
			if ( activeObject )
			{
				// active object
				ActiveId activeId = activeObject->activeId;
				*activeObject = object;
				activeObject->activeId = activeId;
				activationSystem->MoveActiveObject( activeId, object.position.x, object.position.y );
			}
			else
			{
				// inactive object
				objects[id].ActiveToDatabase( object );
				activationSystem->MoveDatabaseObject( id, object.position.x, object.position.y );
			}
		}
				
	protected:
		
		void ProcessPlayerInput( int playerId, float deltaTime )
		{
			if ( GetFlag( FLAG_Pause ) )
				return;
			
			force[playerId] = math::Vector(0,0,0);

			if ( !IsPlayerJoined( playerId ) )
				return;

			int playerObjectId = playerFocus[playerId];

			ActiveObject * activePlayerObject = activeObjects.FindObject( playerObjectId );

			if ( activePlayerObject )
			{			
				// apply strafing force to player cube
				
				float f = 120.0f;
				
				force[playerId].x -= f * (int) input[playerId].left;
				force[playerId].x += f * (int) input[playerId].right;
				force[playerId].y += f * (int) input[playerId].up;
				force[playerId].y -= f * (int) input[playerId].down;

				if ( input[playerId].push < 0.001f )
				{
					if ( activePlayerObject->linearVelocity.dot( force[playerId] ) < 0 )
						force[playerId] *= 2.0f;
				}

 				ActiveId playerActiveId = activePlayerObject->activeId;

				simulation->ApplyForce( playerActiveId, force[playerId] );

				const bool push = input[playerId].push > 0.0f && GetFlag( FLAG_Push );
				const bool pull = input[playerId].pull > 0.0f && GetFlag( FLAG_Pull );

				if ( push )
				{
					// bobbing force on player cube
					{
						float wobble_x = sin(frame[playerId]*0.1+1) + sin(frame[playerId]*0.05f+3) + sin(frame[playerId]+10);
						float wobble_y = sin(frame[playerId]*0.1+2) + sin(frame[playerId]*0.05f+4) + sin(frame[playerId]+11);
						float wobble_z = sin(frame[playerId]*0.1+3) + sin(frame[playerId]*0.05f+5) + sin(frame[playerId]+12);
						math::Vector force = math::Vector( wobble_x, wobble_y, wobble_z ) * 2.0f;
						simulation->ApplyForce( activePlayerObject->activeId, force );
					}
					
					// bobbing torque on player cube
					{
						float wobble_x = sin(frame[playerId]*0.05+10) + sin(frame[playerId]*0.05f+22) + sin(frame[playerId]*1.1f);
						float wobble_y = sin(frame[playerId]*0.09+5) + sin(frame[playerId]*0.045f+16) + sin(frame[playerId]*1.11f);
						float wobble_z = sin(frame[playerId]*0.05+4) + sin(frame[playerId]*0.055f+9) + sin(frame[playerId]*1.12f);
						math::Vector torque = math::Vector( wobble_x, wobble_y, wobble_z ) * 1.5f;
						simulation->ApplyTorque( activePlayerObject->activeId, torque );
					}
					
					// calculate velocity tilt for player cube
					math::Vector targetUp(0,0,1);
					{
						math::Vector velocity = force[playerId];
						velocity.z = 0.0f;
						float speed = velocity.length();
						if ( speed > 0.001f )
						{
							math::Vector axis = -force[playerId].cross( math::Vector(0,0,1) );
							axis.normalize();
							float tilt = speed * 0.1f;
							if ( tilt > 0.25f )
								tilt = 0.25f;
							targetUp = math::Quaternion( tilt, axis ).transform( targetUp );
						}
					}
					
					// stay upright torque on player cube
					{
						math::Vector currentUp = activePlayerObject->orientation.transform( math::Vector(0,0,1) );
						math::Vector axis = targetUp.cross( currentUp );
			 			float angle = math::acos( targetUp.dot( currentUp ) );
						if ( angle > 0.5f )
							angle = 0.5f;
						math::Vector torque = - 100 * axis * angle;
						simulation->ApplyTorque( activePlayerObject->activeId, torque );
					}
					
					// apply damping to player cube
					activePlayerObject->angularVelocity *= 0.95f;
					activePlayerObject->linearVelocity.x *= 0.96f;
					activePlayerObject->linearVelocity.y *= 0.96f;
					activePlayerObject->linearVelocity.z *= 0.999f;
				}
				
				math::Vector push_origin = activePlayerObject->position;
				push_origin.z = -0.1f;

				// iterate across active objects
				if ( push || pull )
				{
					for ( int i = 0; i < activeObjects.GetCount(); ++i )
					{
						ActiveObject * activeObject = &activeObjects.GetObject( i );

						const int authority = activeObject->authority;
						const float mass = simulation->GetObjectMass( activeObject->activeId );

						if ( push )
						{
							math::Vector difference = activeObject->position - push_origin;
							if ( activeObject == activePlayerObject )
								difference.z -= 0.7f;
							float distanceSquared = difference.lengthSquared();
							if ( distanceSquared > 0.01f * 0.01f && distanceSquared < 4.0f )
							{
								float distance = math::sqrt( distanceSquared );
								math::Vector direction = difference / distance;
								float magnitude = 1.0f / distanceSquared * 200.0f;
								if ( magnitude > 500.0f )
									magnitude = 500.0f;
								math::Vector force = direction * magnitude;
								if ( activeObject != activePlayerObject )
									force *= mass;
								else if ( pull )
									force *= 20;

								if ( authority == MaxPlayers || playerId == authority )
								{
									simulation->ApplyForce( activeObject->activeId, force );
									activeObject->authority = playerId;
									activeObject->authorityTime = 0;
								}
							}
						}
						else if ( pull )
						{
							if ( activeObject != activePlayerObject )
							{
								math::Vector difference = activeObject->position - origin;
								float distanceSquared = difference.lengthSquared();
								const float effectiveRadiusSquared = 1.8f * 1.8f;
								if ( distanceSquared > 0.2f*0.2f && distanceSquared < effectiveRadiusSquared )
								{
									float distance = math::sqrt( distanceSquared );
									math::Vector direction = difference / distance;
									float magnitude = 1.0f / distanceSquared * 200.0f;
									if ( magnitude > 2000.0f )
										magnitude = 2000.0f;
									math::Vector force = - direction * magnitude;
									if ( authority == playerId || authority == MaxPlayers )
									{
										simulation->ApplyForce( activeObject->activeId, force * mass );
										activeObject->authority = playerId;
										activeObject->authorityTime = 0;
									}
								}
							}
						}
					}
				}
			}
		}
		
		void MoveOriginPoint()
		{
			if ( InGame() )
			{
				int playerObjectId = playerFocus[localPlayerId];

				ActiveObject * activePlayerObject = activeObjects.FindObject( playerObjectId );

				if ( activePlayerObject )
					activePlayerObject->GetPosition( origin );
				else
					objects[playerObjectId].GetPosition( origin );
			}
			else
			{
				origin = math::Vector(0,0,0);
			}
		}		
		
		void Validate()
		{
			#ifdef DEBUG
			for ( int i = 0; i < activeObjects.GetCount(); ++i )
			{
				ActiveObject & activeObject = activeObjects.GetObject( i );
				assert( activeObject.id >= 1 );
				assert( activeObject.id <= (ObjectId) objectCount );
			}
			#endif
			activationSystem->Validate();
		}
		
	public:
		
		void ProcessActivationEvents()
		{
			int eventCount = activationSystem->GetEventCount();

			for ( int i = 0; i < eventCount; ++i )
			{
				const activation::Event & event = activationSystem->GetEvent(i);
				if ( event.type == activation::Event::Activate )
				{
					ActiveObject * activeObject = &activeObjects.InsertObject( event.id );
					assert( activeObject );
					objects[event.id].DatabaseToActive( *activeObject );

					listener->OnObjectActivated( event.id, (int) ( activeObject - &activeObjects.GetObject(0) ) );

					SimulationObjectState simInitialState;
					activeObject->ActiveToSimulation( simInitialState );
					activeObject->id = event.id;
					activeObject->activeId = simulation->AddObject( simInitialState );
				}
				else
				{
					ActiveObject * activeObject = activeObjects.FindObject( event.id );
					assert( activeObject );

					listener->OnObjectDeactivated( event.id, (int) ( activeObject - &activeObjects.GetObject(0) ) );

					objects[event.id].ActiveToDatabase( *activeObject );
						
					simulation->RemoveObject( activeObject->activeId );
					activeObjects.DeleteObject( activeObject );
				}
			}

			activationSystem->ClearEvents();	
		}
		
	protected:
		
		void UpdateActivation( float deltaTime )
		{
			activationSystem->SetEnabled( InGame() );
			activationSystem->MoveActivationPoint( origin.x, origin.y );
			activationSystem->Update( deltaTime );
			ProcessActivationEvents();
		}
		
		void UpdateSimulation( float deltaTime )
		{
			int numActiveObjects = activeObjects.GetCount();

			for ( int i = 0; i < numActiveObjects; ++i )
			{
				ActiveObject * activeObject = &activeObjects.GetObject( i );
				assert( activeObject );
				SimulationObjectState objectState;
				activeObject->ActiveToSimulation( objectState );
				simulation->SetObjectState( activeObject->activeId, objectState, true );
			}
			
			simulation->Update( deltaTime, GetFlag( FLAG_Pause ) );

			if ( GetFlag( FLAG_Pause ) )
				return;
				
            const vectorial::vec3f position_min( -PositionBoundXY, -PositionBoundXY, 0 );
            const vectorial::vec3f position_max( +PositionBoundXY, +PositionBoundXY, PositionBoundZ );

			for ( int i = 0; i < numActiveObjects; ++i )
			{
				ActiveObject * activeObject = &activeObjects.GetObject( i );
				assert( activeObject );
				
				SimulationObjectState simObjectState;
				simulation->GetObjectState( activeObject->activeId, simObjectState );
				
				activeObject->SimulationToActive( simObjectState );

				vectorial::vec3f position( activeObject->position.x, activeObject->position.y, activeObject->position.z );

				position = vectorial::clamp( position, position_min, position_max );

				activeObject->position = math::Vector( position.x(), position.y(), position.z() );

				float x,y;
				activeObject->GetPositionXY( x, y );
				int activeIndex = activeObject - &activeObjects.GetObject( 0 );
				activationSystem->MoveActiveObject( activeIndex, x, y );
			}
		}
		
		void ConstructViewPacket()
		{
			ActiveObject * localPlayerActiveObject = activeObjects.FindObject( playerFocus[localPlayerId] );
			if ( localPlayerActiveObject )
			{
				viewPacket.origin = origin;
				viewPacket.objectCount = 1;

				localPlayerActiveObject->ActiveToView( viewPacket.object[0], localPlayerId, false );

				int index = 1;
				for ( int i = 0; i < activeObjects.GetCount() && index < MaxViewObjects; ++i )
				{
					ActiveObject * activeObject = &activeObjects.GetObject( i );
					assert( activeObject );
					if ( activeObject == localPlayerActiveObject )
						continue;
					int activeIndex = activeObject - &activeObjects.GetObject( 0 );
					const bool pendingDeactivation = activationSystem->IsPendingDeactivation( activeIndex );
					activeObject->ActiveToView( viewPacket.object[index], activeObject->authority, pendingDeactivation );
					index++;
				}

				viewPacket.objectCount = index;
				assert( viewPacket.objectCount >= 0 );
				assert( viewPacket.objectCount <= MaxViewObjects );
			}
			else
			{
				viewPacket = view::Packet();
			}
		}
				
		void UpdateAuthority( float deltaTime )
		{
			// update authority timeout + force authority for any active player cubes
			
			int maxActiveId = 0;
			for ( int i = 0; i < activeObjects.GetCount(); ++i )
			{
				ActiveObject & activeObject = activeObjects.GetObject( i );
				if ( activeObject.id >= 1 && activeObject.id <= (uint32_t) MaxPlayers )
				{
					activeObject.authority = activeObject.id - 1;
					activeObject.authorityTime = 0;
				}
				else if ( activeObject.enabled )
				{
					activeObject.authorityTime = 0;
				}
				else
				{
					activeObject.authorityTime++;
					if ( activeObject.authorityTime > 100 )
						activeObject.authority = MaxPlayers;
				}
				if ( activeObject.activeId > (ActiveId) maxActiveId )
					maxActiveId = activeObject.activeId;
			}

			// build map from active id to active index
			
			std::vector<int> activeIdToIndex( maxActiveId + 1 );
			for ( int i = 0; i < activeObjects.GetCount(); ++i )
			{
				ActiveObject & activeObject = activeObjects.GetObject( i );
				activeIdToIndex[activeObject.activeId] = i;
			}
			
			// interaction based authority for active objects

			if ( InGame() && !GetFlag( FLAG_DisableInteractionAuthority ) )
			{
				for ( int playerId = 0; playerId < MaxPlayers; ++playerId )
				{
					std::vector<bool> interacting( maxActiveId + 1, false );
					std::vector<bool> ignores( maxActiveId + 1, false );
					std::vector<int> queue( activeObjects.GetCount() );
					int head = 0;
					int tail = 0;

					for ( int i = 0; i < activeObjects.GetCount(); ++i )
					{
						ActiveObject & activeObject = activeObjects.GetObject( i );
						const int activeId = activeObject.activeId;
						assert( activeId >= 0 );
						assert( activeId < (int) ignores.size() );
						assert( activeId < (int) interacting.size() );
						if ( activeObject.authority == playerId )
						{
							interacting[activeId] = true;
							assert( head < activeObjects.GetCount() );
							queue[head++] = activeObject.activeId;
						}
						else if ( activeObject.authority != MaxPlayers || activeObject.enabled == 0 )
						{
							ignores[activeId] = true;
						}
					}
					
					while ( head != tail )
					{
						const std::vector<uint16_t> & objectInteractions = simulation->GetObjectInteractions( queue[tail] );
						for ( int i = 0; i < (int) objectInteractions.size(); ++i )
						{
							const int activeId = objectInteractions[i];
							assert( activeId >= 0 );
							assert( activeId < (int) interacting.size() );
							if ( !ignores[activeId] && !interacting[activeId] )
							{
								assert( head < activeObjects.GetCount() );
								queue[head++] = activeId;
							}
							interacting[activeId] = true;
						}
						tail++;
					}
					
					for ( int i = 0; i <= maxActiveId; ++i )
					{
						if ( interacting[i] )
						{
							int index = activeIdToIndex[i];
							ActiveObject & activeObject = activeObjects.GetObject( index );
							if ( activeObject.authority == MaxPlayers || activeObject.authority == playerId )
							{
								activeObject.authority = playerId;
								if ( activeObject.enabled )
									activeObject.authorityTime = 0;
							}
						}
					}
				}
			}
		}
		
	private:
		
		Config config;

		uint32_t flags;
		
		bool initialized;
		bool initializing;
		bool joined[MaxPlayers];
		Input input[MaxPlayers];
		ObjectId playerFocus[MaxPlayers];
		
		uint32_t frame[MaxPlayers];

		int objectCount;
		int localPlayerId;

		math::Vector origin;
		math::Vector force[MaxPlayers];

		Simulation * simulation;
		ActivationSystem * activationSystem;
		
		DatabaseObject * objects;
		
		Listener * listener;
		Listener dummyListener;

        activation::Set<ActiveObject> activeObjects;

        view::Packet viewPacket;
	};
}
	
#endif
