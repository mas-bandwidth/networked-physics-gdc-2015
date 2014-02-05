// physics simulation (ODE)

#ifndef SIMULATION_H
#define SIMULATION_H

#include "ode/ode.h"
#include <vector>

#define USE_QUICK_STEP

// new simulation code (with dynamic cube id allocation)

class NewSimulation
{	
public:

	NewSimulation()
	{
		printf( "simulation created\n" );

		world = 0;
		space = 0;
		contacts = 0;
	}
	
	void Initialize( const SimulationConfiguration & config )
	{
		this->config = config;
		
		// create simulation

		world = dWorldCreate();
		space = dHashSpaceCreate( 0 );
	    contacts = dJointGroupCreate( 0 );
		
		// configure world
		
		dWorldSetERP( world, config.ERP );
		dWorldSetCFM( world, config.CFM );
		dWorldSetQuickStepNumIterations( world, config.MaxIterations );
		dWorldSetGravity( world, 0, 0, -config.Gravity );
		dWorldSetContactSurfaceLayer( world, config.ContactSurfaceLayer );
		dWorldSetContactMaxCorrectingVel( world, config.MaximumCorrectingVelocity );
		dWorldSetLinearDamping( world, 0.01f );
		dWorldSetAngularDamping( world, 0.01f );		// todo: parameterize this!

		// allocate cubes
		
		cubes.resize( config.MaxCubes );
	}
	
	~NewSimulation()
	{
		if ( contacts )
			dJointGroupDestroy( contacts );
		if ( space )
			dSpaceDestroy( space );
		if ( world )
			dWorldDestroy( world );
	}
	
	static void Shutdown()
	{
		printf( "simulation shutdown\n" );
		dCloseODE();
	}

	void Update( float deltaTime )
	{		
		dJointGroupEmpty( contacts );
		
		dSpaceCollide( space, this, NearCallback );

		if ( config.QuickStep )
			dWorldQuickStep( world, deltaTime );
		else
			dWorldStep( world, deltaTime );
	}
	
	int AddCube( const SimulationInitialCubeState & initialCubeState )
	{
		// find free cube slot

		int id = -1;
		
		for ( int i = 0; i < (int) cubes.size(); ++i )
		{
			if ( !cubes[i].exists() )
			{
				id = i;
				break;
			}
		}
		
		if ( id == -1 )
			return -1;
		
		// setup cube body
		
		cubes[id].body = dBodyCreate( world );
		
		assert( cubes[id].body );

		dMass mass;
		dMassSetBox( &mass, initialCubeState.density, initialCubeState.scale, initialCubeState.scale, initialCubeState.scale );
		dBodySetMass( cubes[id].body, &mass );
		
		dBodySetData( cubes[id].body, &cubes[id] );

		// setup cube geom and attach to body

		cubes[id].scale = initialCubeState.scale;
		cubes[id].geom = dCreateBox( space, initialCubeState.scale, initialCubeState.scale, initialCubeState.scale );
		dGeomSetBody( cubes[id].geom, cubes[id].body );	

		// set cube state
		
		SetCubeState( id, initialCubeState );

		// success!

		return id;
	}
	
	void RemoveCube( int id )
	{
		assert( id >= 0 && id < (int) cubes.size() );
		assert( cubes[id].exists() );

		dBodyDestroy( cubes[id].body );
		dGeomDestroy( cubes[id].geom );
		cubes[id].body = 0;
		cubes[id].geom = 0;
	}

