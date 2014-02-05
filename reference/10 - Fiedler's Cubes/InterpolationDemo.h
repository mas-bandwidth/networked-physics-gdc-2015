/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#include "Cubes.h"

class InterpolationDemo : public Demo
{
private:

	game::Instance<cubes::DatabaseObject, cubes::ActiveObject> * gameInstance;
	GameWorkerThread workerThread;
	view::Packet viewPacket;
	view::ObjectManager viewObjectManager[2];
	render::Render * render;
	Camera camera[2];
	math::Vector origin[2];
	float t;
	int accumulator;
	int sendRate;
	float interpolation_t;
	bool strobe;
	bool follow;
	bool tabDownLastFrame;
	bool tildeDownLastFrame;
	InterpolationMode interpolationMode;

public:

	InterpolationDemo( int displayWidth, int displayHeight )
	{
		game::Config config;
		config.deactivationTime = 0.5f;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;
		config.activationDistance = 5.0f;
		config.simConfig.ERP = 0.1f;
		config.simConfig.CFM = 0.001f;
		config.simConfig.MaxIterations = 12;
		config.simConfig.MaximumCorrectingVelocity = 100.0f;
		config.simConfig.ContactSurfaceLayer = 0.05f;
		config.simConfig.Elasticity = 0.3f;
		config.simConfig.LinearDrag = 0.01f;
		config.simConfig.AngularDrag = 0.01f;
		config.simConfig.Friction = 200.0f;
		gameInstance = new game::Instance<cubes::DatabaseObject, cubes::ActiveObject> ( config );
		origin[0] = math::Vector(0,0,0);
		origin[1] = math::Vector(0,0,0);
		render = new render::Render( displayWidth, displayHeight );
		t = 0.0f;
		accumulator = 0;
		sendRate = 1;
		interpolation_t = 0.0f;
		strobe = true;
		follow = true;
		tabDownLastFrame = false;
		tildeDownLastFrame = false;
		interpolationMode = INTERPOLATE_Linear;
	}

	~InterpolationDemo()
	{
		delete gameInstance;
		delete render;
	}

	void Initialize()
	{
		gameInstance->InitializeBegin();

		gameInstance->AddPlane( math::Vector(0,0,1), 0 );

		AddCube( gameInstance, 1.5f, math::Vector(0,-5,10) );

		const int steps = 20;
		const int border = 1.0f;
		const float origin = -steps / 2 + border;
		const float z = 0.2f;
		const int count = steps - border * 2;
		for ( int y = 0; y < count; ++y )
			for ( int x = 0; x < count; ++x )
				AddCube( gameInstance, 0.4f, math::Vector(x+origin,y+origin,z) );

		gameInstance->InitializeEnd();

		gameInstance->OnPlayerJoined( 0 );
		gameInstance->SetPlayerFocus( 0, 1 );
		gameInstance->SetLocalPlayer( 0 );

		gameInstance->SetFlag( game::FLAG_Hover );
		gameInstance->SetFlag( game::FLAG_Katamari );
	}

	void AddCube( game::Instance<cubes::DatabaseObject, cubes::ActiveObject> * gameInstance, float scale, const math::Vector & position, const math::Vector & linearVelocity = math::Vector(0,0,0), const math::Vector & angularVelocity = math::Vector(0,0,0) )
	{
		cubes::DatabaseObject object;
		object.position = position;
		object.orientation = math::Quaternion(1,0,0,0);
		object.scale = scale;
		object.linearVelocity = linearVelocity;
		object.angularVelocity = angularVelocity;
		object.enabled = 1;
		object.activated = 0;
		gameInstance->AddObject( object, position.x, position.y );
	}

	void ProcessInput( const platform::Input & input )
	{
		// demo controls

		if ( input.one )
			sendRate = 1;

		if ( input.two )
			sendRate = 2;

		if ( input.three )
			sendRate = 3;

		if ( input.four )
			sendRate = 6;

		if ( input.five )
			sendRate = 10;

		if ( input.six )
			sendRate = 15;

		if ( input.seven )
			sendRate = 20;

		if ( input.eight )
			sendRate = 30;

		if ( input.nine )
			sendRate = 60;

		if ( input.zero )
			sendRate = 0;

		if ( input.tab && !tabDownLastFrame )
		{
			strobe = !strobe;
		}
		tabDownLastFrame = input.tab;

		if ( input.tilde && !tildeDownLastFrame )
		{
			follow = !follow;
		}
		tildeDownLastFrame = input.tilde;

		if ( input.f1 )
			interpolationMode = INTERPOLATE_Linear;

		if ( input.f2 )
			interpolationMode = INTERPOLATE_Hermite;

		if ( input.f3 )
			interpolationMode = INTERPOLATE_Extrapolate;

		// pass input to game instance

		game::Input gameInput;
		gameInput.left = input.left ? 1.0f : 0.0f;
		gameInput.right = input.right ? 1.0f : 0.0f;
		gameInput.up = input.up ? 1.0f : 0.0f;
		gameInput.down = input.down ? 1.0f : 0.0f;
		gameInput.push = input.space ? 1.0f : 0.0f;
		gameInput.pull = input.z ? 1.0f : 0.0f;

		gameInstance->SetPlayerInput( 0, gameInput );
	}

