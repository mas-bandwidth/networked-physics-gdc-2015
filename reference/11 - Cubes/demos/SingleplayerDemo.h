/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#include "demos/Demo.h"
#include "shared/Hypercube.h"

class SingleplayerDemo : public Demo
{
private:

	game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * gameInstance;
	GameWorkerThread workerThread;
	render::Interface * renderInterface;
	float t;           // NETHACK: floating point time is a very bad idea
	Camera camera;
	math::Vector origin;

    view::Packet viewPacket;
    view::ObjectManager viewObjectManager;
    	
public:

	enum { steps = 1024 };

	SingleplayerDemo()
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

		gameInstance = new game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> ( config );
		t = 0.0f;
		origin = math::Vector(0,0,0);
		renderInterface = NULL;
	}
	
	~SingleplayerDemo()
	{
		delete gameInstance;
	}
	
	void InitializeWorld()
	{
		gameInstance->InitializeBegin();

		gameInstance->AddPlane( math::Vector(0,0,1), 0 );

		AddCube( gameInstance, 1, math::Vector(0,0,10) );

		const int border = 10.0f;
		const float origin = -steps / 2 + border;
		const float z = hypercube::NonPlayerCubeSize / 2;
		const int count = steps - border * 2;
		for ( int y = 0; y < count; ++y )
			for ( int x = 0; x < count; ++x )
				AddCube( gameInstance, 0, math::Vector(x+origin,y+origin,z) );

		gameInstance->InitializeEnd();

		gameInstance->OnPlayerJoined( 0 );
		gameInstance->SetLocalPlayer( 0 );
		gameInstance->SetPlayerFocus( 0, 1 );
	
		gameInstance->SetFlag( game::FLAG_Push );
		gameInstance->SetFlag( game::FLAG_Pull );
	}
	
	void SetRenderInterface( render::Interface * renderInterface )
	{
		this->renderInterface = renderInterface;
	}
    
	void AddCube( game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * gameInstance, int player, const math::Vector & position )
	{
		hypercube::DatabaseObject object;
		CompressPosition( position, object.position );
		CompressOrientation( math::Quaternion(1,0,0,0), object.orientation );
		object.dirty = player;
		object.enabled = player;
		object.session = 0;
		object.player = player;
		ObjectId id = gameInstance->AddObject( object, position.x, position.y );
		if ( player )
			gameInstance->DisableObject( id );
	}

	void ProcessInput( const platform::Input & input )
	{
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
		// start the worker thread
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
		// update the scene to be rendered
		
		if ( viewPacket.objectCount >= 1 )
		{
			view::ObjectUpdate updates[MaxViewObjects];
			getViewObjectUpdates( updates, viewPacket );
			viewObjectManager.UpdateObjects( updates, viewPacket.objectCount );
		}

		viewObjectManager.ExtrapolateObjects( deltaTime );
		viewObjectManager.Update( deltaTime );

        // update camera

        view::Object * playerCube = viewObjectManager.GetObject( 1 );
        if ( playerCube )
            origin = playerCube->position + playerCube->positionError;
        math::Vector lookat = origin - math::Vector(0,0,1);
        #ifdef WIDESCREEN
        math::Vector position = lookat + math::Vector(0,-11,5);
        #else
        math::Vector position = lookat + math::Vector(0,-12,6);
        #endif
        camera.EaseIn( lookat, position ); 

		// render the scene
		
		renderInterface->ClearScreen();

		Cubes cubes;
		viewObjectManager.GetRenderState( cubes );

		int width = renderInterface->GetDisplayWidth();
		int height = renderInterface->GetDisplayHeight();

		renderInterface->BeginScene( 0, 0, width, height );
		setCameraAndLight( renderInterface, camera );
		ActivationArea activationArea;
		view::setupActivationArea( activationArea, origin, 5.0f, t );
		renderInterface->RenderActivationArea( activationArea, 1.0f );
		renderInterface->RenderCubes( cubes );
		#ifdef SHADOWS
		if ( shadows )
		{
			renderInterface->RenderCubeShadows( cubes );
			renderInterface->RenderShadowQuad();
		}
		#endif
	}
};