	void GetCubeState( int id, SimulationCubeState & cubeState )
	{
		assert( id >= 0 && id < (int) cubes.size() );
		
		assert( cubes[id].exists() );

		const dReal * position = dBodyGetPosition( cubes[id].body );
		const dReal * orientation = dBodyGetQuaternion( cubes[id].body );
		const dReal * linearVelocity = dBodyGetLinearVel( cubes[id].body );
		const dReal * angularVelocity = dBodyGetAngularVel( cubes[id].body );

		cubeState.enabled = dBodyIsEnabled( cubes[id].body ) != 0;
		cubeState.position = math::Vector( position[0], position[1], position[2] );
		cubeState.orientation = math::Quaternion( orientation[0], orientation[1], orientation[2], orientation[3] );
		cubeState.linearVelocity = math::Vector( linearVelocity[0], linearVelocity[1], linearVelocity[2] );
		cubeState.angularVelocity = math::Vector( angularVelocity[0], angularVelocity[1], angularVelocity[2] );
	}
	
	void SetCubeState( int id, const SimulationCubeState & cubeState )
	{
		assert( id >= 0 && id < (int) cubes.size() );

		assert( cubes[id].exists() );

		dQuaternion quaternion;
		quaternion[0] = cubeState.orientation.w;
		quaternion[1] = cubeState.orientation.x;
		quaternion[2] = cubeState.orientation.y;
		quaternion[3] = cubeState.orientation.z;

		dBodySetPosition( cubes[id].body, cubeState.position.x, cubeState.position.y, cubeState.position.z );
		dBodySetQuaternion( cubes[id].body, quaternion );
		dBodySetLinearVel( cubes[id].body, cubeState.linearVelocity.x, cubeState.linearVelocity.y, cubeState.linearVelocity.z );
		dBodySetAngularVel( cubes[id].body, cubeState.angularVelocity.x, cubeState.angularVelocity.y, cubeState.angularVelocity.z );

		dBodySetForce( cubes[id].body, 0, 0, 0 );
		dBodySetTorque( cubes[id].body, 0, 0, 0 );

		if ( cubeState.enabled )
			dBodyEnable( cubes[id].body );
		else
			dBodyDisable( cubes[id].body );
	}

	void AddPlane( const math::Vector & normal, float d )
	{
		planes.push_back( dCreatePlane( space, normal.x, normal.y, normal.z, d ) );
	}
	
	void Reset()
	{
		for ( int i = 0; i < (int) cubes.size(); ++i )
		{
			if ( cubes[i].exists() )
				RemoveCube( i );
		}
		
		for ( int i = 0; i < (int) planes.size(); ++i )
			dGeomDestroy( planes[i] );
			
		planes.clear();
	}

private:
	
	dWorldID world;
	dSpaceID space;
	dJointGroupID contacts;
	
	struct CubeData
	{
		dBodyID body;
		dGeomID geom;
		float scale;
		
		CubeData()
		{
			body = 0;
			geom = 0;
			scale = 1.0f;
		}
		
		bool exists() const
		{
			return body != 0 && geom != 0;
		}
	};

	std::vector<CubeData> cubes;
	std::vector<dGeomID> planes;

	SimulationConfiguration config;

protected:
		
	static void NearCallback( void * data, dGeomID o1, dGeomID o2 )
	{
		NewSimulation * simulation = (NewSimulation*) data;

		if ( !simulation )
			return;
	
	    dBodyID b1 = dGeomGetBody( o1 );
	    dBodyID b2 = dGeomGetBody( o2 );

		const int MaxContacts = 4;

		// todo: better contact processing following hplus suggestions

	    dContact contact[MaxContacts];			
	    for ( int i = 0; i < MaxContacts; i++ ) 
	    {
			contact[i].surface.mode = dContactBounce;
			contact[i].surface.mu = simulation->config.Friction;
			contact[i].surface.bounce = simulation->config.Elasticity;
			contact[i].surface.bounce_vel = 0.01f;
	    }

	    if ( int numc = dCollide( o1, o2, MaxContacts, &contact[0].geom, sizeof(dContact) ) )
	    {
	        for ( int i = 0; i < numc; i++ ) 
	        {
	            dJointID c = dJointCreateContact( simulation->world, simulation->contacts, contact+i );
	            dJointAttach( c, b1, b2 );
	        }
	    }
	}
};

#endif
