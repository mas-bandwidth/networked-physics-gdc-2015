/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef GAME_H
#define GAME_H

#include "Config.h"
#include "Mathematics.h"
#if PLATFORM == PLATFORM_WINDOWS
	#include "stdint.h"
#else
	#include <stdint.h>
#endif
#include <stdio.h>

#include "Activation.h"
#include "Engine.h"
#include "Network.h"
#include "ViewObject.h"

namespace game
{
	using namespace net;
	using namespace engine;

	struct Input
	{
	 	float left;
		float right;
		float up;
		float down;
		float push;
		float pull;
	
		Input()
		{
			left = 0.0f;
			right = 0.0f;
			up = 0.0f;
			down = 0.0f;
	 		push = 0.0f;
			pull = 0.0f;
		}

		bool Serialize( net::Stream & stream )
		{
			stream.SerializeFloat( left );
			stream.SerializeFloat( right );
			stream.SerializeFloat( up );
			stream.SerializeFloat( down );
			stream.SerializeFloat( push );
			stream.SerializeFloat( pull );
			return true;
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
		Game Instance.
		Represents an instance of the game world.
	*/

	struct Config
	{
		float activationDistance;
		float deactivationTime;
		float cellSize;
 		int cellWidth;
		int cellHeight;
		float authorityTimeout;
		SimulationConfig simConfig;
		int maxObjects;
		int initialObjectsPerCell;
		int initialActiveObjects;

		Config()
		{
			activationDistance = 5.0f;
			deactivationTime = 0.0f;
			cellSize = 4.0f;
			cellWidth = 16;
			cellHeight = 16;
			authorityTimeout = 0.1f;
			maxObjects = 1024;
			initialObjectsPerCell = 32;
			initialActiveObjects = 256;
		}
	};

	enum Flag
	{
		FLAG_Pause,
		FLAG_Hover,
		FLAG_Katamari,
		FLAG_Quantize,
		FLAG_DisableInteractionAuthority
	};

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

	template <typename DatabaseObject, typename ActiveObject> class Instance : public Interface
	{
	public:
		
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
		
		Instance( const Config & config = Config() )
		{
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
		
		enum World
		{
			WORLD_PlayersOnly,
			WORLD_Grid
		};
		
		void InitializeBegin()
		{
			assert( !initialized );
			initializing = true;
			printf( "initializing game world\n" );
		}
				
		void AddObject( DatabaseObject & object, float x, float y )
		{
			int id = objectCount + 1;
			assert( id < config.maxObjects );
			objects[id] = object;
			activationSystem->InsertObject( id, x, y );
			objectCount++;
		}

		void AddPlane( const math::Vector & normal, float d )
		{
			assert( initializing );
			simulation->AddPlane( normal, d );
		}

		void InitializeEnd()
		{
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
			initialized = true;
			initializing = false;
		}
		
		void Shutdown()
		{
			assert( initialized );
			objectCount = 0;
			activeObjects.Clear();
			authorityManager.Clear();
			interactionManager.ClearInteractions();
			simulation->Reset();
			initialized = false;
			for ( int i = 0; i < MaxPlayers; ++i )
			{
				prioritySet[i].Clear();
				force[i] = math::Vector(0,0,0);
				joined[i] = false;
				playerFocus[i] = 0;
			}
		}
		
		void OnPlayerJoined( int playerId )
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			assert( !joined[playerId] );
			joined[playerId] = true;
		}
		
		void OnPlayerLeft( int playerId )
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			assert( joined[playerId] );
			if ( localPlayerId == playerId )
				localPlayerId = -1;
			joined[playerId] = false;
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

		void Update( float deltaTime = 1.0f / 60.0f )
		{
			for ( int i = 0; i < MaxPlayers; ++i )
				ProcessPlayerInput( i, deltaTime );

			Validate();

			MoveOriginPoint();

			UpdateActivation( deltaTime );
			
			UpdatePriority( deltaTime );
			
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
		
		void GetActiveObjects( ActiveObject * objects, int & count )
		{
			count = activeObjects.GetCount();
			for ( int i = 0; i < count; ++i )
			{
				ActiveObject & activeObject = activeObjects.GetObject( i );
				*objects = activeObject;
				objects++;
			}
		}
		
		int GetActiveObjectCount() const
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
		
		int GetObjectAuthority( ObjectId id )
		{
			assert( id > 0 );
			assert( id <= (ObjectId) objectCount );
			return authorityManager.GetAuthority( id );
		}
		
		int SetObjectAuthority( ObjectId id, int authority, bool force = false )
		{
			assert( id > 0 );
			assert( id <= (ObjectId) objectCount );
			return authorityManager.SetAuthority( id, authority, force );
		}
		
		void ClearObjectAuthority( ObjectId id )
		{
			assert( id > 0 );
			assert( id <= (ObjectId) objectCount );
			authorityManager.RemoveAuthority( id );
		}

		void GetObjectState( ObjectId id, ActiveObject & object )
		{
			assert( id > 0 );
			assert( id <= (ObjectId) objectCount );
			// active object
			int count = activeObjects.GetCount();
			for ( int i = 0; i < count; ++i )
			{
				ActiveObject * activeObject = &activeObjects.GetObject( i );
				assert( activeObject );
				if ( activeObject->id == id )
				{
					object = *activeObject;
					return;
				}
			}
			// inactive object
			objects[id].DatabaseToActive( object );
			object.activeId = 0;							// todo: i need a way to signal that this is an inactive object
			object.id = id;
		}

		void SetObjectState( ObjectId id, const ActiveObject & object )
		{
			assert( id > 0 );
			assert( id <= (ObjectId) objectCount );
			// active object
			int count = activeObjects.GetCount();
			for ( int i = 0; i < count; ++i )
			{
				ActiveObject & activeObject = activeObjects.GetObject( i );
				if ( activeObject.id == id )
				{
					ActiveId activeId = activeObject.activeId;
					const bool warp = ( activeObject.position - object.position ).lengthSquared() > 25.0f;
					activeObject = object;
					activeObject.activeId = activeId;
					activeObject.framesSinceLastUpdate = 0;
					activationSystem->MoveObject( id, activeObject.position.x, activeObject.position.y, warp );
					return;
				}
			}
			// inactive object
			objects[id].ActiveToDatabase( object );
			activationSystem->MoveObject( id, object.position.x, object.position.y );
		}
		
		const ActiveObject & GetPriorityObject( int playerId, int index )
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			assert( index >= 0 );
			assert( index < activeObjects.GetCount() );
			ObjectId id = prioritySet[playerId].GetPriorityObject( index );
			const ActiveObject * activeObject = activeObjects.FindObject( id );
			assert( activeObject );
			return *activeObject;
		}
		
		void ResetObjectPriority( int playerId, int index )
		{
			assert( playerId >= 0 );
			assert( playerId < MaxPlayers );
			assert( index >= 0 );
			assert( index < activeObjects.GetCount() );
			prioritySet[playerId].SetPriorityAtIndex( index, 0.0f );
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
				float f = 120.0f;
				
				force[playerId].x -= f * input[playerId].left;
				force[playerId].x += f * input[playerId].right;
				force[playerId].y += f * input[playerId].up;
				force[playerId].y -= f * input[playerId].down;

				if ( input[playerId].push < 0.001f )
				{
					if ( activePlayerObject->linearVelocity.dot( force[playerId] ) < 0 )
						force[playerId] *= 2.0f;
				}

 				ActiveId playerActiveId = activePlayerObject->activeId;

				simulation->ApplyForce( playerActiveId, force[playerId] );

				if ( input[playerId].push > 0.0f && GetFlag( FLAG_Hover ) )
				{
					// hovering force
					math::Vector origin = activePlayerObject->position;
					origin.z = -0.1f;
					for ( int j = 0; j < activeObjects.GetCount(); ++j )
					{
						ActiveObject * activeObject = &activeObjects.GetObject( j );
						math::Vector difference = activeObject->position - origin;
						if ( activeObject == activePlayerObject )
							difference.z -= 0.7f;
						float distanceSquared = difference.lengthSquared();
						float distance = math::sqrt( distanceSquared );
						if ( distance > 0.01f && distance < 2.0f )
						{
							math::Vector direction = difference / distance;
							float magnitude = 1.0f / distanceSquared * 200.0f;
							if ( activeObject == activePlayerObject && input[playerId].pull )
							{
								magnitude *= 10.0f;
								if ( magnitude > 2000.0f )
									magnitude = 2000.0f;
							}
							else
							{
								if ( magnitude > 1000.0f )
									magnitude = 1000.0f;
							}
							math::Vector force = direction * magnitude;
							float mass = simulation->GetObjectMass( activeObject->activeId );
							if ( mass > 1.0f )
								mass = 1.0f;
							int authority = authorityManager.GetAuthority( activeObject->id );
							if ( playerId <= authority )
							{
								simulation->ApplyForce( activeObject->activeId, force * mass );
								authorityManager.SetAuthority( activeObject->id, playerId );
							}
						}
					}
					// bobbing force
					{
						float wobble_x = sin(frame[playerId]*0.1+1) + sin(frame[playerId]*0.05+3) + sin(frame[playerId]+10.0);
						float wobble_y = sin(frame[playerId]*0.1+2) + sin(frame[playerId]*0.05+4) + sin(frame[playerId]+11.0);
						float wobble_z = sin(frame[playerId]*0.1+3) + sin(frame[playerId]*0.05+5) + sin(frame[playerId]+12.0);
						math::Vector force = math::Vector( wobble_x, wobble_y, wobble_z ) * 2.0f;
						simulation->ApplyForce( activePlayerObject->activeId, force );
					}
					// bobbing torque
					{
						float wobble_x = sin(frame[playerId]*0.1+10) + sin(frame[playerId]*0.05+22) + sin(static_cast<double>(frame[playerId]));
						float wobble_y = sin(frame[playerId]*0.09+5) + sin(frame[playerId]*0.045+16) + sin(static_cast<double>(frame[playerId]));
						float wobble_z = sin(frame[playerId]*0.11+4) + sin(frame[playerId]*0.055+9) + sin(static_cast<double>(frame[playerId]));
						math::Vector torque = math::Vector( wobble_x, wobble_y, wobble_z ) * 1.5f;
						simulation->ApplyTorque( activePlayerObject->activeId, torque );
					}
					// calculate velocity tilt
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
					// stay upright torque
					{
						math::Vector currentUp = activePlayerObject->orientation.transform( math::Vector(0,0,1) );
						math::Vector axis = targetUp.cross( currentUp );
			 			float angle = math::acos( targetUp.dot( currentUp ) );
						if ( angle > 0.5f )
							angle = 0.5f;
						math::Vector torque = - 100 * axis * angle;
						simulation->ApplyTorque( activePlayerObject->activeId, torque );
					}
					// apply damping
					{
						activePlayerObject->angularVelocity *= 0.95f;
						activePlayerObject->linearVelocity.x *= 0.96f;
						activePlayerObject->linearVelocity.y *= 0.96f;
						activePlayerObject->linearVelocity.z *= 0.999f;
					}
				}

				if ( input[playerId].pull > 0.0f && GetFlag( FLAG_Katamari ) )
				{
					if ( input[playerId].push < 0.001f )
					{
						// katamari force
						math::Vector origin = activePlayerObject->position;
						for ( int j = 0; j < activeObjects.GetCount(); ++j )
						{
							ActiveObject * activeObject = &activeObjects.GetObject( j );
							assert( activeObject );
							if ( activeObject == activePlayerObject )
								continue;
							math::Vector difference = activeObject->position - origin;
							float distanceSquared = difference.lengthSquared();
							const float effectiveRadius = 1.1f + input[playerId].pull * 0.5f;
							if ( distanceSquared > 0.2f*0.2f && distanceSquared < effectiveRadius * effectiveRadius )
							{
								float distance = math::sqrt( distanceSquared );
								math::Vector direction = difference / distance;
								float magnitude = 1.0f / distanceSquared * 200.0f * input[playerId].pull * input[playerId].pull;
								if ( magnitude > 2000.0f )
									magnitude = 2000.0f;
								math::Vector force = - direction * magnitude;
								float mass = simulation->GetObjectMass( activeObject->activeId );
								int authority = authorityManager.GetAuthority( activeObject->id );
								if ( authority == playerId || authority == MaxPlayers )
								{
									simulation->ApplyForce( activeObject->activeId, force * mass );
									authorityManager.SetAuthority( activeObject->id, playerId );
								}
							}
						}
					}
					else
					{
						// repel explosion force!
						math::Vector origin = activePlayerObject->position;
						for ( int j = 0; j < activeObjects.GetCount(); ++j )
						{
							ActiveObject * activeObject = &activeObjects.GetObject( j );
							assert( activeObject );
							if ( activeObject == activePlayerObject )
								continue;
							math::Vector difference = activeObject->position - origin;
							float distanceSquared = difference.lengthSquared();
							if ( distanceSquared > 0.2f*0.2f && distanceSquared < 4.0f*4.0f )
							{
								float distance = math::sqrt( distanceSquared );
								math::Vector direction = difference / distance;
								float magnitude = 1.0f / distanceSquared * 250.0f * input[playerId].pull * input[playerId].push;
								if ( magnitude > 5000.0f )
									magnitude = 5000.0f;
								math::Vector force = direction * magnitude;
								float mass = simulation->GetObjectMass( activeObject->activeId );
								int authority = authorityManager.GetAuthority( activeObject->id );
								if ( authority == playerId || authority == MaxPlayers )
								{
									simulation->ApplyForce( activeObject->activeId, force * mass );
									authorityManager.SetAuthority( activeObject->id, playerId );
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
		
		void UpdateActivation( float deltaTime )
		{
			activationSystem->SetEnabled( InGame() );
			activationSystem->MoveActivationPoint( origin.x, origin.y );
			activationSystem->Update( deltaTime );

			int eventCount = activationSystem->GetEventCount();

			for ( int i = 0; i < eventCount; ++i )
			{
				const activation::Event & event = activationSystem->GetEvent(i);
				if ( event.type == activation::Event::Activate )
				{
					ActiveObject * activeObject = &activeObjects.InsertObject( event.id );
					assert( activeObject );
					objects[event.id].activated = true;
					objects[event.id].DatabaseToActive( *activeObject );

					SimulationObjectState simInitialState;
					activeObject->ActiveToSimulation( simInitialState );
					activeObject->id = event.id;
					activeObject->activeId = simulation->AddObject( simInitialState );

					for ( int i = 0; i < MaxPlayers; ++i )
						prioritySet[i].AddObject( activeObject->id );
				}
				else
				{
					ActiveObject * activeObject = activeObjects.FindObject( event.id );
					assert( activeObject );
					objects[event.id].ActiveToDatabase( *activeObject );
					for ( int i = 0; i < MaxPlayers; ++i )
						prioritySet[i].RemoveObject( activeObject->id );
					authorityManager.RemoveAuthority( activeObject->id );
					simulation->RemoveObject( activeObject->activeId );
					activeObjects.DeleteObject( *activeObject );
				}
			}

			activationSystem->ClearEvents();	
		}
		
		void UpdatePriority( float deltaTime )
		{
			int numActiveObjects = activeObjects.GetCount();
			for ( int playerId = 0; playerId < MaxPlayers; ++playerId )
			{
				for ( int i = 0; i < numActiveObjects; ++i )
				{
					float scale = 1.0f;
				
					ObjectId id = prioritySet[playerId].GetPriorityObject( i );
					const ActiveObject * activeObject = activeObjects.FindObject( id );
					assert( activeObject );
				
					if ( authorityManager.GetAuthority( id ) == localPlayerId )
						scale *= 2.0f;
					
					if ( !activeObject->enabled )
						scale *= 0.25f;
				
					float priority = prioritySet[playerId].GetPriorityAtIndex( i );
				
					priority += deltaTime * scale;

					if ( activeObject->id == playerFocus[localPlayerId] )
						priority = 1000000.0f;
					
					const float distanceSquared = ( activeObject->position - origin ).lengthSquared();
					if ( distanceSquared > config.activationDistance * config.activationDistance )
						priority = 0.0f;

					prioritySet[playerId].SetPriorityAtIndex( i, priority );
				}
				prioritySet[playerId].SortObjects();
			}
		}
		
		void UpdateSimulation( float deltaTime )
		{
			int numActiveObjects = activeObjects.GetCount();

			for ( int i = 0; i < numActiveObjects; ++i )
			{
				ActiveObject * activeObject = &activeObjects.GetObject( i );
				assert( activeObject );
				if ( activeObject->framesSinceLastUpdate < 255 )
					activeObject->framesSinceLastUpdate++;
				SimulationObjectState objectState;
				activeObject->ActiveToSimulation( objectState );
				simulation->SetObjectState( activeObject->activeId, objectState, true );
			}
			
			if ( GetFlag( FLAG_Pause ) )
				return;
				
			simulation->Update( deltaTime );

			for ( int i = 0; i < numActiveObjects; ++i )
			{
				ActiveObject * activeObject = &activeObjects.GetObject( i );
				assert( activeObject );
				
				SimulationObjectState simObjectState;
				simulation->GetObjectState( activeObject->activeId, simObjectState );
				
				activeObject->SimulationToActive( simObjectState );

				// clamp state
				const float bound_x = activationSystem->GetBoundX();
				const float bound_y = activationSystem->GetBoundY();
				activeObject->Clamp( bound_x, bound_y );
				
				// todo: quantize state
				
				/*
				// quantize state
				if ( GetFlag( FLAG_Quantize ) )
				{
					uint64_t compressed_position;
					uint32_t compressed_orientation;
					CompressPosition( objectState.position, compressed_position );
					CompressOrientation( objectState.orientation, compressed_orientation );
					DecompressPosition( compressed_position, objectState.position );
					DecompressOrientation( compressed_orientation, objectState.orientation );
					// todo: quantize linear velocity, angular velocity...
				}
				*/
				
				float x,y;
				activeObject->GetPositionXY( x, y );
				activationSystem->MoveObject( activeObject->id, x, y );
			}
		}
		
		void ConstructViewPacket()
		{
			ActiveObject * localPlayerActiveObject = activeObjects.FindObject( playerFocus[localPlayerId] );
			if ( localPlayerActiveObject )
			{
				viewPacket.origin = origin;
				viewPacket.objectCount = 1;

				localPlayerActiveObject->ActiveToView( viewPacket.object[0], localPlayerId, activationSystem->IsPendingDeactivation( localPlayerActiveObject->id ), localPlayerActiveObject->framesSinceLastUpdate );

				int index = 1;
				for ( int i = 0; i < activeObjects.GetCount() && index < MaxViewObjects; ++i )
				{
					ActiveObject * activeObject = &activeObjects.GetObject( i );
					assert( activeObject );
					if ( activeObject == localPlayerActiveObject )
						continue;
					activeObject->ActiveToView( viewPacket.object[index], authorityManager.GetAuthority( activeObject->id ), activationSystem->IsPendingDeactivation( activeObject->id ), activeObject->framesSinceLastUpdate );
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
			// update authority timeout

			authorityManager.Update( deltaTime, config.authorityTimeout );

			// force authority for player controlled objects

			for ( int i = 0; i < MaxPlayers; ++i )
			{
				if ( !joined[i] )
					continue;
					
				int playerObjectId = GetPlayerFocus( i );
					
				authorityManager.SetAuthority( playerObjectId, i, true );
			}
			
			// make sure enabled objects keep their current authority until at rest
			
			for ( int i = 0; i < activeObjects.GetCount(); ++i )
			{
				ActiveObject & activeObject = activeObjects.GetObject( i );
				if ( activeObject.enabled )
				{
					int authority = authorityManager.GetAuthority( activeObject.id );
					if ( authority != MaxPlayers )
						authorityManager.SetAuthority( activeObject.id, authority );
				}
			}

			// interaction based authority for active objects

			if ( !GetFlag( FLAG_DisableInteractionAuthority ) )
			{
				if ( InGame() )
				{
					/*
						We need to work out what the maximum active id 
						is across all active objects. This can be higher than 
						the current size of the active object set, because the 
						active id is determined via a slot based system and
						remains the same until the object deactivates.
					*/
					int maxActiveId = 0;
					for ( int i = 0; i < activeObjects.GetCount(); ++i )
					{
						ActiveObject & activeObject = activeObjects.GetObject( i );
						if ( activeObject.activeId > (ActiveId) maxActiveId )
							maxActiveId = activeObject.activeId;
					}

					/*
						Here we walk over each player in turn
						determining which objects they are interacting with.
						These objects they are interacting with have their
						authority changed to that player id where possible.
					*/
					for ( int playerId = 0; playerId < MaxPlayers; ++playerId )
					{
						/*
							First we clear all interactions
							and prep for handling interactions
							in the range [0,maxActiveId].
						*/
						interactionManager.ClearInteractions();
						interactionManager.PrepInteractions( maxActiveId );

						/*
							When we are walking interactions we wish to ignore
							any objects that are not default authority (MaxPlayers)
							and are not the authority of the current player for whom we are
							walking interactions. This is important to avoid transmitting
							authority through objects of a different color, for example
							a red cube pushing a blue cube into a white cube should
							*not* turn the white cube red.
						*/
						std::vector<bool> ignores( maxActiveId + 1, false );
						for ( int i = 0; i < activeObjects.GetCount(); ++i )
						{
							ActiveObject & activeObject = activeObjects.GetObject( i );
							int authority = authorityManager.GetAuthority( activeObject.id );
							if ( authority != MaxPlayers && authority != playerId )
								ignores[activeObject.activeId] = true;
							/*
								HACK: we also ignore objects which are currently at rest
								this is a workraround for the fact that the ODE sim does
								not seem capable of determining resting contact for a
								large pile of cubes as a whole with the iterative solver.
								Note that we have to special case player cubes here,
								we never want those ignored, even when at rest...
							*/
							if ( !activeObject.enabled && !activeObject.IsPlayer() )
								ignores[activeObject.activeId] = true;
						}

						/*
							The physics simulation keeps track of the set of interaction
							pairs, eg. object X interacted with Y. There is only one unique
							pair per-interaction, eg. a->b, or b->a, but not both.
						*/	
						int numInteractionPairs = simulation->GetNumInteractionPairs();
						const InteractionPair * interactionPairs = simulation->GetInteractionPairs();

						/*				
							The basic algorithm is to walk starting at the set of
							objects which have player authority, transmitting this
							authority to other objects along the chain of interactions.
							Ignore objects break the chain. See the "WalkInteractions"
							function in Engine.h for details.
						*/
						for ( int i = 0; i < activeObjects.GetCount(); ++i )
						{
							ActiveObject & activeObject = activeObjects.GetObject( i );
							int authority = authorityManager.GetAuthority( activeObject.id );
							if ( authority == playerId )
								interactionManager.WalkInteractions( activeObject.activeId, interactionPairs, numInteractionPairs, ignores );
						}

						/*
							Now once we have the full set of interacting objects,
							meaning the set of objects which are in contact with
							player id authority objects, without going through
							ignore objects -- we can now mark all of these as
							being under authority of the current player id.
						*/
						for ( int i = 0; i < activeObjects.GetCount(); ++i )
						{
							ActiveObject & activeObject = activeObjects.GetObject( i );
							if ( activeObject.enabled && interactionManager.IsInteracting( activeObject.activeId ) )
								authorityManager.SetAuthority( activeObject.id, playerId );
						}
						
						/*
							Safety check: print out the active ids of objects
							which are trivially interacting with the player,
							but did not have their interacting bit set.
						*/
						#ifdef DEBUG
						if ( playerId == 0 )
						{
 							int playerObjectId = GetPlayerFocus( playerId );
							ActiveObject * playerActiveObject = activeObjects.FindObject( playerObjectId );
							if ( playerActiveObject )
							{
								for ( int i = 0; i < activeObjects.GetCount(); ++i )
								{
									ActiveObject & activeObject = activeObjects.GetObject( i );
									if ( activeObject.activeId == playerActiveObject->activeId )
										continue;
									for ( int j = 0; j < numInteractionPairs; ++j )
									{
										if ( interactionPairs[j].a == (int) playerActiveObject->activeId && interactionPairs[j].b == (int) activeObject.activeId ||
											 interactionPairs[j].b == (int) playerActiveObject->activeId && interactionPairs[j].a == (int) activeObject.activeId )
										{
											if ( !interactionManager.IsInteracting( activeObject.activeId ) )
												printf( "interaction failure: %d\n", (int) activeObject.activeId );
										}
									}
								}
							}
						}
						#endif
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
		
		activation::Set<ActiveObject> activeObjects;
		PrioritySet prioritySet[MaxPlayers];
		AuthorityManager authorityManager;
		InteractionManager interactionManager;

		view::Packet viewPacket;

		DatabaseObject * objects;
	};
}
	
#endif
