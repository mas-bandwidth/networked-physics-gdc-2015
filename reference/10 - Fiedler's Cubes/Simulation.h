/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef SIMULATION_H
#define SIMULATION_H

#include "Config.h"

#define dSINGLE
#include <ode/ode.h>
#include <vector>

namespace engine
{	
	// simulation config

	struct SimulationConfig
	{
		float ERP;
		float CFM;
	 	int MaxIterations;
		float Gravity;
		float LinearDrag;
		float AngularDrag;
		float Friction;
		float Elasticity;
		float ContactSurfaceLayer;
		float MaximumCorrectingVelocity;
		bool QuickStep;
		float RestTime;
		float LinearRestThresholdSquared;
		float AngularRestThresholdSquared;

		SimulationConfig()
		{
			ERP = 0.1f;
			CFM = 0.001f;
			MaxIterations = 12;
			MaximumCorrectingVelocity = 100.0f;
			ContactSurfaceLayer = 0.02f;
			Elasticity = 0.2f;
			LinearDrag = 0.0f;
			AngularDrag = 0.0f;
			Friction = 100.0f;
			Gravity = 20.0f;
			QuickStep = true;
			RestTime = 0.2f;
			LinearRestThresholdSquared = 0.2f * 0.2f;
			AngularRestThresholdSquared = 0.2f * 0.2f;
		}  
	};

	// new simulation state

	struct SimulationObjectState
	{
		SimulationObjectState()
		{
			enabled = true;
			scale = 1.0f;
			density = 1.0f;
			position = math::Vector(0,0,0);
	 		orientation = math::Quaternion(1,0,0,0);
			linearVelocity = math::Vector(0,0,0);
			angularVelocity = math::Vector(0,0,0);
		}

		bool enabled;
		float scale;
		float density;
		math::Vector position;
		math::Quaternion orientation;
		math::Vector linearVelocity;
		math::Vector angularVelocity;
	};

	// interaction pair for walking contacts

	struct InteractionPair
	{
		int a,b;
	};

	// simulation class with dynamic object allocation

	class Simulation
	{	
		static int* GetInitCount()
		{
			static int initCount = 0;
			return &initCount;
		}

	public:

		Simulation()
		{
			int * initCount = GetInitCount();
			if ( *initCount == 0 )
				dInitODE();
			(*initCount)++;

			world = 0;
			space = 0;
			contacts = 0;
		}

		void Initialize( const SimulationConfig & config = SimulationConfig() )
		{
			this->config = config;

			// create simulation

			world = dWorldCreate();
		    contacts = dJointGroupCreate( 0 );
			space = dHashSpaceCreate( 0 );

			// configure world

			dWorldSetERP( world, config.ERP );
			dWorldSetCFM( world, config.CFM );
			dWorldSetQuickStepNumIterations( world, config.MaxIterations );
			dWorldSetGravity( world, 0, 0, -config.Gravity );
			dWorldSetContactSurfaceLayer( world, config.ContactSurfaceLayer );
			dWorldSetContactMaxCorrectingVel( world, config.MaximumCorrectingVelocity );
			dWorldSetLinearDamping( world, 0.01f );
			dWorldSetAngularDamping( world, 0.01f );

			// setup contacts

		    for ( int i = 0; i < MaxContacts; i++ ) 
		    {
				contact[i].surface.mode = dContactBounce;
				contact[i].surface.mu = config.Friction;
				contact[i].surface.bounce = config.Elasticity;
				contact[i].surface.bounce_vel = 0.001f;
		    }
		
			objects.resize( 32 );
			interactionPairs.reserve( 32 );
		}

		~Simulation()
		{
			if ( contacts )
				dJointGroupDestroy( contacts );
			if ( world )
				dWorldDestroy( world );
			if ( space )
				dSpaceDestroy( space );

			int * initCount = GetInitCount();
			(*initCount)--;
			if ( *initCount == 0 )
				dCloseODE();
		}

