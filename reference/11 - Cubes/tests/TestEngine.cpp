/*****************************************************************************
 * Networking Library by Glenn Fiedler <gaffer@gaffer.org>
 * Copyright (c) Sony Computer Entertainment America 2008-2010
 ****************************************************************************/

#include "PreCompiled.h"
#include "shared/Activation.h"
#include "shared/Engine.h"
#include "shared/Game.h"
#include "shared/Cubes.h"

using namespace activation;
using namespace engine;
using namespace game;

SUITE( Activation )
{
	TEST( test_activation_initial_conditions )
	{
		const float activation_radius = 10.0f;
		const int grid_width = 20;
		const int grid_height = 20;
		const int cell_size = 1;

		activation::ActivationSystem activationSystem( 1024, activation_radius, grid_width, grid_height, cell_size, 32, 32 );
		
		CHECK( activationSystem.GetEventCount() == 0 );
		CHECK( activationSystem.GetX() == 0.0f );
		CHECK( activationSystem.GetY() == 0.0f );
		CHECK( activationSystem.GetActiveCount() == 0 );
		
		activationSystem.Validate();
		
		CHECK( activationSystem.GetWidth() == grid_width );
		CHECK( activationSystem.GetHeight() == grid_height );
		CHECK( activationSystem.GetCellSize() == cell_size );

		CHECK( activationSystem.IsEnabled() == true );
	}
		
	TEST( test_activation_activate_deactivate )
	{
		// first we setup the activation system and add some objects to it
		
		const float activation_radius = 10.0f;
		const int grid_width = 100;
		const int grid_height = 100;
		const int cell_size = 1;

		activation::ActivationSystem activationSystem( 1024, activation_radius, grid_width, grid_height, cell_size, 32, 32 );
		int id = 1;
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float( 0.0f, 1.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float( 0.0f, 1.0f ) );

		// verify that objects activate and we receive activation events 
		// for each object in the order we expect

		for ( int i = 0; i < 10; ++i )
			activationSystem.Update( 0.1f );
		CHECK( activationSystem.GetActiveCount() == 40 );
		CHECK( activationSystem.GetEventCount() == 40 );
		const int eventCount = activationSystem.GetEventCount();
		for ( int i = 0; i < eventCount; ++i )
		{
			const activation::Event & event = activationSystem.GetEvent(i);
			CHECK( event.type == activation::Event::Activate );
			CHECK( event.id == (activation::ObjectId) ( i + 1 ) );
		}
		activationSystem.ClearEvents();
		activationSystem.Validate();
		for ( int i = 1; i <= 40; ++i )
			CHECK( activationSystem.IsActive(i) );
	
		// now if we move the activation point really far away,
		// the objects should deactivate...
		{
			CHECK( activationSystem.GetEventCount() == 0 );
			activationSystem.MoveActivationPoint( 1000.0f, 1000.0f );
			for ( int i = 0; i < 10; ++i )
				activationSystem.Update( 0.1f );
			CHECK( activationSystem.GetActiveCount() == 0 );
			CHECK( activationSystem.GetEventCount() == 40 );
			const int eventCount = activationSystem.GetEventCount();
			for ( int i = 0; i < eventCount; ++i )
			{
				const activation::Event & event = activationSystem.GetEvent(i);
				CHECK( event.type == activation::Event::Deactivate );
				CHECK( event.id >= 1 );
				CHECK( event.id <= 40 );
			}
			activationSystem.ClearEvents();
			activationSystem.Validate();
			for ( int i = 1; i <= 40; ++i )
				CHECK( !activationSystem.IsActive(i) );
		}
		
		// when we move back to the origin we expect the objects to reactivate
		{
			activationSystem.MoveActivationPoint( 0, 0 );
			for ( int i = 0; i < 10; ++i )
				activationSystem.Update( 0.1f );
			CHECK( activationSystem.GetActiveCount() == 40 );
			CHECK( activationSystem.GetEventCount() == 40 );
			const int eventCount = activationSystem.GetEventCount();
			for ( int i = 0; i < eventCount; ++i )
			{
				const activation::Event & event = activationSystem.GetEvent(i);
				CHECK( event.type == activation::Event::Activate );
				CHECK( event.id >= 1 );
				CHECK( event.id <= 40 );
			}
			activationSystem.ClearEvents();
			activationSystem.Validate();
			for ( int i = 1; i <= 40; ++i )
				CHECK( activationSystem.IsActive(i) );
		}

		// move objects from one grid cell to another and verify they stay active
	
		CHECK( activationSystem.IsActive( 1 ) );
		activationSystem.MoveObject( 1, 0.5f, -0.5f );
		activationSystem.Validate();
		CHECK( activationSystem.IsActive( 1 ) );
		CHECK( activationSystem.GetActiveCount() == 40 );
		CHECK( activationSystem.GetEventCount() == 0 );

		CHECK( activationSystem.IsActive( 11 ) );
		activationSystem.MoveObject( 11, -0.5f, -0.5f );
		activationSystem.Validate();
		CHECK( activationSystem.IsActive( 11 ) );
		CHECK( activationSystem.GetActiveCount() == 40 );
		CHECK( activationSystem.GetEventCount() == 0 );
		
		// move an active object out of the activation circle and verify it deactivates

		CHECK( activationSystem.IsActive( 1 ) );
		activationSystem.MoveObject( 1, -15.0f, -15.0f );
		for ( int i = 0; i < 10; ++i )
		{
			activationSystem.Validate();
			activationSystem.Update( 0.1f );
		}
		CHECK( !activationSystem.IsActive( 1 ) );
		CHECK( activationSystem.GetActiveCount() == 39 );
		CHECK( activationSystem.GetEventCount() == 1 );
		if ( activationSystem.GetEventCount() == 1 )
		{
			const activation::Event & event = activationSystem.GetEvent(0);
			CHECK( event.type == activation::Event::Deactivate );
			CHECK( event.id == 1 );
		}
		activationSystem.ClearEvents();
		
		// move the deactivated object into the activation circle and verify it reactivates

		CHECK( !activationSystem.IsActive( 1 ) );
		activationSystem.MoveObject( 1, 0.0f, 0.0f );
		for ( int i = 0; i < 10; ++i )
		{
			activationSystem.Validate();
			activationSystem.Update( 0.1f );
		}
		// NETHACK: deactivated object does not seem to have activated here
		CHECK( activationSystem.IsActive( 1 ) );
		CHECK( activationSystem.GetActiveCount() == 40 );
		CHECK( activationSystem.GetEventCount() == 1 );
		if ( activationSystem.GetEventCount() == 1 )
		{
			const activation::Event & event = activationSystem.GetEvent(0);
			CHECK( event.type == activation::Event::Activate );
			CHECK( event.id == 1 );
		}
		activationSystem.ClearEvents();
		activationSystem.Validate();
	}
	
	TEST( test_activation_enable_disable )
	{
		// first we setup the activation system and add some objects to it
		
		const float activation_radius = 10.0f;
		const int grid_width = 20;
		const int grid_height = 20;
		const int cell_size = 1;

		activation::ActivationSystem activationSystem( 1024, activation_radius, grid_width, grid_height, cell_size, 32, 32 );
		int id = 1;
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float( 0.0f, 1.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float( 0.0f, 1.0f ) );

		// now we disable the activation system, pump updates and verify nothing activates...

		activationSystem.SetEnabled( false );
		for ( int i = 0; i < 10; ++i )
			activationSystem.Update( 0.1f );
		CHECK( activationSystem.GetActiveCount() == 0 );
		CHECK( activationSystem.GetEventCount() == 0 );
		
		// enable the activation system, pump updates, objects should now activate...

		activationSystem.SetEnabled( true );
		for ( int i = 0; i < 10; ++i )
			activationSystem.Update( 0.1f );
		CHECK( activationSystem.GetActiveCount() == 40 );
		CHECK( activationSystem.GetEventCount() == 40 );
		const int eventCount = activationSystem.GetEventCount();
		for ( int i = 0; i < eventCount; ++i )
		{
			const activation::Event & event = activationSystem.GetEvent(i);
			CHECK( event.type == activation::Event::Activate );
			CHECK( event.id == (activation::ObjectId) ( i + 1 ) );
		}
		activationSystem.ClearEvents();
		activationSystem.Validate();
		for ( int i = 1; i <= 40; ++i )
			CHECK( activationSystem.IsActive(i) );
			
		// disable the activation system, pump updates, all objects should deactivate
		{
			activationSystem.SetEnabled( false );
			for ( int i = 0; i < 10; ++i )
				activationSystem.Update( 0.1f );
			CHECK( activationSystem.GetActiveCount() == 0 );
			CHECK( activationSystem.GetEventCount() == 40 );
			const int eventCount = activationSystem.GetEventCount();
			for ( int i = 0; i < eventCount; ++i )
			{
				const activation::Event & event = activationSystem.GetEvent(i);
				CHECK( event.type == activation::Event::Deactivate );
				CHECK( event.id >= 1 );
				CHECK( event.id <= 40 );
			}
			activationSystem.ClearEvents();
			activationSystem.Validate();
			for ( int i = 1; i <= 40; ++i )
				CHECK( !activationSystem.IsActive(i) );
		}
	}

	TEST( test_activation_sweep )
	{
		// first we setup the activation system and add some objects to it
		
		const float activation_radius = 10.0f;
		const int grid_width = 50;
		const int grid_height = 50;
		const int cell_size = 1;

		activation::ActivationSystem activationSystem( 1024, activation_radius, grid_width, grid_height, cell_size, 32, 32 );
		int id = 1;
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float( 0.0f, 1.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float( 0.0f, 1.0f ) );

		// now we do a slow sweep from left to right and verify objects activate and deactivate *once* only
		
		bool activated[40];
		memset( activated, 0, sizeof(activated) );
		for ( float x = -100.0f; x < +100.0f; x += 0.1f )
		{
			activationSystem.MoveActivationPoint( x, 0.0f );
			activationSystem.Update( 0.1f );
			const int eventCount = activationSystem.GetEventCount();
			for ( int i = 0; i < eventCount; ++i )
			{
				const activation::Event & event = activationSystem.GetEvent(i);
				CHECK( event.id >= 1 );
				CHECK( event.id <= 40 );
				if ( event.type == activation::Event::Activate )
				{
					CHECK( !activated[event.id-1] );
					activated[event.id-1] = true;
				}
				else if ( event.type == activation::Event::Deactivate )
				{
					CHECK( activated[event.id-1] );
					activated[event.id-1] = false;
				}
			}
			activationSystem.ClearEvents();
		}
		
		// now we expect all objects to be inactive, and to have no pending events
		
		for ( int i = 0; i < 40; ++i )
			CHECK( activated[i] == false );
			
		CHECK( activationSystem.GetEventCount() == 0 );
		CHECK( activationSystem.GetActiveCount() == 0 );
	}

	TEST( test_activation_stress_test )
	{
		// first we setup the activation system and add some objects to it
		
		const float activation_radius = 10.0f;
		const int grid_width = 40;
		const int grid_height = 40;			// note: this asserts out if we reduce to 20x20 (need to switch to fixed point...)
		const int cell_size = 1;

		activation::ActivationSystem activationSystem( 1024, activation_radius, grid_width, grid_height, cell_size, 32, 32 );
		int id = 1;
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float( 0.0f, 1.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float( 0.0f, 1.0f ) );

		// randomly move objects and the activation point in an attempt to break the system...
	
		for ( int i = 0; i < 100; ++i )
		{
			activationSystem.Update( 0.1f );

			if ( math::chance( 0.1 ) )
				activationSystem.MoveActivationPoint( math::random_float( -20.0f, +20.0f ), math::random_float( -20.0f, +20.0f ) );

			int numMoves = math::random( 20 );
			for ( int i = 0; i < numMoves; ++i )
			{
				int id = 1 + math::random( 40 );
				assert( id >=  1 );
				assert( id <= 40 );
				activationSystem.MoveObject( id, math::random_float( -20.0f, +20.0f ), math::random_float( -20.0f, +20.0f ) );
			}

			activationSystem.Validate();
			
			activationSystem.ClearEvents();
		}

		// disable the activation system, pump updates, all objects should deactivate

		activationSystem.SetEnabled( false );
		for ( int i = 0; i < 10; ++i )
			activationSystem.Update( 0.1f );
		CHECK( activationSystem.GetActiveCount() == 0 );
		const int eventCount = activationSystem.GetEventCount();
		for ( int i = 0; i < eventCount; ++i )
		{
			const activation::Event & event = activationSystem.GetEvent(i);
			CHECK( event.type == activation::Event::Deactivate );
			CHECK( event.id >= 1 );
			CHECK( event.id <= 40 );
		}
		activationSystem.ClearEvents();
		activationSystem.Validate();
		for ( int i = 1; i <= 40; ++i )
			CHECK( !activationSystem.IsActive(i) );
	}
}

