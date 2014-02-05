// Common data (avoids coupling)

#ifndef COMMON_H
#define COMMON_H

// configuration

const int DisplayWidth = 640;
const int DisplayHeight = 480;

const unsigned short ServerPort = 40001;
const unsigned short ClientPort = 40002;

const int RandomSeed = 21;

const int MaxCubes = 64;
const float ShadowDistance = 10.0f;
const float ColorChangeTightness = 0.05f;
const float GhostScale = 0.95f;
const float Boundary = 10.0f;

// simulation configuration

struct SimulationConfiguration
{
	float ERP;
	float CFM;
 	int MaxIterations;
	float Gravity;
	float Friction;
	float Elasticity;
	float ContactSurfaceLayer;
	float MaximumCorrectingVelocity;
	bool QuickStep;
	int MaxCubes;
	
	SimulationConfiguration()
	{
		ERP = 0.001f;
		CFM = 0.001f;
		MaxIterations = 32;
		MaximumCorrectingVelocity = 100.0f;
		ContactSurfaceLayer = 0.001f;
		Elasticity = 0.8f;
		Friction = 15.0f;
		Gravity = 20.0f;
		QuickStep = true;
		MaxCubes = 32;
	}
};

// abstract render state

struct RenderState
{
	RenderState()
	{
		numCubes = 0;
	}
	
	struct Cube
	{
		math::Vector position;
		math::Quaternion orientation;
		float scale;
		float r,g,b,a;
		bool operator < ( const Cube & other ) const { return position.y > other.position.y; }		// for back to front sort only!
	};

	struct HibernationArea
	{
		HibernationArea()
		{
			show = false;
		}
		bool show;
		math::Vector origin;
		float radius;
		float startAngle;
		float r,g,b,a;
	};

	unsigned int numCubes;
	Cube cubes[MaxCubes];
	HibernationArea hibernationArea;
};

// new simulation state

struct SimulationCubeState
{
	SimulationCubeState()
	{
		position = math::Vector(0,0,0);
 		orientation = math::Quaternion(1,0,0,0);
		linearVelocity = math::Vector(0,0,0);
		angularVelocity = math::Vector(0,0,0);
		enabled = true;
	}
	
	math::Vector position;
	math::Quaternion orientation;
	math::Vector linearVelocity;
	math::Vector angularVelocity;
	bool enabled;
};

struct SimulationInitialCubeState : public SimulationCubeState
{
	SimulationInitialCubeState()
	{
		scale = 1.0f;
		density = 1.0f;
	}
	
	float scale;
	float density;
};

#endif