		void Update( float deltaTime )
		{		
			interactionPairs.clear();

			dJointGroupEmpty( contacts );

			dSpaceCollide( space, this, NearCallback );

			if ( config.QuickStep )
				dWorldQuickStep( world, deltaTime );
			else
				dWorldStep( world, deltaTime );

			for ( int i = 0; i < (int) objects.size(); ++i )
			{
				if ( objects[i].exists() )
				{
					const dReal * linearVelocity = dBodyGetLinearVel( objects[i].body );
					const dReal * angularVelocity = dBodyGetAngularVel( objects[i].body );

					const float linearVelocityLengthSquared = linearVelocity[0]*linearVelocity[0] + linearVelocity[1]*linearVelocity[1] + linearVelocity[2]*linearVelocity[2];
					const float angularVelocityLengthSquared = angularVelocity[0]*angularVelocity[0] + angularVelocity[1]*angularVelocity[1] + angularVelocity[2]*angularVelocity[2];

					if ( linearVelocityLengthSquared < config.LinearRestThresholdSquared && angularVelocityLengthSquared < config.AngularRestThresholdSquared )
						objects[i].timeAtRest += deltaTime;
					else
						objects[i].timeAtRest = 0.0f;

					if ( objects[i].timeAtRest >= config.RestTime )
						dBodyDisable( objects[i].body );
					else
						dBodyEnable( objects[i].body );
				}
			}
		}

		int AddObject( const SimulationObjectState & initialObjectState )
		{
			// find free object slot

			int id = -1;
			for ( int i = 0; i < (int) objects.size(); ++i )
			{
				if ( !objects[i].exists() )
				{
					id = i;
					break;
				}
			}
			if ( id == -1 )
			{
				id = objects.size();
				objects.resize( objects.size() + 1 );
			}

			// setup object body

			objects[id].body = dBodyCreate( world );

			assert( objects[id].body );

			dMass mass;
			dMassSetBox( &mass, initialObjectState.density, initialObjectState.scale, initialObjectState.scale, initialObjectState.scale );
			dBodySetMass( objects[id].body, &mass );
			dBodySetData( objects[id].body, (void*) id );

			// setup geom and attach to body

			objects[id].scale = initialObjectState.scale;
			objects[id].geom = dCreateBox( space, initialObjectState.scale, initialObjectState.scale, initialObjectState.scale );

			dGeomSetBody( objects[id].geom, objects[id].body );	

			// set object state

			SetObjectState( id, initialObjectState );

			// success!

			return id;
		}

		bool ObjectExists( int id )
		{
			assert( id >= 0 && id < (int) objects.size() );
			return objects[id].exists();
		}

		float GetObjectMass( int id )
		{
			assert( id >= 0 && id < (int) objects.size() );
			assert( objects[id].exists() );
			dMass mass;
			dBodyGetMass( objects[id].body, &mass );
			return mass.mass;
		}

		void RemoveObject( int id )
		{
			assert( id >= 0 && id < (int) objects.size() );
			assert( objects[id].exists() );

			dBodyDestroy( objects[id].body );
			dGeomDestroy( objects[id].geom );
			objects[id].body = 0;
			objects[id].geom = 0;
		}

		void GetObjectState( int id, SimulationObjectState & objectState )
		{
			assert( id >= 0 );
			assert( id < (int) objects.size() );

			assert( objects[id].exists() );

			const dReal * position = dBodyGetPosition( objects[id].body );
			const dReal * orientation = dBodyGetQuaternion( objects[id].body );
			const dReal * linearVelocity = dBodyGetLinearVel( objects[id].body );
			const dReal * angularVelocity = dBodyGetAngularVel( objects[id].body );

			objectState.position = math::Vector( position[0], position[1], position[2] );
			objectState.orientation = math::Quaternion( orientation[0], orientation[1], orientation[2], orientation[3] );
			objectState.linearVelocity = math::Vector( linearVelocity[0], linearVelocity[1], linearVelocity[2] );
			objectState.angularVelocity = math::Vector( angularVelocity[0], angularVelocity[1], angularVelocity[2] );

			objectState.enabled = objects[id].timeAtRest < config.RestTime;
		}