SUITE( ResponseQueue )
{
	struct Response
	{
		ObjectId id;
		Response() { id = 0; }
		Response( ObjectId id ) { this->id = id; }
	};
	
	TEST( response_queue_initial_conditions )
	{
		ResponseQueue<Response> responseQueue;

		Response response;
		CHECK( !responseQueue.PopResponse( response ) );

		for ( int i = 0; i < 100; ++i )
			CHECK( !responseQueue.AlreadyQueued( i ) );
	}
	
	TEST( response_queue_pop )
	{
		ResponseQueue<Response> responseQueue;

 		Response a = 10;
 		Response b = 15;
 		Response c = 6;

		responseQueue.QueueResponse( a );
		responseQueue.QueueResponse( b );
		responseQueue.QueueResponse( c );

		Response response;
		CHECK( responseQueue.PopResponse( response ) );
		CHECK( response.id == a.id );

		CHECK( responseQueue.PopResponse( response ) );
		CHECK( response.id == b.id );

		CHECK( responseQueue.PopResponse( response ) );
		CHECK( response.id == c.id );
	}

	TEST( response_queue_clear )
	{
		ResponseQueue<Response> responseQueue;

 		Response a = 10;
 		Response b = 15;
 		Response c = 6;

		responseQueue.QueueResponse( a );
		responseQueue.QueueResponse( b );
		responseQueue.QueueResponse( c );

		responseQueue.Clear();

		Response response;
		CHECK( !responseQueue.PopResponse( response ) );

		for ( int i = 0; i < 100; ++i )
			CHECK( !responseQueue.AlreadyQueued( i ) );
	}
}