	void Update( float deltaTime )
	{
		// grab the view packet & start the worker thread...
		gameInstance->GetViewPacket( viewPacket );
		workerThread.Start( gameInstance );
		t += deltaTime;
	}

	void Render( float deltaTime, bool shadows )
	{
		// update the scene to be rendered (left)

		if ( viewPacket.objectCount >= 1 )
		{
			view::ObjectUpdate updates[MaxViewObjects];
			getViewObjectUpdates( updates, viewPacket );
			viewObjectManager[0].UpdateObjects( updates, viewPacket.objectCount );
		}
		viewObjectManager[0].ExtrapolateObjects( deltaTime );
		viewObjectManager[0].Update( deltaTime );

		// update the interpolated scene (right)

		if ( ++accumulator >= sendRate )
		{
			view::ObjectUpdate updates[MaxViewObjects];
			getViewObjectUpdates( updates, viewPacket );
			for ( int i = 0; i < (int) viewPacket.objectCount; ++i )
			{
				if ( updates[i].authority == 0 )
				{
					updates[i].authority = 1;
					getAuthorityColor( updates[i].authority, updates[i].r, updates[i].g, updates[i].b );
				}
			}	
			viewObjectManager[1].UpdateObjects( updates, viewPacket.objectCount );
			accumulator = 0;
			interpolation_t = 0.0f;
		}
		viewObjectManager[1].InterpolateObjects( interpolation_t, 1.0f / 60.0f * sendRate, interpolationMode );
		viewObjectManager[1].Update( deltaTime );
		if ( !strobe )
			interpolation_t += 1.0f / sendRate;
		else
			interpolation_t = 0.0f;

		// update cameras

		view::Object * playerCube = viewObjectManager[0].GetObject( 1 );
		if ( playerCube )
			origin[0] = playerCube->position + playerCube->positionError;

		math::Vector lookat = origin[0];
		math::Vector position = lookat + math::Vector(0,-10,5);

		if ( !follow )
		{
			lookat = math::Vector( 0.0f, 0.0f, 0.0f );
			position = math::Vector( 0.0f, -15.0f, 4.0f );
		}

		camera[0].EaseIn( lookat, position ); 

		if ( !strobe )
		{
			view::Object * playerCube = viewObjectManager[1].GetObject( 1 );
			if ( playerCube )
				origin[1] = playerCube->interpolatedPosition;

			math::Vector lookat = origin[1];
			math::Vector position = lookat + math::Vector(0,-10,5);

			if ( !follow )
			{
				lookat = math::Vector( 0.0f, 0.0f, 0.0f );
				position = math::Vector( 0.0f, -15.0f, 4.0f );
			}

			camera[1].EaseIn( lookat, position ); 
		}
		else
		{
			camera[1] = camera[0];
			origin[1] = origin[0];
		}

		// render the simulated scene (left)

		render->ClearScreen();

		int width = render->GetDisplayWidth();
		int height = render->GetDisplayHeight();

		Cubes cubes;
		viewObjectManager[0].GetRenderState( cubes );
		setCameraAndLight( render, camera[0] );
		render->BeginScene( 0, 0, width/2, height );
		ActivationArea activationArea;
		setupActivationArea( activationArea, origin[0], 5.0f, t );
		render->RenderActivationArea( activationArea, 1.0f );
		render->RenderCubes( cubes );
		if ( shadows )
			render->RenderCubeShadows( cubes );

		// render the interpolated scene (right)

		viewObjectManager[1].GetRenderState( cubes, true );
		setCameraAndLight( render, camera[1] );
		render->BeginScene( width/2, 0, width, height );
		setupActivationArea( activationArea, origin[1], 5.0f, t );
		render->RenderActivationArea( activationArea, 1.0f );
		render->RenderCubes( cubes );
		if ( shadows )
			render->RenderCubeShadows( cubes );

		// shadow quad on top

		if ( shadows )
		{
			render->EnterScreenSpace();
			render->RenderShadowQuad();
		}
	}

	void PostRender()
	{
		// join worker threads
		workerThread.Join();
	}
};
