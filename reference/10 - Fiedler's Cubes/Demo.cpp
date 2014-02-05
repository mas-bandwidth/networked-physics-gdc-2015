/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#include <assert.h>

#if PLATFORM != PLATFORM_WINDOWS
	#include <unistd.h>
#endif

#include "Game.h"
#include "View.h"
#include "Render.h"

using namespace net;
using namespace game;
using namespace view;
using namespace render;
using namespace engine;

const float DeltaTime = 1.0f / 60.0f;

// -------------------------------------------------------------------------

class GameWorkerThread : public WorkerThread
{
public:
	
	GameWorkerThread()
	{
		instance = NULL;
		time = 0.0f;
	} 
	
	void Start( game::Interface * instance )
	{
		assert( instance );
		this->instance = instance;
		WorkerThread::Start();
	}
	
	float GetTime() const
	{
		return time;
	}
	
private:
	
	virtual void Run()
	{
		platform::Timer timer;
		instance->Update( DeltaTime );
 		time = timer.delta();
	}
	
	game::Interface * instance;
	float time;
};

// ------------------------------------------------------

class Demo
{
public:
	
	virtual ~Demo() {}
	virtual void Initialize() = 0;
	virtual void ProcessInput( const platform::Input & input ) = 0;
	virtual void Update( float deltaTime ) = 0;
	virtual void Render( float deltaTime, bool shadows ) = 0;
	virtual void PostRender() = 0;
};

#include "SingleplayerDemo.h"
#include "InterpolationDemo.h"
#include "StateReplicationDemo.h"
#include "AuthorityDemo.h"
#include "CorrectionsDemo.h"

// --------------------------------------------------------------

Demo * CreateDemo( int index, int width, int height )
{
	if ( index == 0 )
		return new SingleplayerDemo( width, height );
		
	if ( index == 1 )
		return new InterpolationDemo( width, height );
		
	if ( index == 2 )
		return new StateReplicationDemo( width, height );
		
	if ( index == 3 )
		return new AuthorityDemo( width, height );
		
	if ( index == 4 )
		return new CorrectionsDemo( width, height );
		
	return NULL;
}

void ClearDisplay( int displayWidth, int displayHeight )
{
	glViewport( 0, 0, displayWidth, displayHeight );
	glDisable( GL_SCISSOR_TEST );
	glClearStencil( 0 );
	glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );		
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	platform::UpdateDisplay();
}

int main( int argc, char * argv[] )
{	
	int displayWidth = DisplayWidth;
	int displayHeight = DisplayHeight;
	
	if ( !OpenDisplay( "Fiedler's Cubes", displayWidth, displayHeight ) )
	{
		printf( "failed to open display!\n" );
		return 1;
	}
	
	HideMouseCursor();
	
	int currentDemo = 0;
	Demo * demo = CreateDemo( 0, displayWidth, displayHeight );
	assert( demo );
	demo->Initialize();
	
	bool shadows = true;
	
	while ( true )
	{
		platform::Input input = platform::Input::Sample();
		
		if ( input.alt )
		{
			int demoIndex = -1;
			
			if ( input.one )
				demoIndex = 0;
				
			if ( input.two )
				demoIndex = 1;
				
			if ( input.three )
				demoIndex = 2;
				
			if ( input.four )
				demoIndex = 3;
				
			if ( input.five )
				demoIndex = 4;
				
			if ( input.six )
				demoIndex = 5;
				
			if ( input.seven )
				demoIndex = 6;
				
			if ( input.eight )
				demoIndex = 7;
				
			if ( input.nine )
				demoIndex = 8;
				
			if ( input.zero )
				demoIndex = 9;

			if ( input.f4 )
				break;

			static bool enterDownLastFrame = false;
			if ( input.enter && !enterDownLastFrame )
				shadows = !shadows;
			enterDownLastFrame = input.enter;
				
			if ( demoIndex != -1 )
			{
				Demo * newDemo = CreateDemo( demoIndex, displayWidth, displayHeight );
				if ( newDemo )
				{
					ClearDisplay( displayWidth, displayHeight );
					delete demo;
					demo = newDemo;
					assert( demo );
					demo->Initialize();
					currentDemo = demoIndex;
				}
			}
		}
		
		static bool escapeDownLastFrame = false;		
		if ( input.escape && !escapeDownLastFrame )
		{
			ClearDisplay( displayWidth, displayHeight );
			delete demo;
			demo = CreateDemo( currentDemo, displayWidth, displayHeight );
			assert( demo );
			demo->Initialize();
		}
		escapeDownLastFrame = input.escape;
		
		demo->ProcessInput( !input.alt ? input : platform::Input() );
		
		demo->Update( DeltaTime );
		
		demo->Render( DeltaTime, shadows );
		
		UpdateDisplay( 1 );
		
		demo->PostRender();
	}

	CloseDisplay();

	delete demo;
	
	return 0;
}
