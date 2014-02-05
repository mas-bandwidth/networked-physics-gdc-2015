/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#include "demos/Demo.h"
#include "shared/Hypercube.h"
#include <queue>

const int MaxFrames = 256;

struct Frame
{
    int number;
    float deltaTime;
    game::Input input;  
};

class DeterministicLockstepWorkerThread : public WorkerThread
{
public:
    
    DeterministicLockstepWorkerThread()
    {
        instance = NULL;
        numFrames = 0;
    } 
    
    void Start( game::Interface * instance, int numFrames, Frame * frames )
    {
        assert( frames <= MaxFrames );
        assert( instance );
        this->instance = instance;
        this->numFrames = numFrames;
        for ( int i = 0; i < numFrames; ++i )
            this->frames[i] = frames[i];
        WorkerThread::Start();
    }

private:
    
    virtual void Run()
    {
        for ( int i = 0; i < numFrames; ++i )
        {
            instance->SetPlayerInput( 0, frames[i].input );
            instance->Update( frames[i].deltaTime );
        }
    }
    
    game::Interface * instance;
    int numFrames;
    Frame frames[MaxFrames];
};

class DeterministicLockstepDemo : public Demo
{
public:

    enum Mode { Unsynced, Deterministic, NonDeterministic };

private:

    Mode mode;

	render::Interface * renderInterface;

    uint32_t frameNumber;

    struct Instance
    {
        game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * game;
        float t;
        int lag;
        bool updated;
        Camera camera;
        math::Vector origin;
        std::queue<Frame> frames;
        DeterministicLockstepWorkerThread workerThread;
        view::Packet viewPacket;
        view::ObjectManager viewObjectManager;
    };

    static const int NumInstances = 2;

    Instance instance[NumInstances];
	
public:

	enum { steps = 1024 };

	DeterministicLockstepDemo( Mode mode = Deterministic )
	{
        this->mode = mode;

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

        for ( int i = 0; i < NumInstances; ++i )
        {
    		instance[i].game = new game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> ( config );
    		instance[i].origin = math::Vector(0,0,0);
            instance[i].lag = ( i == 0 || mode == NonDeterministic ) ? 0 : 90;
            instance[i].t = 0.0f;
            instance[i].updated = false;
        }

        frameNumber = 0;

        renderInterface = NULL;
	}
	
	~DeterministicLockstepDemo()
	{
        for ( int i = 0; i < NumInstances; ++i )
		  delete instance[i].game;
	}
	
	void InitializeWorld()
	{
		for ( int i = 0; i < NumInstances; ++i )
        {
            instance[i].game->InitializeBegin();

    		instance[i].game->AddPlane( math::Vector(0,0,1), 0 );

    		AddCube( instance[i].game, 1, math::Vector(0,0,10) );

    		const int border = 10.0f;
    		const float origin = -steps / 2 + border;
    		const float z = hypercube::NonPlayerCubeSize / 2;
    		const int count = steps - border * 2;
    		for ( int y = 0; y < count; ++y )
    			for ( int x = 0; x < count; ++x )
    				AddCube( instance[i].game, 0, math::Vector(x+origin,y+origin,z) );

    		instance[i].game->InitializeEnd();

    		instance[i].game->OnPlayerJoined( 0 );
    		instance[i].game->SetLocalPlayer( 0 );
    		instance[i].game->SetPlayerFocus( 0, 1 );
    	
    		instance[i].game->SetFlag( game::FLAG_Push );
    		instance[i].game->SetFlag( game::FLAG_Pull );
        }
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
        // process debug inputs

        if ( input.one )
            instance[1].lag = 0;
        else if ( input.two )
            instance[1].lag = 10;
        else if ( input.three )
            instance[1].lag = 20;
        else if ( input.four )
            instance[1].lag = 40;
        else if ( input.five )
            instance[1].lag = 60;
        else if ( input.six )
            instance[1].lag = 90;

		// determine last game input
		
		game::Input gameInput;
		gameInput.left = input.left ? 1.0f : 0.0f;
		gameInput.right = input.right ? 1.0f : 0.0f;
		gameInput.up = input.up ? 1.0f : 0.0f;
		gameInput.down = input.down ? 1.0f : 0.0f;
		gameInput.push = input.space ? 1.0f : 0.0f;
		gameInput.pull = input.z ? 1.0f : 0.0f;

        // queue up frames for each instance to process

        for ( int i = 0; i < NumInstances; ++i )
        {
            Frame frame;
            frame.number = frameNumber;
            if ( mode == NonDeterministic && i != 0 )
                frame.deltaTime = DeltaTime + ( (frame.number%2) ? +1 : -1 ) * 0.0001f;
            else
                frame.deltaTime = DeltaTime;
            frame.input = gameInput;
            instance[i].frames.push( frame );
        }

        frameNumber++;
	}
	
