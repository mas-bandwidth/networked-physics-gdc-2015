/*
	Authority Management Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "ode/ode.h"
#include "Platform.h"
#include "Display.h"
#include "Mathematics.h"
#include "NetStream.h"
//#include "NetTransport.h"

using namespace std;
using namespace net;

// ------------------------------------------------------------------------------

// constants

const int DisplayWidth = 640;
const int DisplayHeight = 480;

const float ShadowDistance = 10.0f;
const float ColorChangeTightness = 0.1f;

const float RestTime = 1.0f;
const float LinearRestThreshold = 0.01f;
const float AngularRestThreshold = 0.01f;

const float AuthorityTimeOut = 1.0f;

const int MaxCubes = 64;
const int MaxPlayers = 2;
const int PlayerBits = 1;
const float CubeSize = 2.0f;
const float CubeMass = 0.1f;
const float PlayerStrafeForce = 30.0f;
const float PlayerMaximumStrafeSpeed = 10.0f;
const float PlayerMass = 1.0f;
const float Boundary = 10.0f;
const float Gravity = 20.0f;
const float Friction = 30.0f;
const int MaxIterations = 20;
const float ERP = 0.2f;
const float CFM = 0.05f;
const float MaximumCorrectingVelocity = 100.0f;
const float ContactSurfaceLayer = 0.001f;

#define USE_QUICK_STEP
#define CUBE_DISPLAY_LIST
#define FLOOR_AND_WALLS_DISPLAY_LIST
#define MULTITHREADED
#define VSYNC
#define RENDER_SHADOWS
//#define DEBUG_SHADOW_VOLUMES
//#define CAMERA_FOLLOW

// ------------------------------------------------------------------------------

// abstracted render state (decouple sim and render)

struct RenderState
{
	RenderState()
	{
		localPlayerId = -1;
		numCubes = 0;
	}
	
	struct Player
	{
		bool exists;
		math::Vector position;
		
		Player()
		{
			exists = false;
			position = math::Vector(0,0,0);
		}
	};
	
	struct Cube
	{		
 		math::Vector position;
 		math::Quaternion orientation;
		float scale;
		float r,g,b;
	};

	int localPlayerId;
	Player player[MaxPlayers];
	int numCubes;
	Cube cubes[MaxCubes];
};

// ------------------------------------------------------------------------------

// abstracted simulation data (decouple sim and networking)

struct SimulationPlayerInput
{
	SimulationPlayerInput()
	{
		left = false;
		right = false;
		forward = false;
		back = false;
	}
	
	bool left;
	bool right;
	bool forward;
	bool back;
};

struct SimulationPlayerState
{
	SimulationPlayerState()
	{
		cubeId = -1;
	}
	
	int cubeId;
};

struct SimulationCubeState
{
	SimulationCubeState()
	{
		enabled = false;
		position = math::Vector(0,0,0);
		linearVelocity = math::Vector(0,0,0);
		angularVelocity = math::Vector(0,0,0);
		orientation = math::Quaternion(1,0,0,0);
	}
	
	bool enabled;
 	math::Vector position;
 	math::Vector linearVelocity;
 	math::Vector angularVelocity;
 	math::Quaternion orientation;	
};

struct SimulationCubeContacts
{
	SimulationCubeContacts()
	{
		memset( this, 0, sizeof( SimulationCubeContacts ) );
	}
	
	bool touching[MaxCubes];
};

struct SimulationPlayerInteractions
{
	SimulationPlayerInteractions()
	{
		memset( this, 0, sizeof( SimulationCubeContacts ) );
	}
	
	bool interacting[MaxCubes];
};

struct SimulationState
{
	SimulationState()
	{
		for ( int i = 0; i < MaxPlayers; ++i )
			playerExists[i] = false;
		numCubes = 0;
	}

	bool playerExists[MaxPlayers];

	SimulationPlayerInput playerInput[MaxPlayers];
	SimulationPlayerState playerState[MaxPlayers];

	int numCubes;	
	SimulationCubeState cubeState[MaxCubes];
	SimulationCubeContacts cubeContacts[MaxCubes];
	SimulationPlayerInteractions playerInteractions[MaxPlayers];
};

// physics simulation (ODE)

class PhysicsSimulation
{	
public:
	
	PhysicsSimulation()
	{
		// create simulation

		world = dWorldCreate();
		space = dHashSpaceCreate( 0 );
	    contacts = dJointGroupCreate( 0 );
		
		// configure world
		
		dWorldSetERP( world, ERP );
		dWorldSetCFM( world, CFM );
		dWorldSetQuickStepNumIterations( world, MaxIterations );
		dWorldSetGravity( world, 0, -Gravity, 0 );
		dWorldSetContactSurfaceLayer( world, ContactSurfaceLayer );
		dWorldSetContactMaxCorrectingVel( world, MaximumCorrectingVelocity );
		dWorldSetLinearDamping( world, 0.001f );
		dWorldSetAngularDamping( world, 0.001f );
		dWorldSetAutoDisableFlag( world, false );
		
		// add boundary planes
		
		dCreatePlane( space, 0, 1, 0, 0 );						// floor
		dCreatePlane( space, +1, 0, 0, -Boundary );				// left wall
		dCreatePlane( space, -1, 0, 0, -Boundary );				// right wall
		dCreatePlane( space, 0, 0, +1, -Boundary );				// front wall
		dCreatePlane( space, 0, 0, -1, -Boundary );				// back wall
		
		// add some cubes to the world

		numCubes = 0;

		srand( 280090412 );

		AddCube( math::Vector(-5,10,0), 1 );
		AddCube( math::Vector(+5,10,0), 1 );

		AddCube( math::Vector(0,1,0), 0.9f );
		AddCube( math::Vector(0,4,0), 0.9f );
		AddCube( math::Vector(0,8,0), 0.9f );

		for ( int i = 0; i < 30; ++i )
		{
			float x = math::random_float( -Boundary, +Boundary );
			float y = math::random_float( 1.0f, 4.0f );
			float z = math::random_float( -Boundary, +Boundary );
			float scale = math::random_float( 0.1f, 0.6f );

			AddCube( math::Vector(x,y,z), scale );
		}
		
		// players have not joined yet
		
		for ( int i = 0; i < MaxPlayers; ++i )
			playerData[i].exists = false;
	}

	~PhysicsSimulation()
	{
		for ( int i = 0; i < numCubes; ++i )
			if ( cubes[i].fixedJoint )
				dJointDestroy( cubes[i].fixedJoint );
		
		dJointGroupDestroy( contacts );
		dSpaceDestroy( space );
		dWorldDestroy( world );
	}
	
	void Update( float deltaTime )
	{			
		// process player input
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			PlayerData & player = playerData[i];
			
			if ( player.exists )
			{
				// apply strafe forces
				
				if ( player.input.left || player.input.right || player.input.forward || player.input.back )
				{
					assert( player.cubeId >= 0 );
					assert( player.cubeId < MaxCubes );
				
					dBodyID body = cubes[player.cubeId].body;

					const dReal * linearVelocity = dBodyGetLinearVel( body );

					math::Vector velocity( linearVelocity[0], linearVelocity[1], linearVelocity[2] );
					
					float fx = 0.0f;
					float fz = 0.0f;
					
					if ( velocity.x < PlayerMaximumStrafeSpeed )
						fx += PlayerStrafeForce * player.input.right;
					
					if ( velocity.x > -PlayerMaximumStrafeSpeed )
						fx -= PlayerStrafeForce * player.input.left;
					
					if ( velocity.z < PlayerMaximumStrafeSpeed )
						fz += PlayerStrafeForce * player.input.forward;
					
					if ( velocity.z > -PlayerMaximumStrafeSpeed )
						fz -= PlayerStrafeForce * player.input.back;
				
					dBodyAddForce( body, fx, 0.0f, fz );

					dBodyEnable( body );
				}
			}
		}

		// detect collisions
		        
		for ( int i = 0; i < numCubes; ++i )
			cubeContacts[i] = SimulationCubeContacts();
			
		dJointGroupEmpty( contacts );
		
		dSpaceCollide( space, this, NearCallback );

		DeterminePlayerInteractions();
	
		// step world forward

		#ifdef USE_QUICK_STEP
		dWorldQuickStep( world, deltaTime );
		#else
		dWorldStep( world, deltaTime );
		#endif
	}
		
	void GetRenderState( RenderState & renderState )
	{
		renderState.localPlayerId = -1;		// note: this is filled in by the network game
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			renderState.player[i].exists = playerData[i].exists;
			
			if ( playerData[i].exists )
			{
				const dReal * position = dBodyGetPosition( cubes[playerData[i].cubeId].body );
				renderState.player[i].position = math::Vector( position[0], position[1], position[2] );
			}
		}
		
		assert( numCubes <= MaxCubes );
		
		renderState.numCubes = numCubes;

		for ( int i = 0; i < numCubes; ++i )
		{
			assert( cubes[i].body );
			
			const dReal * position = dBodyGetPosition( cubes[i].body );
			const dReal * orientation = dBodyGetQuaternion( cubes[i].body );
		
			renderState.cubes[i].position = math::Vector( position[0], position[1], position[2] );
			renderState.cubes[i].orientation = math::Quaternion( orientation[0], orientation[1], orientation[2], orientation[3] );
			renderState.cubes[i].scale = cubes[i].scale * CubeSize * 0.5f;

			assert( MaxPlayers == 2 );

			bool interacting[2] = { false, false };
			
			for ( int j = 0; j < MaxPlayers; ++j )
			{
				if ( playerInteractions[j].interacting[i] )
					interacting[j] = true;
			}

			if ( playerData[0].cubeId == i )
			{
				renderState.cubes[i].r = 1.0f;
				renderState.cubes[i].g = 0.0f;
				renderState.cubes[i].b = 0.0f;
			}
			else if ( playerData[1].cubeId == i )
			{
				renderState.cubes[i].r = 0.0f;
				renderState.cubes[i].g = 0.0f;
				renderState.cubes[i].b = 1.0f;
			}
			else
			{	
				renderState.cubes[i].r = 1.0f;
				renderState.cubes[i].g = 1.0f;
				renderState.cubes[i].b = 1.0f;
			}
		}
	}
		
	void GetSimulationState( SimulationState & simulationState )
	{
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			if ( playerData[i].exists )
			{
				simulationState.playerExists[i] = true;
				simulationState.playerInput[i] = playerData[i].input;
				simulationState.playerState[i].cubeId = playerData[i].cubeId;
			}
			else
			{
				simulationState.playerExists[i] = false;
			}
		}
		
		simulationState.numCubes = numCubes;
		
		for ( int i = 0; i < numCubes; ++i )
		{
			assert( cubes[i].body );

			const dReal * position = dBodyGetPosition( cubes[i].body );
			const dReal * orientation = dBodyGetQuaternion( cubes[i].body );
			const dReal * linearVelocity = dBodyGetLinearVel( cubes[i].body );
			const dReal * angularVelocity = dBodyGetAngularVel( cubes[i].body );

			simulationState.cubeState[i].enabled = dBodyIsEnabled( cubes[i].body ) != 0;
			simulationState.cubeState[i].position = math::Vector( position[0], position[1], position[2] );
			simulationState.cubeState[i].orientation = math::Quaternion( orientation[0], orientation[1], orientation[2], orientation[3] );
			simulationState.cubeState[i].linearVelocity = math::Vector( linearVelocity[0], linearVelocity[1], linearVelocity[2] );
			simulationState.cubeState[i].angularVelocity = math::Vector( angularVelocity[0], angularVelocity[1], angularVelocity[2] );
		}

		memcpy( &simulationState.cubeContacts, &cubeContacts, sizeof( SimulationCubeContacts ) * MaxCubes );
		memcpy( &simulationState.playerInteractions, &playerInteractions, sizeof( SimulationPlayerInteractions ) * MaxPlayers );
	}
	
	void SetSimulationState( SimulationState & simulationState )
	{
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			if ( playerData[i].exists && simulationState.playerExists[i] )
				playerData[i].input = simulationState.playerInput[i];
		}
		
		assert( simulationState.numCubes == numCubes );

		for ( int i = 0; i < numCubes; ++i )
		{
			const SimulationCubeState & cubeState = simulationState.cubeState[i];

			dQuaternion quaternion;
			quaternion[0] = cubeState.orientation.w;
			quaternion[1] = cubeState.orientation.x;
			quaternion[2] = cubeState.orientation.y;
			quaternion[3] = cubeState.orientation.z;

			dBodySetPosition( cubes[i].body, cubeState.position.x, cubeState.position.y, cubeState.position.z );
			dBodySetQuaternion( cubes[i].body, quaternion );
			dBodySetLinearVel( cubes[i].body, cubeState.linearVelocity.x, cubeState.linearVelocity.y, cubeState.linearVelocity.z );
			dBodySetAngularVel( cubes[i].body, cubeState.angularVelocity.x, cubeState.angularVelocity.y, cubeState.angularVelocity.z );

			dBodySetForce( cubes[i].body, 0, 0, 0 );
			dBodySetTorque( cubes[i].body, 0, 0, 0 );

			if ( cubeState.enabled )
				dBodyEnable( cubes[i].body );
			else
				dBodyDisable( cubes[i].body );
		}
	}
	
	void SetPlayerInput( int playerId, const SimulationPlayerInput & input )
	{
		assert( playerId >= 0 );
		assert( playerId < MaxPlayers );
		assert( playerData[playerId].exists );
		playerData[playerId].input = input;
	}

	void OnPlayerJoin( int playerId )
	{
		assert( playerId >= 0 );
		assert( playerId < MaxPlayers );

		assert( playerData[playerId].exists == false );
		
		printf( "simulation: player %d joined\n", playerId );

		PlayerPossessCube( playerId, playerId );
		
		playerData[playerId].exists = true;
	}
	
	void OnPlayerLeft( int playerId )
	{
		assert( playerId >= 0 );
		assert( playerId < MaxPlayers );
		
		assert( playerData[playerId].exists == true );
		
		printf( "simulation: player %d left\n", playerId );
		
		if ( playerData[playerId].cubeId >= 0 )
			PlayerUnpossessCube( playerId, playerData[playerId].cubeId );
			
		playerData[playerId] = PlayerData();
	}
	
protected:

	void AddCube( const math::Vector & position, float scale )
	{
		assert( numCubes >= 0 );
		assert( numCubes < MaxCubes );
		
		// setup cube body
		
		cubes[numCubes].body = dBodyCreate( world );
		
		assert( cubes[numCubes].body );

		dMass mass;
		dMassSetBox( &mass, 1, 1, 1, 1 );
		dMassAdjust( &mass, CubeMass * scale );
		dBodySetMass( cubes[numCubes].body, &mass );

		dBodySetPosition( cubes[numCubes].body, position.x, position.y, position.z );

		dBodySetData( cubes[numCubes].body, &cubes[numCubes] );

		// setup cube geom and set body

		cubes[numCubes].scale = scale;
		cubes[numCubes].geom = dCreateBox( space, scale * CubeSize, scale * CubeSize, scale * CubeSize );
		dGeomSetBody( cubes[numCubes].geom, cubes[numCubes].body );	

		numCubes++;
	}
	
	void PlayerPossessCube( int playerId, int cubeId )
	{
		assert( playerId >= 0 );
		assert( playerId < MaxPlayers );
		assert( cubeId >= 0 );
		assert( cubeId < MaxCubes );
		
		playerData[playerId].cubeId = cubeId;
		cubes[cubeId].ownerPlayerId = playerId;
		
		assert( cubes[cubeId].body );
			
		dMass mass;
		dMassSetBox( &mass, 1, 1, 1, 1 );
		dMassAdjust( &mass, PlayerMass * cubes[cubeId].scale );
		dBodySetMass( cubes[cubeId].body, &mass );
	}
	
	void PlayerUnpossessCube( int playerId, int cubeId )
	{
		assert( playerId >= 0 );
		assert( playerId < MaxPlayers );
		assert( cubeId >= 0 );
		assert( cubeId < MaxCubes );
		
		cubes[cubeId].ownerPlayerId = -1;
		playerData[playerId].cubeId = -1;
		
		dMass mass;
		dMassSetBox( &mass, 1, 1, 1, 1 );
		dMassAdjust( &mass, CubeMass * cubes[cubeId].scale );
		dBodySetMass( cubes[cubeId].body, &mass );
	}

	void DeterminePlayerInteractions()
	{
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			playerInteractions[i] = SimulationPlayerInteractions();
			if ( playerData[i].exists )
			{
				int cubeId = playerData[i].cubeId;
				RecurseInteractions( playerInteractions[i], cubeId );
			}
		}
	}
	
 	void RecurseInteractions( SimulationPlayerInteractions & playerInteractions, int cubeId )
	{
		assert( cubeId >= 0 );
		assert( cubeId < MaxCubes );
		
		for ( int i = 0; i < numCubes; ++i )
		{
			if ( cubeContacts[cubeId].touching[i] && !playerInteractions.interacting[i] )
			{
				playerInteractions.interacting[i] = true;
				RecurseInteractions( playerInteractions, i );
			}
		}		
	}	

private:
	
	dWorldID world;
	dSpaceID space;
	dJointGroupID contacts;
	
	struct PlayerData
	{
		bool exists;
		int cubeId;
		SimulationPlayerInput input;
		
		PlayerData()
		{
			exists = false;
			cubeId = 0;
		}
	};

	PlayerData playerData[MaxPlayers];

	struct CubeData
	{
		dBodyID body;
		dGeomID geom;
		float scale;
		int parentCubeId;
		dJointID fixedJoint;
		int ownerPlayerId;
		
		CubeData()
		{
			body = 0;
			geom = 0;
			scale = 1.0f;
			parentCubeId = -1;
			fixedJoint = 0;
			ownerPlayerId = -1;
		}
	};
	
	int numCubes;
	CubeData cubes[MaxCubes];
	SimulationCubeContacts cubeContacts[MaxCubes];
	SimulationPlayerInteractions playerInteractions[MaxPlayers];

protected:
		
	static void NearCallback( void * data, dGeomID o1, dGeomID o2 )
	{
		PhysicsSimulation * simulation = (PhysicsSimulation*) data;
	
		assert( simulation );
	
	    dBodyID b1 = dGeomGetBody( o1 );
	    dBodyID b2 = dGeomGetBody( o2 );

		const int MaxContacts = 4;

	    dContact contact[MaxContacts];			
	    for ( int i = 0; i < MaxContacts; i++ ) 
	    {
			contact[i].surface.mode = 0;
			contact[i].surface.mu = Friction;
	    }

	    if ( int numc = dCollide( o1, o2, MaxContacts, &contact[0].geom, sizeof(dContact) ) )
	    {
	        for ( int i = 0; i < numc; i++ ) 
	        {
	            dJointID c = dJointCreateContact( simulation->world, simulation->contacts, contact+i );
	            dJointAttach( c, b1, b2 );
	        }

			if ( b1 && b2 )
			{
				const CubeData * cube1 = (const CubeData*) dBodyGetData( b1 );
				const CubeData * cube2 = (const CubeData*) dBodyGetData( b2 );
			
				assert( cube1 );
				assert( cube2 );
			
				if ( cube1->parentCubeId == -1 && cube2->parentCubeId == -1 )
				{
					int cubeId1 = (int) ( uint64(cube1) - uint64(simulation->cubes) ) / sizeof( CubeData );
					int cubeId2 = (int) ( uint64(cube2) - uint64(simulation->cubes) ) / sizeof( CubeData );
			
					if ( ! ( cube1->ownerPlayerId != -1 && cube1->ownerPlayerId == cube2->parentCubeId ||
						     cube2->ownerPlayerId != -1 && cube2->ownerPlayerId == cube1->parentCubeId ) )
					{
						assert( cubeId1 >= 0 );
						assert( cubeId2 >= 0 );
						assert( cubeId1 < MaxCubes );
						assert( cubeId2 < MaxCubes );
	
						simulation->cubeContacts[cubeId1].touching[cubeId2] = true;
						simulation->cubeContacts[cubeId2].touching[cubeId1] = true;
					}
				}
			}
	    }
	}
};

// authority management

struct AuthorityData
{
	int authority;							// authority id. -1 = default authority, 0 is server player, 1 is client player
	float timeAtRest;						// time at rest. disable body if at rest for ~1 second or so
	float authorityTimeOut;					// we revert to default authority if timeout exceeds some value like 2 seconds
	
	AuthorityData()
	{
		authority = -1;
		timeAtRest = 0;
		authorityTimeOut = 0;
	}
};

struct AuthorityState
{
	int numCubes;
	AuthorityData cubeAuthority[MaxCubes];
};

class AuthorityManagement
{
public:
	
	AuthorityManagement()
	{
		this->localAuthorityId = 0;
		this->authorityByDefault = true;
	}
	
	void SetSimulationState( const SimulationState & simulationState )
	{
		this->simulationState = simulationState;
	}
	
	void Update( float deltaTime, int localAuthorityId, bool authorityByDefault )
	{
		if ( localAuthorityId == -1 )
			return;
		
		assert( localAuthorityId >= 0 );
		assert( localAuthorityId < MaxPlayers );

		this->localAuthorityId = localAuthorityId;
		this->authorityByDefault = authorityByDefault;

		// detect local authority changes

		authorityState.numCubes = simulationState.numCubes;

		for ( int i = 0; i < simulationState.numCubes; ++i )
		{
			for ( int j = 0; j < MaxPlayers; ++j )
			{
				if ( simulationState.playerInteractions[j].interacting[i] && authorityState.cubeAuthority[i].authority == -1 )
				{
					authorityState.cubeAuthority[i].authority = j;
					authorityState.cubeAuthority[i].authorityTimeOut = 0.0f;
					break;
				}
			}
		}
		
		// detect simulation bodies at rest

		for ( int i = 0; i < authorityState.numCubes; ++i )
		{
			const float linearSpeed = simulationState.cubeState[i].linearVelocity.length();
			const float angularSpeed = simulationState.cubeState[i].angularVelocity.length();

			const bool resting = linearSpeed < LinearRestThreshold && angularSpeed < AngularRestThreshold;

			if ( resting )
				authorityState.cubeAuthority[i].timeAtRest += deltaTime;
			else
				authorityState.cubeAuthority[i].timeAtRest = 0.0f;

//			simulationState.cubeState[i].enabled = authorityState.cubeAuthority[i].timeAtRest > RestTime;
		}

		// update authority timeout

		for ( int i = 0; i < authorityState.numCubes; ++i )
		{
			if ( !simulationState.cubeState[i].enabled )
				authorityState.cubeAuthority[i].authorityTimeOut += deltaTime;
			else
				authorityState.cubeAuthority[i].authorityTimeOut = 0.0f;

			if ( authorityState.cubeAuthority[i].authorityTimeOut > AuthorityTimeOut )
				authorityState.cubeAuthority[i].authority = -1;
		}
		
		// force authority for player controlled cubes
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			if ( simulationState.playerExists[i] )
			{
				int playerCubeId = simulationState.playerState[i].cubeId;
				
				assert( playerCubeId >= 0 );
				assert( playerCubeId < MaxCubes );
				
				authorityState.cubeAuthority[playerCubeId].authority = i;
				authorityState.cubeAuthority[playerCubeId].timeAtRest = 0.0f;
			}
		}
	}
	
	void PushPlayerInput( int playerId, const SimulationPlayerInput & playerInput )
	{
		assert( playerId >= 0 );
		assert( playerId < MaxPlayers );

		simulationState.playerInput[playerId] = playerInput;
	}
	
	void PushPlayerState( int playerId, const SimulationPlayerState & playerState )
	{
		assert( playerId >= 0 );
		assert( playerId < MaxPlayers );

		// note: only push remote player state
		if ( playerId != localAuthorityId )
			simulationState.playerState[playerId] = playerState;
	}
	
	void PushCubeState( int authorityId, int cubeId, const SimulationCubeState & cubeState )
	{
		assert( authorityId >= -1 );
		assert( authorityId < MaxPlayers );

		assert( cubeId >= 0 );
		assert( cubeId < MaxCubes );
		assert( cubeId < simulationState.numCubes );

		// if we have default authority, dont allow non-authority pushes
		if ( authorityByDefault && authorityId == -1 )
			return;
			
		// dont let anybody push state to our "absolute" authority cases (player controlled cubes)
		if ( simulationState.playerState[localAuthorityId].cubeId == cubeId )
			return;
		
		// if we have default authority, other authority is not allowed to override ours
		if ( authorityByDefault && authorityState.cubeAuthority[cubeId].authority == localAuthorityId && authorityId != localAuthorityId )
			return;
			
		// if there is an existing authority, you cannot override it with no authority
		if ( authorityId == -1 && authorityState.cubeAuthority[cubeId].authority != -1 )
			return;
			
		// change authority if applicable
		if ( authorityId != -1 && authorityId != authorityState.cubeAuthority[cubeId].authority )
			authorityState.cubeAuthority[cubeId].authority = authorityId;
		
		// set cube state
		simulationState.cubeState[cubeId] = cubeState;
	}
	
	void GetSimulationState( SimulationState & simulationState )
	{
		simulationState = this->simulationState;
	}
	
	const AuthorityState & GetAuthorityState()
	{
		return this->authorityState;
	}
	
private:
	
	int localAuthorityId;						// player id of local authority
	bool authorityByDefault;					// true if we have authority by default (server)
	SimulationState simulationState;			// the simulation state we are managing
	AuthorityState authorityState;				// authority state for each cube
};

// ------------------------------------------------------------------------------

// rendering

class Renderer
{
public:
	
	Renderer( int displayWidth, int displayHeight )
	{
		this->displayWidth = displayWidth;
		this->displayHeight = displayHeight;
		
		lightPosition = math::Vector( 25.0f, 50.0f, -25.0f );
		
		#ifdef CUBE_DISPLAY_LIST
		
			// setup cube display list
		
			cubeDisplayList = glGenLists( 1 );
		
			glNewList( cubeDisplayList, GL_COMPILE );
		
			glBegin( GL_QUADS );

				glNormal3f(  0,  0, +1 );
				glVertex3f( -1, -1, +1 );
				glVertex3f( -1, +1, +1 );
				glVertex3f( +1, +1, +1 );
				glVertex3f( +1, -1, +1 );

				glNormal3f(  0,  0,  -1 );
				glVertex3f( -1, -1, -1 );
				glVertex3f( +1, -1, -1 );
				glVertex3f( +1, +1, -1 );
				glVertex3f( -1, +1, -1 );

				glNormal3f(  0, +1,  0 );
				glVertex3f( -1, +1, -1 );
				glVertex3f( +1, +1, -1 );
				glVertex3f( +1, +1, +1 );
				glVertex3f( -1, +1, +1 );

				glNormal3f(  0, -1,  0 );
				glVertex3f( -1, -1, -1 );
				glVertex3f( -1, -1, +1 );
				glVertex3f( +1, -1, +1 );
				glVertex3f( +1, -1, -1 );

				glNormal3f( +1,  0,  0 );
				glVertex3f( +1, -1, -1 );
				glVertex3f( +1, -1, +1 );
				glVertex3f( +1, +1, +1 );
				glVertex3f( +1, +1, -1 );

				glNormal3f( -1,  0,  0 );
				glVertex3f( -1, -1, -1 );
				glVertex3f( -1, +1, -1 );
				glVertex3f( -1, +1, +1 );
				glVertex3f( -1, -1, +1 );

			glEnd();
        
			glEndList();
			
		#endif
		
		#ifdef FLOOR_AND_WALLS_DISPLAY_LIST

			floorAndWallsDisplayList = glGenLists( 1 );
		
			glNewList( floorAndWallsDisplayList, GL_COMPILE );
		
			const float r = 1.0f;
			const float g = 1.0f;
			const float b = 1.0f;
			const float a = 1.0f;

			glColor3f( r, g, b );

	        GLfloat color[] = { r, g, b, a };

			glMaterialfv( GL_FRONT, GL_AMBIENT, color );
			glMaterialfv( GL_FRONT, GL_DIFFUSE, color );
			glColor3f( r, g, b );

			const float s = Boundary;

	        glBegin( GL_QUADS );

				glNormal3f( 0, +1, 0 );
				glVertex3f( -s, 0, -15 );
				glVertex3f( +s, 0, -15 );
				glVertex3f( +s, 0, +10 );
				glVertex3f( -s, 0, +10 );

				glNormal3f( +1, 0, 0 );
				glVertex3f( -s, -s*2, -s*2 );
				glVertex3f( -s, -s*2, +s*2 );
				glVertex3f( -s, +s*2, +s*2 );
				glVertex3f( -s, +s*2, -s*2 );
		
				glNormal3f( -1, 0, 0 );
				glVertex3f( +s, -s*2, -s*2 );
				glVertex3f( +s, +s*2, -s*2 );
				glVertex3f( +s, +s*2, +s*2 );
				glVertex3f( +s, -s*2, +s*2 );

				glNormal3f( 0, 0, -1 );
				glVertex3f( -s*2, -s*2, +s );
				glVertex3f( +s*2, -s*2, +s );
				glVertex3f( +s*2, +s*2, +s );
				glVertex3f( -s*2, +s*2, +s );

			glEnd();
        
			glEndList();

		#endif
	}
	
	~Renderer()
	{
		#ifdef CUBE_DISPLAY_LIST
		glDeleteLists( cubeDisplayList, 1 );
		#endif
		#ifdef FLOOR_AND_WALS_DISPLAY_LIST
		glDeleteLists( floorAndWallsDisplayList, 1 );
		#endif
	}
	
	void ClearScreen()
	{
		glViewport( 0, 0, DisplayWidth, DisplayHeight );
		glDisable( GL_SCISSOR_TEST );
		glClearStencil( 0 );
		glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );		
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	}
	
	void Render( RenderState & renderState, float x1, float y1, float x2, float y2 )
	{
		// setup viewport & scissor
	
		glViewport( x1, y1, x2 - x1, y2 - y1 );
		glScissor( x1, y1, x2 - x1, y2 - y1 );
		glEnable( GL_SCISSOR_TEST );

		// setup view

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		float fov = 45.0f;
		gluPerspective( fov, ( x2 - x1 ) / ( y2 - y1 ), 0.1f, 50.0f );
	
		#ifdef CAMERA_FOLLOW
	
			if ( renderState.localPlayerId == -1 )
				return;
			
			assert( renderState.player[localPlayerId].exists );
			
			math::Vector playerPosition = renderState.player[renderState.localPlayerId].position;

			float x = Clamp( playerPosition.x, -Boundary * 0.95f, +Boundary * 0.95f );
			float z = Clamp( playerPosition.z, -Boundary * 0.8f, +Boundary * 0.95f );
			float h1 = 1.5f;
			float h2 = 2.0f;
			float d1 = 2.0f;
			float d2 = 5.0f;
	
			glMatrixMode( GL_MODELVIEW );
			glLoadIdentity();
			glScalef( -1.0f, 1.0f, 1.0f );
			gluLookAt( x, h2, z - d2, 
			           x, h1, z - d1,
					   0.0f, 1.0f, 0.0 );
	
		#else

			glMatrixMode( GL_MODELVIEW );
			glLoadIdentity();
			glScalef( -1.0f, 1.0f, 1.0f );
			gluLookAt( 0.0f, 4.0f, -16.0f, 
			           0.0f, 3.0f, 0.0f,
					   0.0f, 1.0f, 0.0 );
		
		#endif
				
		// setup lights

        glShadeModel( GL_SMOOTH );
	
        glEnable( GL_LIGHT0 );

        GLfloat lightAmbientColor[] = { 0.5, 0.5, 0.5, 1.0 };
        glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lightAmbientColor );

        glLightf( GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f );

		GLfloat position[4];
		position[0] = lightPosition.x;
		position[1] = lightPosition.y;
		position[2] = lightPosition.z;
		position[3] = 1.0f;
        glLightfv( GL_LIGHT0, GL_POSITION, position );

		// enable depth buffering and backface culling
	
		glEnable( GL_DEPTH_TEST );
	    glDepthFunc( GL_LEQUAL );
    
	    glEnable( GL_CULL_FACE );
	    glCullFace( GL_BACK );
	    glFrontFace( GL_CCW );

		// update cube colors
		
		for ( int i = 0; i < renderState.numCubes; ++i )
		{
			const float dr = renderState.cubes[i].r - cubeData[i].r;
			const float dg = renderState.cubes[i].g - cubeData[i].g;
			const float db = renderState.cubes[i].b - cubeData[i].b;
			
			const float epsilon = 0.001f;
			
			if ( math::abs(dr) > epsilon )
				cubeData[i].r += dr * ColorChangeTightness;
			else
				cubeData[i].r = renderState.cubes[i].r;

			if ( math::abs(dg) > epsilon )
				cubeData[i].g += dg * ColorChangeTightness;
			else
				cubeData[i].g = renderState.cubes[i].g;

			if ( math::abs(db) > epsilon )
				cubeData[i].b += db * ColorChangeTightness;
			else
				cubeData[i].b = renderState.cubes[i].b;
		}

		// render scene

		glEnable( GL_LIGHTING );
		glEnable( GL_NORMALIZE );

		glDepthFunc( GL_LESS );

		RenderFloorAndWalls();
		
		RenderCubes( renderState.numCubes, renderState.cubes );

		glDisable( GL_LIGHTING );
		glDisable( GL_NORMALIZE );
	}
	
	void RenderShadows( RenderState & renderState, float x1, float y1, float x2, float y2 )
	{
	#ifdef RENDER_SHADOWS
		
		// render shadows

		RenderCubeShadowVolumes( renderState.numCubes, renderState.cubes, lightPosition );
	
		RenderShadow();
		
	#endif	
	}
	
	void RenderFloorAndWalls()
	{
		#ifdef FLOOR_AND_WALLS_DISPLAY_LIST
		
			glCallList( floorAndWallsDisplayList );

		#else
		
			const float r = 1.0f;
			const float g = 1.0f;
			const float b = 1.0f;
			const float a = 1.0f;

			glColor3f( r, g, b );

	        GLfloat color[] = { r, g, b, a };

			glMaterialfv( GL_FRONT, GL_AMBIENT, color );
			glMaterialfv( GL_FRONT, GL_DIFFUSE, color );
			glColor3f( r, g, b );

			const float s = Boundary;

	        glBegin( GL_QUADS );

				glNormal3f( 0, +1, 0 );
				glVertex3f( -s, 0, -15 );
				glVertex3f( +s, 0, -15 );
				glVertex3f( +s, 0, +10 );
				glVertex3f( -s, 0, +10 );

				glNormal3f( +1, 0, 0 );
				glVertex3f( -s, -s*2, -s*2 );
				glVertex3f( -s, -s*2, +s*2 );
				glVertex3f( -s, +s*2, +s*2 );
				glVertex3f( -s, +s*2, -s*2 );
		
				glNormal3f( -1, 0, 0 );
				glVertex3f( +s, -s*2, -s*2 );
				glVertex3f( +s, +s*2, -s*2 );
				glVertex3f( +s, +s*2, +s*2 );
				glVertex3f( +s, -s*2, +s*2 );

				glNormal3f( 0, 0, -1 );
				glVertex3f( -s*2, -s*2, +s );
				glVertex3f( +s*2, -s*2, +s );
				glVertex3f( +s*2, +s*2, +s );
				glVertex3f( -s*2, +s*2, +s );

			glEnd();
			
		#endif
	}
	
	void RenderCubes( int numCubes, RenderState::Cube cubes[] )
	{
		// render cubes

		for ( int i = 0; i < numCubes; ++i )
		{
			const RenderState::Cube & cube = cubes[i];

			PushTransform( cube.position, cube.orientation, cube.scale );

			const float r = cubeData[i].r;
			const float g = cubeData[i].g;
			const float b = cubeData[i].b;

	        GLfloat color[] = { r, g, b, 1.0f };

			glMaterialfv( GL_FRONT, GL_AMBIENT, color );
			glMaterialfv( GL_FRONT, GL_DIFFUSE, color );

			#ifdef CUBE_DISPLAY_LIST

				glCallList( cubeDisplayList );

			#else

		        glBegin( GL_QUADS );

					glNormal3f(  0,  0, +1 );
					glVertex3f( -1, -1, +1 );
					glVertex3f( -1, +1, +1 );
					glVertex3f( +1, +1, +1 );
					glVertex3f( +1, -1, +1 );

					glNormal3f(  0,  0,  -1 );
					glVertex3f( -1, -1, -1 );
					glVertex3f( +1, -1, -1 );
					glVertex3f( +1, +1, -1 );
					glVertex3f( -1, +1, -1 );

					glNormal3f(  0, +1,  0 );
					glVertex3f( -1, +1, -1 );
					glVertex3f( +1, +1, -1 );
					glVertex3f( +1, +1, +1 );
					glVertex3f( -1, +1, +1 );

					glNormal3f(  0, -1,  0 );
					glVertex3f( -1, -1, -1 );
					glVertex3f( -1, -1, +1 );
					glVertex3f( +1, -1, +1 );
					glVertex3f( +1, -1, -1 );

					glNormal3f( +1,  0,  0 );
					glVertex3f( +1, -1, -1 );
					glVertex3f( +1, -1, +1 );
					glVertex3f( +1, +1, +1 );
					glVertex3f( +1, +1, -1 );

					glNormal3f( -1,  0,  0 );
					glVertex3f( -1, -1, -1 );
					glVertex3f( -1, +1, -1 );
					glVertex3f( -1, +1, +1 );
					glVertex3f( -1, -1, +1 );

				glEnd();

			#endif

			PopTransform();
		}
	}
	
	void RenderCubeShadowVolumes( int numCubes, RenderState::Cube cubes[], const math::Vector & lightPosition )
	{
		#ifdef DEBUG_SHADOW_VOLUMES
		
			// visualize shadow volumes in pink

			glDepthMask( GL_FALSE );
			
			glColor3f( 1.0f, 0.75f, 0.8f );

			for ( int i = 0; i < numCubes; ++i )
			{
				const RenderState::Cube & cube = cubes[i];

				PushTransform( cube.position, cube.orientation, cube.scale );

				math::Vector localLightPosition = InverseTransform( lightPosition, cube.position, cube.orientation, cube.scale );

	            RenderShadowVolume( cube, localLightPosition );

				PopTransform();
			}

			glDepthMask( GL_TRUE);
		
		#else
			
			// render shadow volumes to stencil buffer
		
			glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
			glDepthMask( GL_FALSE );
            
			glEnable( GL_STENCIL_TEST );

            glCullFace( GL_BACK );
            glStencilFunc( GL_ALWAYS, 0x0, 0xff );
            glStencilOp( GL_KEEP, GL_KEEP, GL_INCR );

			for ( int i = 0; i < numCubes; ++i )
			{
				const RenderState::Cube & cube = cubes[i];

				PushTransform( cube.position, cube.orientation, cube.scale );

				math::Vector localLightPosition = InverseTransform( lightPosition, cube.position, cube.orientation, cube.scale );

	            RenderShadowVolume( cube, localLightPosition );
	
				PopTransform();
			}

            glCullFace( GL_FRONT );
            glStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

			for ( int i = 0; i < numCubes; ++i )
			{
				const RenderState::Cube & cube = cubes[i];

				PushTransform( cube.position, cube.orientation, cube.scale );

				math::Vector localLightPosition = InverseTransform( lightPosition, cube.position, cube.orientation, cube.scale );

	            RenderShadowVolume( cube, localLightPosition );
	
				PopTransform();
			}
		
			glCullFace( GL_BACK );
			glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
			glDepthMask( GL_TRUE);

			glDisable( GL_STENCIL_TEST );
			
		#endif
	}
	
	void RenderShadowVolume( const RenderState::Cube & cube, const math::Vector & light )
	{
        glBegin( GL_QUADS );

        RenderSilhouette( light, math::Vector(-1,+1,-1), math::Vector(+1,+1,-1) );
        RenderSilhouette( light, math::Vector(+1,+1,-1), math::Vector(+1,+1,+1) );
        RenderSilhouette( light, math::Vector(+1,+1,+1), math::Vector(-1,+1,+1) );
        RenderSilhouette( light, math::Vector(-1,+1,+1), math::Vector(-1,+1,-1) );

        RenderSilhouette( light, math::Vector(-1,-1,-1), math::Vector(+1,-1,-1) );
		RenderSilhouette( light, math::Vector(+1,-1,-1), math::Vector(+1,-1,+1) );
        RenderSilhouette( light, math::Vector(+1,-1,+1), math::Vector(-1,-1,+1) );
		RenderSilhouette( light, math::Vector(-1,-1,+1), math::Vector(-1,-1,-1) );

        RenderSilhouette( light, math::Vector(-1,+1,-1), math::Vector(-1,-1,-1) );
        RenderSilhouette( light, math::Vector(+1,+1,-1), math::Vector(+1,-1,-1) );
        RenderSilhouette( light, math::Vector(+1,+1,+1), math::Vector(+1,-1,+1) );
        RenderSilhouette( light, math::Vector(-1,+1,+1), math::Vector(-1,-1,+1) );

        glEnd();
	}

    void RenderSilhouette( const math::Vector & light, math::Vector a, math::Vector b )
    {
        // determine edge normals

        math::Vector midpoint = ( a + b ) * 0.5f;
        
        math::Vector leftNormal;

		const float epsilon = 0.00001f;

        if ( midpoint.x > epsilon || midpoint.x < -epsilon )
            leftNormal = math::Vector( midpoint.x, 0, 0 );
        else
            leftNormal = math::Vector( 0, midpoint.y, 0 );

        math::Vector rightNormal = midpoint - leftNormal;

        // check if silhouette edge

        const math::Vector differenceA = a - light;

		const float leftDot = leftNormal.dot( differenceA );
		const float rightDot = rightNormal.dot( differenceA );

        if ( ( leftDot < 0 && rightDot > 0 ) || ( leftDot > 0 && rightDot < 0 ) )
        {
            // extrude quad

            const math::Vector differenceB = b - light;

            math::Vector _a = a + differenceA * ShadowDistance;
            math::Vector _b = b + differenceB * ShadowDistance;

            // ensure correct winding order for silhouette edge

            const math::Vector cross = ( b - a ).cross( differenceA );
            
            if ( cross.dot( a ) < 0 )
            {
                math::Vector t = a;
                a = b;
                b = t;

                t = _a;
                _a = _b;
                _b = t;
            }

            // render extruded quad

            glVertex3f( b.x, b.y, b.z );
            glVertex3f( a.x, a.y, a.z ); 
            glVertex3f( _a.x, _a.y, _a.z ); 
            glVertex3f( _b.x, _b.y, _b.z );
        }
    }
	
	void RenderShadow()
	{
		#ifndef DEBUG_SHADOW_VOLUMES
		
		EnterScreenSpace();
	
		glDisable( GL_DEPTH_TEST );

		glEnable( GL_STENCIL_TEST );

		glStencilFunc( GL_NOTEQUAL, 0x0, 0xff );
		glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

		glColor4f( 0.0f, 0.0f, 0.0f, 0.15f );

		glBegin( GL_QUADS );

			glVertex2f( 0, 0 );
			glVertex2f( 1, 0 );
			glVertex2f( 1, 1 );
			glVertex2f( 0, 1 );

		glEnd();

		glDisable( GL_BLEND );

		glDisable( GL_STENCIL_TEST );
		
		LeaveScreenSpace();
		
		#endif
	}
	
protected:
	
	math::Vector InverseTransform( const math::Vector & input, const math::Vector & position, const math::Quaternion & orientation, float scale )
	{
		assert( scale >= 0.0f );
		
		math::Vector output = input - position;
		output /= scale;
		output = RotateVectorByQuaternion( output, orientation.inverse() );
		
		return output;
	}
	
 	math::Vector RotateVectorByQuaternion( const math::Vector & input, const math::Quaternion & q )
	{
		math::Quaternion q_inv = q.inverse();
		math::Quaternion a( 0, input.x, input.y, input.z );
		math::Quaternion r = ( q * a ) * q_inv;
		return math::Vector( r.x, r.y, r.z );
	}
	
	void EnterScreenSpace()
	{
	    glMatrixMode( GL_PROJECTION );
	    glPushMatrix();
	    glLoadIdentity();
	    glOrtho( 0, 1, 0, 1, 1, -1 );

	    glMatrixMode( GL_MODELVIEW );
	    glPushMatrix();
	    glLoadIdentity();
	}
	
	void LeaveScreenSpace()
	{
	    glMatrixMode( GL_PROJECTION );
	    glPopMatrix();

	    glMatrixMode( GL_MODELVIEW );
	    glPopMatrix();
	}
		
	void PushTransform( const math::Vector & position, const math::Quaternion & orientation, float scale )
	{
		glPushMatrix();

		glTranslatef( position.x, position.y, position.z ); 

		const float pi = 3.1415926f;

		float angle;
		math::Vector axis;
		GetAxisAngle( orientation, axis, angle );
		glRotatef( angle / pi * 180, axis.x, axis.y, axis.z );

		glScalef( scale, scale, scale );
	}
	
	void PopTransform()
	{
		glPopMatrix();
	}
	
	void GetAxisAngle( const math::Quaternion & quaternion, math::Vector & axis, float & angle ) const
	{
		const float w = quaternion.w;
		const float x = quaternion.x;
		const float y = quaternion.y;
		const float z = quaternion.z;
		
		const float lengthSquared = x*x + y*y + z*z;
		
		if ( lengthSquared > 0.00001f )
		{
			angle = 2.0f * (float) acos( w );
			const float inverseLength = 1.0f / (float) pow( lengthSquared, 0.5f );
			axis.x = x * inverseLength;
			axis.y = y * inverseLength;
			axis.z = z * inverseLength;
		}
		else
		{
			angle = 0.0f;
			axis.x = 1.0f;
			axis.y = 0.0f;
			axis.z = 0.0f;
		}
	}
	
private:
	
	int displayWidth;
	int displayHeight;
	
	#ifdef CUBE_DISPLAY_LIST
	int cubeDisplayList;
	#endif

	#ifdef FLOOR_AND_WALLS_DISPLAY_LIST
	int floorAndWallsDisplayList;
	#endif
	
	math::Vector lightPosition;
	
	struct CubeData
	{
		CubeData()
		{
			r = 1.0f;
			g = 1.0f;
			b = 1.0f;
		}
		
		float r;
		float g;
		float b;
	};
	
	CubeData cubeData[MaxCubes];
};

// ------------------------------------------------------------------------------

// worker thread 

#ifdef MULTITHREADED
#include <pthread.h>
#endif

class WorkerThread
{
public:
	
	WorkerThread()
	{
		#ifdef MULTITHREADED
		thread = 0;
		#endif
	}
	
	virtual ~WorkerThread()
	{
		#ifdef MULTITHREADED
		thread = 0;
		#endif
	}
	
	bool Start()
	{
		#ifdef MULTITHREADED
	
			if ( pthread_create( &thread, NULL, StaticRun, (void*)this ) != 0 )
			{
				printf( "error: pthread_create failed\n" );
				return false;
			}
		
		#else
		
			Run();
			
		#endif
		
		return true;
	}
	
	bool Join()
	{
		#ifdef MULTITHREADED
		if ( pthread_join( thread, NULL ) != 0 )
		{
			printf( "error: pthread_join failed\n" );
			return false;
		}
		#endif
		return true;
	}
	
protected:
	
	static void* StaticRun( void * data )
	{
		WorkerThread * self = (WorkerThread*) data;
		self->Run();
		return NULL;
	}
	
	virtual void Run() = 0;			// note: override this to implement your thread task
	
private:

	#ifdef MULTITHREADED	
	pthread_t thread;
	#endif
};

// simulation worker thread

class SimulationWorkerThread : public WorkerThread
{
public:
	
	SimulationWorkerThread( PhysicsSimulation * simulation, float deltaTime )
	{
		assert( simulation );
		this->simulation = simulation;
		this->deltaTime = deltaTime;
	}
	
protected:
	
	void Run()
	{
		assert( simulation );
		simulation->Update( deltaTime );
	}
	
private:

	PhysicsSimulation * simulation;
	float deltaTime;
};

// ------------------------------------------------------------------------------

int main( int argc, char * argv[] )
{
	if ( !OpenDisplay( "Authority Management", DisplayWidth, DisplayHeight ) )
	{
		printf( "failed to open display\n" );
		return 1;
	}
	
	Renderer renderer( DisplayWidth, DisplayHeight );

	AuthorityManagement authorityManagement;

	PhysicsSimulation clientSimulation;
	PhysicsSimulation serverSimulation;
	
	clientSimulation.OnPlayerJoin( 0 );
	clientSimulation.OnPlayerJoin( 1 );

	serverSimulation.OnPlayerJoin( 0 );
	serverSimulation.OnPlayerJoin( 1 );
	
	Timer timer;
	
	while ( true )
	{
		#ifndef VSYNC
		float deltaTime = timer.delta();
		if ( deltaTime < 0.001f )
			deltaTime = 0.001f;
		if ( deltaTime > 0.01f )
			deltaTime = 0.01f;
		#else
		float deltaTime = 1.0f / 60.0f;
		#endif
		
		RenderState renderState;
		serverSimulation.GetRenderState( renderState );

		const AuthorityState & authorityState = authorityManagement.GetAuthorityState();
	
		for ( int i = 0; i < authorityState.numCubes; ++i )
		{
			if ( authorityState.cubeAuthority[i].authority == 0 )
			{
				renderState.cubes[i].r = 1.0f;
				renderState.cubes[i].g = 0.0f;
				renderState.cubes[i].b = 0.0f;
			}
			else if ( authorityState.cubeAuthority[i].authority == 1 )
			{
				renderState.cubes[i].r = 0.0f;
				renderState.cubes[i].g = 0.0f;
				renderState.cubes[i].b = 1.0f;
			}
			else
			{
				renderState.cubes[i].r = 1.0f;
				renderState.cubes[i].g = 1.0f;
				renderState.cubes[i].b = 1.0f;
			}
		}

		SimulationWorkerThread clientThread( &clientSimulation, deltaTime );
		SimulationWorkerThread serverThread( &serverSimulation, deltaTime );
		clientThread.Start();
		serverThread.Start();

		renderer.ClearScreen();
		renderer.Render( renderState, 0, 0, DisplayWidth, DisplayHeight );
		renderer.RenderShadows( renderState, 0, 0, DisplayWidth, DisplayHeight );

		#ifdef VSYNC
		UpdateDisplay( 1 );
		#else
		UpdateDisplay( 0 );
		#endif

		Input input = Input::Sample();
		
		if ( input.escape )
			break;

		SimulationPlayerInput serverInput;
		serverInput.left = input.left;
		serverInput.right = input.right;
		serverInput.forward = input.up;
		serverInput.back = input.down;

		SimulationPlayerInput clientInput;
		clientInput.left = input.a;
		clientInput.right = input.d;
		clientInput.forward = input.w;
		clientInput.back = input.s;

		serverThread.Join();
		clientThread.Join();

		serverSimulation.SetPlayerInput( 0, serverInput );
		serverSimulation.SetPlayerInput( 1, clientInput );

		SimulationState simulationState;
		serverSimulation.GetSimulationState( simulationState );
		authorityManagement.SetSimulationState( simulationState );
		authorityManagement.Update( deltaTime, 0, true );
		authorityManagement.GetSimulationState( simulationState );
		serverSimulation.SetSimulationState( simulationState );
	}
	
	CloseDisplay();

	return 0;
}