		void SetObjectState( int id, const SimulationObjectState & objectState, bool ignoreEnabledFlag = false )
		{
			assert( id >= 0 );
			assert( id < (int) objects.size() );

			assert( objects[id].exists() );

			dQuaternion quaternion;
			quaternion[0] = objectState.orientation.w;
			quaternion[1] = objectState.orientation.x;
			quaternion[2] = objectState.orientation.y;
			quaternion[3] = objectState.orientation.z;

			dBodySetPosition( objects[id].body, objectState.position.x, objectState.position.y, objectState.position.z );
			dBodySetQuaternion( objects[id].body, quaternion );
			dBodySetLinearVel( objects[id].body, objectState.linearVelocity.x, objectState.linearVelocity.y, objectState.linearVelocity.z );
			dBodySetAngularVel( objects[id].body, objectState.angularVelocity.x, objectState.angularVelocity.y, objectState.angularVelocity.z );

			if ( !ignoreEnabledFlag )
			{
				if ( objectState.enabled )
				{
					objects[id].timeAtRest = 0.0f;
					dBodyEnable( objects[id].body );
				}
				else
				{
					objects[id].timeAtRest = config.RestTime;
					dBodyDisable( objects[id].body );
				}
			}
		}

		const InteractionPair * GetInteractionPairs() const
		{
			// returning &interactionPairs[0] caused an array bounds error with in a debug build with vs2008 - h3r3tic
			return interactionPairs.size() > 0 ? &interactionPairs[0] : NULL;
		}

		int GetNumInteractionPairs() const
		{
			return interactionPairs.size();
		}

		void ApplyForce( int id, const math::Vector & force )
		{
			assert( id >= 0 );
			assert( id < (int) objects.size() );
			assert( objects[id].exists() );
			if ( force.length() > 0.001f )
			{
				objects[id].timeAtRest = 0.0f;
				dBodyEnable( objects[id].body );
				dBodyAddForce( objects[id].body, force.x, force.y, force.z );
			}
		}

		void ApplyTorque( int id, const math::Vector & torque )
		{
			assert( id >= 0 );
			assert( id < (int) objects.size() );
			assert( objects[id].exists() );
			if ( torque.length() > 0.001f )
			{
				objects[id].timeAtRest = 0.0f;
				dBodyEnable( objects[id].body );
				dBodyAddTorque( objects[id].body, torque.x, torque.y, torque.z );
			}
		}

		void AddPlane( const math::Vector & normal, float d )
		{
			planes.push_back( dCreatePlane( space, normal.x, normal.y, normal.z, d ) );
		}

		void Reset()
		{
			for ( int i = 0; i < (int) objects.size(); ++i )
			{
				if ( objects[i].exists() )
					RemoveObject( i );
			}

			for ( int i = 0; i < (int) planes.size(); ++i )
				dGeomDestroy( planes[i] );

			planes.clear();
		}

	private:

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
				return body != 0 && geom != 0;
			}
		};

		SimulationConfig config;
		std::vector<dGeomID> planes;
		std::vector<ObjectData> objects;
		std::vector<InteractionPair> interactionPairs;

	protected:

		enum { MaxContacts = 8 };
	    dContact contact[MaxContacts];			

		static void NearCallback( void * data, dGeomID o1, dGeomID o2 )
		{
			Simulation * simulation = (Simulation*) data;

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

				if ( b1 && b2 )
				{
					int64_t objectId1 = reinterpret_cast<uint64_t>( dBodyGetData( b1 ) );
					int64_t objectId2 = reinterpret_cast<uint64_t>( dBodyGetData( b2 ) );

					assert( objectId1 >= 0 );
					assert( objectId2 >= 0 );
					assert( objectId1 < (int) simulation->objects.size() );
					assert( objectId2 < (int) simulation->objects.size() );
					assert( objectId1 != objectId2 );

					/* 
						The simulation may generate multiple contacts
						between the same object, so here we filter out
						unique contacts only. We consider a->b the same
						as b->a so we only want one of these pairs.
					*/

					bool unique = true;
					for ( int i = 0; i < (int) simulation->interactionPairs.size(); ++i )
					{
						if ( simulation->interactionPairs[i].a == objectId1 &&
							 simulation->interactionPairs[i].b == objectId2 ||
							 simulation->interactionPairs[i].a == objectId2 &&
						     simulation->interactionPairs[i].b == objectId1 )
						{
							unique = false;
							break;
						}
					}

					if ( unique )
					{
						int size = simulation->interactionPairs.size();
						simulation->interactionPairs.resize( size + 1 );
						simulation->interactionPairs[size].a = objectId1;
						simulation->interactionPairs[size].b = objectId2;
					}
				}
			}
		}
	};
}

#endif