	void Update( float deltaTime )
	{
        for ( int i = 0; i < NumInstances; ++i )
        {
            int numFrames = 0;
            Frame frames[MaxFrames];
            const int nextFrame = frameNumber - instance[i].lag;
            while ( instance[i].frames.size() && instance[i].frames.front().number < nextFrame && numFrames < MaxFrames )
            {
                frames[numFrames++] = instance[i].frames.front();
                instance[i].frames.pop();                
            }
            instance[i].workerThread.Start( instance[i].game, numFrames, frames );
            if ( numFrames > 0 )
                instance[i].updated = true;
        }
	}

    void WaitForSim()
    {
        for ( int i = 0; i < NumInstances; ++i )
        {
            instance[i].workerThread.Join();
     
            instance[i].game->GetViewPacket( instance[i].viewPacket );
        }
    }
    	
	void Render( float deltaTime, bool shadows )
	{
		// update the scene to be rendered
		
        for ( int i = 0; i < NumInstances; ++i )
        {
            view::Object * playerCube = instance[i].viewObjectManager.GetObject( 1 );
            if ( playerCube )
                instance[i].origin = playerCube->position + playerCube->positionError;

            math::Vector lookat = instance[i].origin - math::Vector(0,0,1);
            #ifdef WIDESCREEN
            math::Vector position = lookat + math::Vector(0,-11,5);
            #else
            math::Vector position = lookat + math::Vector(0,-12,6);
            #endif

            instance[i].camera.EaseIn( lookat, position ); 

    		if ( instance[i].viewPacket.objectCount >= 1 )
    		{
    			view::ObjectUpdate updates[MaxViewObjects];
    			getViewObjectUpdates( updates, instance[i].viewPacket );
    			instance[i].viewObjectManager.UpdateObjects( updates, instance[i].viewPacket.objectCount );
    		}

    		instance[i].viewObjectManager.ExtrapolateObjects( deltaTime );

    		instance[i].viewObjectManager.Update( deltaTime );
        }

        // prepare for rendering

        renderInterface->ClearScreen();

        int width = renderInterface->GetDisplayWidth();
        int height = renderInterface->GetDisplayHeight();

        // render the local view (left)

        if ( instance[0].updated )
        {
            Cubes cubes;
            instance[0].viewObjectManager.GetRenderState( cubes );
            setCameraAndLight( renderInterface, instance[0].camera );
            renderInterface->BeginScene( 0, 0, width/2, height );
            ActivationArea activationArea;
            setupActivationArea( activationArea, instance[0].origin, 5.0f, instance[0].t );
            renderInterface->RenderActivationArea( activationArea, 1.0f );
            renderInterface->RenderCubes( cubes );
            #ifdef SHADOWS
            if ( shadows )
                renderInterface->RenderCubeShadows( cubes );
            #endif
        }

        // render the remote view (right)

        if ( instance[1].updated )
        {
            Cubes cubes;
            instance[1].viewObjectManager.GetRenderState( cubes );
            setCameraAndLight( renderInterface, instance[1].camera );
            renderInterface->BeginScene( width/2, 0, width, height );
            ActivationArea activationArea;
            setupActivationArea( activationArea, instance[1].origin, 5.0f, instance[0].t );
            renderInterface->RenderActivationArea( activationArea, 1.0f );
            renderInterface->RenderCubes( cubes );
            #ifdef SHADOWS
            if ( shadows )
                renderInterface->RenderCubeShadows( cubes );
            #endif
        }

        // shadow quad on top

        #ifdef SHADOWS
        if ( shadows )
            renderInterface->RenderShadowQuad();
        #endif

        // divide splitscreen

        renderInterface->DivideSplitscreen();
	}
};
