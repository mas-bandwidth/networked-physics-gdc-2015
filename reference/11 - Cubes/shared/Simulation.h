/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef SIMULATION_H
#define SIMULATION_H

#include "Config.h"

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
		bool enabled;
		float scale;
		math::Vector position;
		math::Quaternion orientation;
		math::Vector linearVelocity;
		math::Vector angularVelocity;
	};

	// simulation class with dynamic object allocation

	class Simulation
	{	
		static int * GetInitCount();

	public:

		Simulation();
		~Simulation();

		void Initialize( const SimulationConfig & config = SimulationConfig() );

		void Update( float deltaTime, bool paused = false );

		int AddObject( const SimulationObjectState & initialObjectState );

		bool ObjectExists( int id );

		float GetObjectMass( int id );

		void RemoveObject( int id );
		
		void GetObjectState( int id, SimulationObjectState & objectState );

		void SetObjectState( int id, const SimulationObjectState & objectState, bool ignoreEnabledFlag = false );

		const std::vector<uint16_t> & GetObjectInteractions( int id ) const;

		int GetNumInteractionPairs() const;

		void ApplyForce( int id, const math::Vector & force );

		void ApplyTorque( int id, const math::Vector & torque );

		void AddPlane( const math::Vector & normal, float d );

		void Reset();

	private:

		struct SimulationImpl * impl;
	};
}

#endif