SUITE( Compression )
{
	TEST( compress_position )
	{
		math::Vector input(10,100,200.5f);
		math::Vector output(0,0,0);
		uint64_t compressed;
		game::CompressPosition( input, compressed );
		game::DecompressPosition( compressed, output );
		CHECK_CLOSE( input.x, output.x, 0.001f );
		CHECK_CLOSE( input.y, output.y, 0.001f );
		CHECK_CLOSE( input.z, output.z, 0.001f );
	}

	TEST( compress_orientation )
	{
		math::Quaternion input(1,0,0,0);
		math::Quaternion output(0,0,0,0);
		uint32_t compressed;
		game::CompressOrientation( input, compressed );
		game::DecompressOrientation( compressed, output );
		CHECK_CLOSE( input.w, output.w, 0.001f );
		CHECK_CLOSE( input.x, output.x, 0.001f );
		CHECK_CLOSE( input.y, output.y, 0.001f );
		CHECK_CLOSE( input.z, output.z, 0.001f );
	}
}

SUITE( Game )
{
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
	
	TEST( game_initial_conditions )
	{
		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance;
		CHECK( instance.GetLocalPlayer() == -1 );
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			CHECK( !instance.IsPlayerJoined(i) );
			game::Input input;
			instance.GetPlayerInput( i, input );
			CHECK( input == game::Input() );
			CHECK( instance.GetPlayerFocus( i ) == 0 );
		}
		CHECK( instance.GetLocalPlayer() == -1 );
		CHECK( !instance.InGame() );
	}
	
	TEST( game_initialize_shutdown )
	{
		game::Config config;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance( config );

		for ( int i = 0; i < 2; ++i )
		{
			instance.InitializeBegin();
			instance.InitializeEnd();
			instance.Shutdown();
		}
	}
	
	// NETHACK: todo - need to extend this test such that it handles multiple player join and leave
	// and verifies that player cubes activate/deactivate on join/leave (new behavior)
	
	TEST( game_player_join_and_leave )
	{
		game::Config config;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance( config );
		
		instance.InitializeBegin();
		AddCube( &instance, 1.0f, math::Vector(0,0,0) );
		AddCube( &instance, 1.0f, math::Vector(0,0,0) );
		AddCube( &instance, 1.0f, math::Vector(0,0,0) );
		AddCube( &instance, 1.0f, math::Vector(0,0,0) );
		instance.InitializeEnd();
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			CHECK( !instance.IsPlayerJoined( i ) );
			instance.OnPlayerJoined( i );
			instance.SetPlayerFocus( i, i + 1 );
			CHECK( instance.IsPlayerJoined( i ) );
			CHECK( instance.GetPlayerFocus( i ) == ObjectId( i + 1 ) );
		}

		CHECK( !instance.InGame() );
		CHECK( instance.GetLocalPlayer() == -1 );
		instance.SetLocalPlayer( 1 );
		CHECK( instance.GetLocalPlayer() == 1 );
		CHECK( instance.InGame() );		
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			CHECK( instance.IsPlayerJoined( i ) );
			instance.OnPlayerLeft( i );
			CHECK( !instance.IsPlayerJoined( i ) );
		}
		
		instance.Shutdown();

		CHECK( instance.GetLocalPlayer() == -1 );
		CHECK( !instance.InGame() );

		for ( int i = 0; i < MaxPlayers; ++i )
		{
			CHECK( !instance.IsPlayerJoined( i ) );
			CHECK( instance.GetPlayerFocus( i ) == 0 );
		}
	}
	
	TEST( game_object_activation )
	{
		game::Config config;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance( config );
		
		instance.InitializeBegin();
		AddCube( &instance, 1.0f, math::Vector(0,0,0) );
		instance.InitializeEnd();

		instance.SetFlag( game::FLAG_Pause );

		instance.OnPlayerJoined( 0 );
		instance.SetPlayerFocus( 0, 1 );
		instance.SetLocalPlayer( 0 );

		instance.Update();
		
		int numActiveObjects = 0;
		cubes::ActiveObject activeObjects[1];
		instance.CopyActiveObjects( activeObjects, numActiveObjects );
		CHECK( numActiveObjects == 1 );
		CHECK( instance.IsObjectActive( 1 ) );

		instance.OnPlayerLeft( 0 );

		instance.Update();
		
		instance.CopyActiveObjects( activeObjects, numActiveObjects );
		CHECK( numActiveObjects == 0 );
		CHECK( !instance.IsObjectActive( 1 ) );
	}

	TEST( game_object_get_set_state )
	{
		game::Config config;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance( config );
		
		instance.InitializeBegin();
		for ( int i = 0; i < 20; ++i )
		{
			cubes::DatabaseObject object;
			object.position = math::Vector(0,0,0);
			object.orientation = math::Quaternion(1,0,0,0);
			object.scale = ( i == 0 ) ? 1.4f : 0.4f;
			object.linearVelocity = math::Vector(0,0,0);
			object.angularVelocity = math::Vector(0,0,0);
			object.enabled = 1;			
			object.activated = 0;
			instance.AddObject( object, 0, 0 );
		}
		
		instance.InitializeEnd();

		instance.SetFlag( game::FLAG_Pause );

		// join player focused on object 1 to activate some objects

		instance.OnPlayerJoined( 0 );
		instance.SetLocalPlayer( 0 );
		instance.SetPlayerFocus( 0, 1 );

		instance.Update();

		int numActiveObjects = 0;
		cubes::ActiveObject activeObjects[32];
		instance.CopyActiveObjects( activeObjects, numActiveObjects );
		CHECK( numActiveObjects > 0 );

		// move active objects outside activation distance (except player object)
		
		for ( int i = 0; i < numActiveObjects; ++i )
		{
			if ( activeObjects[i].scale < 1.0f )
			{
				activeObjects[i].position.x = 20.0;
				activeObjects[i].position.y = 20.0;
				instance.SetObjectState( activeObjects[i].id, activeObjects[i] );
			}
		}

		// update and verify all non-player objects have deactivated
		
		instance.Update();

		int after_numActiveObjects = 0;
		cubes::ActiveObject after_activeObjects[32];
		instance.CopyActiveObjects( after_activeObjects, after_numActiveObjects );
		CHECK( after_numActiveObjects == 1 );
		CHECK( after_activeObjects[0].scale > 1.0f );
			
		// move the objects back in to the origin point

		const math::Vector origin = instance.GetOrigin();
		for ( int i = 0; i < numActiveObjects; ++i )
		{
			activeObjects[i].position.x = origin.x;
			activeObjects[i].position.y = origin.y;
			instance.SetObjectState( activeObjects[i].id, activeObjects[i] );
		}
		
		// verify they activate again

		instance.Update();
		
		int newActiveCount = instance.GetNumActiveObjects();
		CHECK( newActiveCount == numActiveObjects );
		for ( int i = 0; i < numActiveObjects; ++i )
		{
			CHECK( instance.IsObjectActive( activeObjects[i].id ) );
		}		
	}

	TEST( game_object_persistence )
	{
		// make sure objects remember their state when they re-activate
		
		game::Config config;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance( config );
		
		instance.InitializeBegin();
		for ( int i = 0; i < 20; ++i )
		{
			cubes::DatabaseObject object;
			object.position = math::Vector( math::random_float( -10.0f, +10.0f ), math::random_float( -10.0f, +10.0f ), 5 );
			object.orientation = math::Quaternion(1,0,0,0);
			object.scale = ( i == 0 ) ? 1.4f : 0.4f;
			object.linearVelocity = math::Vector(0,0,0);
			object.angularVelocity = math::Vector(0,0,0);
			object.enabled = 1;
			object.activated = 0;
			instance.AddObject( object, object.position.x, object.position.y );
		}
		instance.InitializeEnd();

		instance.SetFlag( game::FLAG_Pause );

		// join and activate some objects

		instance.OnPlayerJoined( 0 );
		instance.SetLocalPlayer( 0 );
		instance.SetPlayerFocus( 0, 1 );

		for ( int i = 0; i < 10; ++i )
			instance.Update();

		int numActiveObjects = 0;
		cubes::ActiveObject activeObjects[32];
		instance.CopyActiveObjects( activeObjects, numActiveObjects );
		CHECK( numActiveObjects > 0 );

		// verify player cube is active

		int focusObjectId = instance.GetPlayerFocus( 0 );
		CHECK( focusObjectId >= 0 );
		CHECK( instance.IsObjectActive( focusObjectId ) );
		
		// change active object positions and orientations
		
		math::Vector origin = instance.GetOrigin();

		for ( int i = 0; i < numActiveObjects; ++i )
		{
			CHECK( activeObjects[i].id >= 0 );
			if ( activeObjects[i].scale > 1.0f )
			{
				activeObjects[i].position.x = origin.x + ( activeObjects[i].position.x - origin.x ) * 0.5f;
				activeObjects[i].position.y = origin.y + ( activeObjects[i].position.y - origin.y ) * 0.5f;
				activeObjects[i].position.z = 0.0f;
				activeObjects[i].orientation = math::Quaternion( 0.5f, activeObjects[i].position.x, activeObjects[i].position.y, activeObjects[i].position.x + activeObjects[i].position.y );
				activeObjects[i].orientation.normalize();
				instance.SetObjectState( activeObjects[i].id, activeObjects[i] );
			}
		}

		// verify all objects are still active
		
		for ( int i = 0; i < 5; ++i )
			instance.Update();		
		
		CHECK( instance.GetNumActiveObjects() == numActiveObjects );

		// move player cube away, deactivate objects

		instance.OnPlayerLeft( 0 );

		for ( int i = 0; i < 5; ++i )
			instance.Update();

		CHECK( instance.GetNumActiveObjects() == 0 );

		// rejoin player

		instance.OnPlayerJoined( 0 );
		instance.SetLocalPlayer( 0 );

		for ( int i = 0; i < 5; ++i )
			instance.Update();

		// verify original objects are active and remember their positions and orientations

		int after_numActiveObjects = 0;
		cubes::ActiveObject after_activeObjects[32];
		instance.CopyActiveObjects( after_activeObjects, after_numActiveObjects );

		CHECK( after_numActiveObjects == numActiveObjects );

		for ( int i = 0; i < after_numActiveObjects; ++i )
		{
			bool found = false;
			for ( int j = 0; j < numActiveObjects; ++j )
			{
				if ( after_activeObjects[i].id == activeObjects[j].id )
				{
					found = true;
					
					CHECK_CLOSE( after_activeObjects[i].position.x, activeObjects[j].position.x, 0.001f );
					CHECK_CLOSE( after_activeObjects[i].position.y, activeObjects[j].position.y, 0.001f );
					CHECK_CLOSE( after_activeObjects[i].position.z, activeObjects[j].position.z, 0.001f );
					
					const float cosine = after_activeObjects[i].orientation.w * activeObjects[j].orientation.w +
										 after_activeObjects[i].orientation.x * activeObjects[j].orientation.x +
										 after_activeObjects[i].orientation.y * activeObjects[j].orientation.y +
										 after_activeObjects[i].orientation.z * activeObjects[j].orientation.z;

					if ( cosine < 0 )
						after_activeObjects[i].orientation *= -1;
					
					CHECK_CLOSE( after_activeObjects[i].orientation.w, activeObjects[j].orientation.w, 0.03f );
					CHECK_CLOSE( after_activeObjects[i].orientation.x, activeObjects[j].orientation.x, 0.03f );
					CHECK_CLOSE( after_activeObjects[i].orientation.y, activeObjects[j].orientation.y, 0.03f );
					CHECK_CLOSE( after_activeObjects[i].orientation.z, activeObjects[j].orientation.z, 0.03f );
				}
			}
			CHECK( found );
		}
	}
}
