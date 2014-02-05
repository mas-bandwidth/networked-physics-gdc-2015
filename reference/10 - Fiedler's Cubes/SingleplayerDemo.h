/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#include "Hypercube.h"

class SingleplayerDemo : public Demo
{
private:

	game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * gameInstance;
	GameWorkerThread workerThread;
	view::Packet viewPacket;
	view::ObjectManager viewObjectManager;
	render::Render * render;
	float t;
	Camera camera;
	math::Vector origin;
	
public:

	enum { steps = 1024 };

	SingleplayerDemo( int displayWidth, int displayHeight )
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
		render = new render::Render( displayWidth, displayHeight );
		t = 0.0f;
		origin = math::Vector(0,0,0);
	}
	
	~SingleplayerDemo()
	{
		delete gameInstance;
		delete render;
	}
	
	void Initialize()
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
	
		gameInstance->SetFlag( game::FLAG_Hover );
		gameInstance->SetFlag( game::FLAG_Katamari );
	}
	
	void AddCube( game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * gameInstance, int player, const math::Vector & position )
	{
		hypercube::DatabaseObject object;
		CompressPosition( position, object.position );
		CompressOrientation( math::Quaternion(1,0,0,0), object.orientation );
		object.enabled = player;
		object.activated = 0;
		object.confirmed = 0;
		object.corrected = 0;
		object.player = player;
		gameInstance->AddObject( object, position.x, position.y );
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
		// update camera
		view::Object * playerCube = viewObjectManager.GetObject( 1 );
		if ( playerCube )
			origin = playerCube->position + playerCube->positionError;
		math::Vector lookat = origin;
		math::Vector position = lookat + math::Vector(0,-10,5);
		camera.EaseIn( lookat, position ); 

		// grab the view packet & start the worker thread...
		gameInstance->GetViewPacket( viewPacket );
		workerThread.Start( gameInstance );
		t += deltaTime;
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

		// render the scene
		
		render->ClearScreen();

		Cubes cubes;
		viewObjectManager.GetRenderState( cubes );

		int width = render->GetDisplayWidth();
		int height = render->GetDisplayHeight();

		render->BeginScene( 0, 0, width, height );
		setCameraAndLight( render, camera );
		ActivationArea activationArea;
		view::setupActivationArea( activationArea, origin, 5.0f, t );
		render->RenderActivationArea( activationArea, 1.0f );
		render->RenderCubes( cubes );
		if ( shadows )
		{
			render->RenderCubeShadows( cubes );
			render->EnterScreenSpace();
			render->RenderShadowQuad();
		}
	}
	
	void PostRender()
	{
		// join worker thread
		workerThread.Join();
	}
};
