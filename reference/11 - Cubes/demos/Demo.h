/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef DEMO_H
#define DEMO_H

#include "PreCompiled.h"
#include "shared/Game.h"
#include "client/View.h"
#include "client/Render.h"

using namespace game;
using namespace view;
#ifdef HAS_OPENGL
using namespace render;
#endif
using namespace engine;
using namespace platform;

const float DeltaTime = 1.0f / 60.0f;

// -------------------------------------------------------------------------

class GameWorkerThread : public WorkerThread
{
public:
	
	GameWorkerThread()
	{
		instance = NULL;
        numSteps = 1;
	} 
	
	void Start( game::Interface * instance, float deltaTime = DeltaTime, int numSteps = 1 )
	{
		assert( instance );
		this->instance = instance;
        this->deltaTime = deltaTime;
        this->numSteps = numSteps;
		WorkerThread::Start();
	}
	
	#ifdef HAS_OPENGL
	float GetSimTime() const
	{
		return simTime;
	}
	#endif
	
private:
	
	virtual void Run()
	{
		// TODO: port timer to linux
		#ifdef HAS_OPENGL
		platform::Timer timer;
		#endif
        for ( int i = 0; i < numSteps; ++i )
		  instance->Update( deltaTime );
		#ifdef HAS_OPENGL
		simTime = timer.delta();
		#endif
	}
	
	#ifdef HAS_OPENGL
	float simTime;
	#endif
	game::Interface * instance;
    int numSteps;
    float deltaTime;
};

// ------------------------------------------------------

class Demo
{
public:
	
	virtual ~Demo() {}
	virtual void InitializeWorld() = 0;
	virtual void SetRenderInterface( render::Interface * renderInterface ) = 0;
	virtual void ProcessInput( const platform::Input & input ) = 0;
	virtual void Update( float deltaTime ) = 0;
	virtual void Render( float deltaTime, bool shadows ) = 0;
	virtual void WaitForSim() = 0;
};

#endif
