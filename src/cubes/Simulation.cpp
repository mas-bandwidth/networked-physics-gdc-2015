/*
	Networked Physics Demo
	Copyright © 2008-2015 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#include "Simulation.h"
#define dSINGLE
#include <ode/ode.h>

namespace cubes
{	
	// simulation internal implementation
	
	const int MaxContacts = 16;

	struct SimulationImpl
	{
		SimulationImpl()
		{
			world = 0;
			space = 0;
			contacts = 0;
		}
		
		~SimulationImpl()
		{
			if ( contacts )
				dJointGroupDestroy( contacts );
			if ( world )
				dWorldDestroy( world );
			if ( space )
				dSpaceDestroy( space );
				
			contacts = 0;
			world = 0;
			space = 0;
		}
		
		dWorldID world;
		dSpaceID space;
		dJointGroupID contacts;

		struct ObjectData
		{
			dBodyID body;
			dGeomID geom;
			float scale;
			float timeAtRest;

			ObjectData()
			{
				body = 0;
				geom = 0;
				scale = 1.0f;
				timeAtRest = 0.0f;
			}

			bool exists() const
			{
				return body != 0;
			}
		};

		SimulationConfig config;
		std::vector<dGeomID> planes;
		std::vector<ObjectData> objects;
		std::vector< std::vector<uint16_t> > interactions;

	    dContact contact[MaxContacts];			

		void UpdateInteractionPairs( dBodyID b1, dBodyID b2 )
		{
			if ( !b1 || !b2 )
				return;

			uint64_t objectId1 = reinterpret_cast<uint64_t>( dBodyGetData( b1 ) );
			uint64_t objectId2 = reinterpret_cast<uint64_t>( dBodyGetData( b2 ) );

			interactions[objectId1].push_back( objectId2 );
			interactions[objectId2].push_back( objectId1 );
		}

		static void NearCallback( void * data, dGeomID o1, dGeomID o2 )
		{
			SimulationImpl * simulation = (SimulationImpl*) data;

			assert( simulation );

		    dBodyID b1 = dGeomGetBody( o1 );
		    dBodyID b2 = dGeomGetBody( o2 );

			if ( int numc = dCollide( o1, o2, MaxContacts, &simulation->contact[0].geom, sizeof(dContact) ) )
			{
		        for ( int i = 0; i < numc; i++ )
		        {
		            dJointID c = dJointCreateContact( simulation->world, simulation->contacts, simulation->contact+i );
		            dJointAttach( c, b1, b2 );
		        }
		
				simulation->UpdateInteractionPairs( b1, b2 );
			}
		}
	};

	// ------------------------------------------
	
	int * Simulation::GetInitCount()
	{
		static int initCount = 0;
		return &initCount;
	}

	Simulation::Simulation()
	{
		int * initCount = GetInitCount();
		if ( *initCount == 0 )
			dInitODE();
		(*initCount)++;
		impl = new SimulationImpl();
	}
	
	Simulation::~Simulation()
	{
		delete impl;
		impl = NULL;
		int * initCount = GetInitCount();
		(*initCount)--;
		if ( *initCount == 0 )
			dCloseODE();
	}

	void Simulation::Initialize( const SimulationConfig & config )
	{
		impl->config = config;

		// create simulation

		impl->world = dWorldCreate();
	    impl->contacts = dJointGroupCreate( 0 );
	    dVector3 center = { 0,0,0 };
	    dVector3 extents = { 100,100,100 };
	    impl->space = dQuadTreeSpaceCreate( 0, center, extents, 10 );

		// configure world

		dWorldSetERP( impl->world, config.ERP );
		dWorldSetCFM( impl->world, config.CFM );
		dWorldSetQuickStepNumIterations( impl->world, config.MaxIterations );
		dWorldSetGravity( impl->world, 0, 0, -config.Gravity );
		dWorldSetContactSurfaceLayer( impl->world, config.ContactSurfaceLayer );
		dWorldSetContactMaxCorrectingVel( impl->world, config.MaximumCorrectingVelocity );
		dWorldSetLinearDamping( impl->world, 0.01f );
		dWorldSetAngularDamping( impl->world, 0.01f );

		// setup contacts

	    for ( int i = 0; i < MaxContacts; i++ ) 
	    {
			impl->contact[i].surface.mode = dContactBounce;
			impl->contact[i].surface.mu = config.Friction;
			impl->contact[i].surface.bounce = config.Elasticity;
			impl->contact[i].surface.bounce_vel = 0.001f;
	    }
	
		impl->objects.resize( 1024 );
	}

	void Simulation::Update( float deltaTime, bool paused )
	{		
		impl->interactions.clear();
		
		impl->interactions.resize( impl->objects.size() );

		if ( paused )
			return;

		// IMPORTANT: do this *first* before updating simulation then at rest calculations
		// will work properly with rough quantization (quantized state is fed in prior to update)
		for ( int i = 0; i < (int) impl->objects.size(); ++i )
		{
			if ( impl->objects[i].exists() )
			{
				const dReal * linearVelocity = dBodyGetLinearVel( impl->objects[i].body );
				const dReal * angularVelocity = dBodyGetAngularVel( impl->objects[i].body );

				const float linearVelocityLengthSquared = linearVelocity[0]*linearVelocity[0] + linearVelocity[1]*linearVelocity[1] + linearVelocity[2]*linearVelocity[2];
				const float angularVelocityLengthSquared = angularVelocity[0]*angularVelocity[0] + angularVelocity[1]*angularVelocity[1] + angularVelocity[2]*angularVelocity[2];

				if ( linearVelocityLengthSquared > MaxLinearSpeed * MaxLinearSpeed )
				{
					const float linearSpeed = sqrt( linearVelocityLengthSquared );

					const float scale = MaxLinearSpeed / linearSpeed;

					dReal clampedLinearVelocity[3];

					clampedLinearVelocity[0] = linearVelocity[0] * scale;
					clampedLinearVelocity[1] = linearVelocity[1] * scale;
					clampedLinearVelocity[2] = linearVelocity[2] * scale;

					dBodySetLinearVel( impl->objects[i].body, clampedLinearVelocity[0], clampedLinearVelocity[1], clampedLinearVelocity[2] );

					linearVelocity = &clampedLinearVelocity[0];
				}

				if ( angularVelocityLengthSquared > MaxAngularSpeed * MaxAngularSpeed )
				{
					const float angularSpeed = sqrt( angularVelocityLengthSquared );

					const float scale = MaxAngularSpeed / angularSpeed;

					dReal clampedAngularVelocity[3];

					clampedAngularVelocity[0] = angularVelocity[0] * scale;
					clampedAngularVelocity[1] = angularVelocity[1] * scale;
					clampedAngularVelocity[2] = angularVelocity[2] * scale;

					dBodySetAngularVel( impl->objects[i].body, clampedAngularVelocity[0], clampedAngularVelocity[1], clampedAngularVelocity[2] );

					angularVelocity = &clampedAngularVelocity[0];
				}

				if ( linearVelocityLengthSquared < impl->config.LinearRestThresholdSquared && angularVelocityLengthSquared < impl->config.AngularRestThresholdSquared )
					impl->objects[i].timeAtRest += deltaTime;
				else
					impl->objects[i].timeAtRest = 0.0f;

				if ( impl->objects[i].timeAtRest >= impl->config.RestTime )
					dBodyDisable( impl->objects[i].body );
				else
					dBodyEnable( impl->objects[i].body );
			}
		}

		dJointGroupEmpty( impl->contacts );

		dSpaceCollide( impl->space, impl, SimulationImpl::NearCallback );

		if ( impl->config.QuickStep )
			dWorldQuickStep( impl->world, deltaTime );
		else
			dWorldStep( impl->world, deltaTime );
	}
	
	int Simulation::AddObject( const SimulationObjectState & initialObjectState )
	{
		// find free object slot

		uint64_t id = -1;
		for ( int i = 0; i < (int) impl->objects.size(); ++i )
		{
			if ( !impl->objects[i].exists() )
			{
				id = i;
				break;
			}
		}
		if ( id == -1 )
		{
			id = impl->objects.size();
			impl->objects.resize( id + 1 );
		}

		// setup object body

		impl->objects[id].body = dBodyCreate( impl->world );

		assert( impl->objects[id].body );

		dMass mass;
		const float density = 1.0f;
		dMassSetBox( &mass, density, initialObjectState.scale, initialObjectState.scale, initialObjectState.scale );
		dBodySetMass( impl->objects[id].body, &mass );
		dBodySetData( impl->objects[id].body, (void*) id );

		// setup geom and attach to body

		impl->objects[id].scale = initialObjectState.scale;
		impl->objects[id].geom = dCreateBox( impl->space, initialObjectState.scale, initialObjectState.scale, initialObjectState.scale );

		dGeomSetBody( impl->objects[id].geom, impl->objects[id].body );	

		// set object state

		SetObjectState( id, initialObjectState );

		// success!

		return id;
	}

	bool Simulation::ObjectExists( int id )
	{
		assert( id >= 0 && id < (int) impl->objects.size() );
		return impl->objects[id].exists();
	}

	float Simulation::GetObjectMass( int id )
	{
		assert( id >= 0 && id < (int) impl->objects.size() );
		assert( impl->objects[id].exists() );
		dMass mass;
		dBodyGetMass( impl->objects[id].body, &mass );
		return mass.mass;
	}

	void Simulation::RemoveObject( int id )
	{
		assert( id >= 0 && id < (int) impl->objects.size() );
		assert( impl->objects[id].exists() );

		dBodyDestroy( impl->objects[id].body );
		dGeomDestroy( impl->objects[id].geom );
		impl->objects[id].body = 0;
		impl->objects[id].geom = 0;
	}

	void Simulation::GetObjectState( int id, SimulationObjectState & objectState )
	{
		assert( id >= 0 );
		assert( id < (int) impl->objects.size() );

		assert( impl->objects[id].exists() );

		const dReal * position = dBodyGetPosition( impl->objects[id].body );
		const dReal * orientation = dBodyGetQuaternion( impl->objects[id].body );
		const dReal * linearVelocity = dBodyGetLinearVel( impl->objects[id].body );
		const dReal * angularVelocity = dBodyGetAngularVel( impl->objects[id].body );

		objectState.position = math::Vector( position[0], position[1], position[2] );
		objectState.orientation = math::Quaternion( orientation[0], orientation[1], orientation[2], orientation[3] );
		objectState.linearVelocity = math::Vector( linearVelocity[0], linearVelocity[1], linearVelocity[2] );
		objectState.angularVelocity = math::Vector( angularVelocity[0], angularVelocity[1], angularVelocity[2] );

		objectState.enabled = impl->objects[id].timeAtRest < impl->config.RestTime;
	}

	void Simulation::SetObjectState( int id, const SimulationObjectState & objectState, bool ignoreEnabledFlag )
	{
		assert( id >= 0 );
		assert( id < (int) impl->objects.size() );

		assert( impl->objects[id].exists() );

		dQuaternion quaternion;
		quaternion[0] = objectState.orientation.w;
		quaternion[1] = objectState.orientation.x;
		quaternion[2] = objectState.orientation.y;
		quaternion[3] = objectState.orientation.z;

		dBodySetPosition( impl->objects[id].body, objectState.position.x, objectState.position.y, objectState.position.z );
		dBodySetQuaternion( impl->objects[id].body, quaternion );
		dBodySetLinearVel( impl->objects[id].body, objectState.linearVelocity.x, objectState.linearVelocity.y, objectState.linearVelocity.z );
		dBodySetAngularVel( impl->objects[id].body, objectState.angularVelocity.x, objectState.angularVelocity.y, objectState.angularVelocity.z );

		if ( !ignoreEnabledFlag )
		{
			if ( objectState.enabled )
			{
				impl->objects[id].timeAtRest = 0.0f;
				dBodyEnable( impl->objects[id].body );
			}
			else
			{
				impl->objects[id].timeAtRest = impl->config.RestTime;
				dBodyDisable( impl->objects[id].body );
			}
		}
	}

	const std::vector<uint16_t> & Simulation::GetObjectInteractions( int id ) const
	{
		assert( id >= 0 );
		assert( id < (int) impl->interactions.size() );
		return impl->interactions[id];
	}

	void Simulation::ApplyForce( int id, const math::Vector & force )
	{
		assert( id >= 0 );
		assert( id < (int) impl->objects.size() );
		assert( impl->objects[id].exists() );
		if ( force.length() > 0.001f )
		{
			impl->objects[id].timeAtRest = 0.0f;
			dBodyEnable( impl->objects[id].body );
			dBodyAddForce( impl->objects[id].body, force.x, force.y, force.z );
		}
	}

	void Simulation::ApplyTorque( int id, const math::Vector & torque )
	{
		assert( id >= 0 );
		assert( id < (int) impl->objects.size() );
		assert( impl->objects[id].exists() );
		if ( torque.length() > 0.001f )
		{
			impl->objects[id].timeAtRest = 0.0f;
			dBodyEnable( impl->objects[id].body );
			dBodyAddTorque( impl->objects[id].body, torque.x, torque.y, torque.z );
		}
	}

	void Simulation::AddPlane( const math::Vector & normal, float d )
	{
		impl->planes.push_back( dCreatePlane( impl->space, normal.x, normal.y, normal.z, d ) );
	}

	void Simulation::Reset()
	{
		for ( int i = 0; i < (int) impl->objects.size(); ++i )
		{
			if ( impl->objects[i].exists() )
				RemoveObject( i );
		}

		for ( int i = 0; i < (int) impl->planes.size(); ++i )
			dGeomDestroy( impl->planes[i] );

		impl->planes.clear();
	}
}
