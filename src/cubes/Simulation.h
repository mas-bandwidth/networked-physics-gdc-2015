/*
    Networked Physics Demo

    Copyright © 2008 - 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CUBES_SIMULATION_H
#define CUBES_SIMULATION_H

#include "Config.h"
#include "Mathematics.h"
#include <vector>

namespace cubes
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
			MaximumCorrectingVelocity = 10.0f;
			ContactSurfaceLayer = 0.02f;
			Elasticity = 0.2f;
			LinearDrag = 0.0f;
			AngularDrag = 0.0f;
			Friction = 100.0f;
			Gravity = 20.0f;
			QuickStep = true;
			RestTime = 0.1f;
			LinearRestThresholdSquared = 0.25f * 0.25f;
			AngularRestThresholdSquared = 0.25f * 0.25f;
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
