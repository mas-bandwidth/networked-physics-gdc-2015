/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#include "shared/Cubes.h"

class InterpolationExtrapolationDemo : public Demo
{
private:

	game::Instance<cubes::DatabaseObject, cubes::ActiveObject> * gameInstance;
	GameWorkerThread workerThread;
	view::Packet viewPacket;
	view::ObjectManager viewObjectManager[2];
	render::Interface * renderInterface;
	Camera camera[2];
	math::Vector origin[2];
	float t;
	int accumulator;
	int sendRate;
	float interpolation_t;
	bool follow;
	bool tabDownLastFrame;
	bool tildeDownLastFrame;
	InterpolationMode interpolationMode;

	enum { steps = 1024 };

public:

	InterpolationExtrapolationDemo()
	{
        game::Config config;
        config.maxObjects = steps * steps + MaxPlayers + 1;
        config.deactivationTime = 0.5f;
        config.cellSize = 4.0f;
        config.cellWidth = steps / config.cellSize + 2;
        config.cellHeight = config.cellWidth;
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
		t = 0.0f;
		accumulator = 0;
		sendRate = 6;
		interpolation_t = 0.0f;
		follow = true;
		tabDownLastFrame = false;
		tildeDownLastFrame = false;
		interpolationMode = INTERPOLATE_None;
		renderInterface = NULL;
	}

	~InterpolationExtrapolationDemo()
	{
		delete gameInstance;
	}

	void InitializeWorld()
	{
		gameInstance->InitializeBegin();

		gameInstance->AddPlane( math::Vector(0,0,1), 0 );

		AddCube( gameInstance, 1.5f, math::Vector(0,-5,10) );

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

		gameInstance->SetFlag( game::FLAG_Push );
		gameInstance->SetFlag( game::FLAG_Pull );
	}
	
    void SetRenderInterface( render::Interface * renderInterface )
    {
        this->renderInterface = renderInterface;
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

		if ( input.f1 )
			sendRate = 1;

		if ( input.f2 )
			sendRate = 3;

		if ( input.f3 )
			sendRate = 6;

		if ( input.f4 )
			sendRate = 10;

		if ( input.f5 )
			sendRate = 20;

		if ( input.f6 )
			sendRate = 30;

        if ( input.f7 )
            sendRate = 60;

		if ( input.tilde && !tildeDownLastFrame )
		{
			follow = !follow;
		}
		tildeDownLastFrame = input.tilde;

		if ( input.one )
			interpolationMode = INTERPOLATE_None;

        if ( input.two )
            interpolationMode = INTERPOLATE_Linear;

		if ( input.three )
			interpolationMode = INTERPOLATE_Hermite;

		if ( input.four )
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
		workerThread.Start( gameInstance );
		t += deltaTime;
	}

    void WaitForSim()
    {
        workerThread.Join();
        gameInstance->GetViewPacket( viewPacket );
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

        bool updateState = false;
		if ( ++accumulator >= sendRate )
		{
            updateState = true;
			accumulator = 0;
			interpolation_t = 0.0f;
		}
        view::ObjectUpdate updates[MaxViewObjects];
        getViewObjectUpdates( updates, viewPacket );
        viewObjectManager[1].UpdateObjects( updates, viewPacket.objectCount, updateState );
		viewObjectManager[1].InterpolateObjects( interpolation_t, 1.0f / 60.0f * sendRate, interpolationMode );
		viewObjectManager[1].Update( deltaTime );
		interpolation_t += 1.0f / sendRate;

		// update cameras

		view::Object * playerCube = viewObjectManager[0].GetObject( 1 );
		if ( playerCube )
			origin[0] = playerCube->position + playerCube->positionError;

		math::Vector lookat = origin[0] - math::Vector(0,0,1);
		#ifdef WIDESCREEN
		math::Vector position = lookat + math::Vector(0,-12.5,6);
		#else
		math::Vector position = lookat + math::Vector(0,-10,5);
		#endif

		if ( !follow )
		{
			lookat = math::Vector( 0.0f, 0.0f, 0.0f );
			position = math::Vector( 0.0f, -15.0f, 4.0f );
		}

		camera[0].EaseIn( lookat, position ); 

		if ( interpolationMode != INTERPOLATE_None )
		{
			view::Object * playerCube = viewObjectManager[1].GetObject( 1 );
			if ( playerCube )
				origin[1] = playerCube->interpolatedPosition;

			math::Vector lookat = origin[1] - math::Vector(0,0,1);
			#ifdef WIDESCREEN
			math::Vector position = lookat + math::Vector(0,-12.5,6);
			#else
			math::Vector position = lookat + math::Vector(0,-10,5);
			#endif

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

		renderInterface->ClearScreen();

		int width = renderInterface->GetDisplayWidth();
		int height = renderInterface->GetDisplayHeight();

		Cubes cubes;
		viewObjectManager[0].GetRenderState( cubes );
		setCameraAndLight( renderInterface, camera[0] );
		renderInterface->BeginScene( 0, 0, width/2, height );
		ActivationArea activationArea;
		setupActivationArea( activationArea, origin[0], 5.0f, t );
		renderInterface->RenderActivationArea( activationArea, 1.0f );
		renderInterface->RenderCubes( cubes );
		#ifdef SHADOWS
		if ( shadows )
			renderInterface->RenderCubeShadows( cubes );
		#endif

		// render the interpolated scene (right)

		viewObjectManager[1].GetRenderState( cubes, true );
		setCameraAndLight( renderInterface, camera[1] );
		renderInterface->BeginScene( width/2, 0, width, height );
		setupActivationArea( activationArea, origin[1], 5.0f, t );
		renderInterface->RenderActivationArea( activationArea, 1.0f );
		renderInterface->RenderCubes( cubes );
		#ifdef SHADOWS
		if ( shadows )
			renderInterface->RenderCubeShadows( cubes );
		#endif

		// shadow quad on top

		#ifdef SHADOWS
		if ( shadows )
			renderInterface->RenderShadowQuad();
		#endif

        // divide splitscreen

        renderInterface->DivideSplitscreen();
	}
};
