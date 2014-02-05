/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#include "PreCompiled.h"
#include "network/netSockets.h"
#include "demos/SingleplayerDemo.h"
#include "demos/DeterministicLockstepDemo.h"
#include "demos/InterpolationExtrapolationDemo.h"

#if PLATFORM == PLATFORM_MAC
#include <OpenGl/gl.h>
#include <OpenGl/glu.h>
#include <OpenGL/glext.h>
#include <OpenGL/OpenGL.h>
#endif

render::Interface * renderInterface = NULL;

Demo * CreateDemo( int index )
{
	if ( index == 0 )
		return new SingleplayerDemo();

    if ( index == 1 )
        return new DeterministicLockstepDemo( DeterministicLockstepDemo::Deterministic );

    if ( index == 2 )
        return new DeterministicLockstepDemo( DeterministicLockstepDemo::NonDeterministic );

    if ( index == 3 )
        return new InterpolationExtrapolationDemo();

	return NULL;
}

bool WriteTGA( const char filename[], int width, int height, uint8_t * ptr )
{
    FILE * file = fopen( filename, "wb" );
    if ( !file )
        return false;

    putc( 0, file );
    putc( 0, file );
    putc( 10, file );                        /* compressed RGB */
    putc( 0, file ); putc( 0, file );
    putc( 0, file ); putc( 0, file );
    putc( 0, file );
    putc( 0, file ); putc( 0, file );           /* X origin */
    putc( 0, file ); putc( 0, file );           /* y origin */
    putc( ( width & 0x00FF ),file );
    putc( ( width & 0xFF00 ) >> 8,file );
    putc( ( height & 0x00FF ), file );
    putc( ( height & 0xFF00 ) >> 8, file );
    putc( 24, file );                         /* 24 bit bitmap */
    putc( 0, file );

    for ( int y = 0; y < height; ++y )
    {
        uint8_t * line = ptr + width * 3 * y;
        uint8_t * end_of_line = line + width * 3;
        uint8_t * pixel = line;
        while ( true )
        {
            if ( pixel >= end_of_line )
                break;

            uint8_t * start = pixel;
            uint8_t * finish = pixel + 128 * 3;
            if ( finish > end_of_line )
                finish = end_of_line;
            uint32_t previous = ( pixel[0] << 16 ) | ( pixel[1] << 8 ) | pixel[2];
            pixel += 3;
            int counter = 1;

            // RLE packet
            while ( pixel < finish )
            {
                NET_ASSERT( pixel < end_of_line );
                uint32_t current = ( pixel[0] << 16 ) | ( pixel[1] << 8 ) | pixel[2];
                if ( current != previous )
                    break;
                previous = current;
                pixel += 3;
                counter++;
            }
            if ( counter > 1 )
            {
                NET_ASSERT( counter <= 128 );
                putc( uint8_t( counter - 1 ) | 128, file );
                putc( start[0], file );
                putc( start[1], file );
                putc( start[2], file );
                continue;
            }

            // RAW packet
            while ( pixel < finish )
            {
                NET_ASSERT( pixel < end_of_line );
                uint32_t current = ( pixel[0] << 16 ) | ( pixel[1] << 8 ) | pixel[2];
                if ( current == previous )
                    break;
                previous = current;
                pixel += 3;
                counter++;
            }
            NET_ASSERT( counter >= 1 );
            NET_ASSERT( counter <= 128 );
            putc( uint8_t( counter - 1 ), file );
            fwrite( start, counter * 3, 1, file );
        }
    }

    fclose( file );

    return true;
}

//#define PROFILE

int main( int argc, char * argv[] )
{	
	printf( "networked physics demo\n" );

    bool shadows = true;
    bool playback = false;
    bool video = false;

    for ( int i = 1; i < argc; ++i )
    {
        if ( strcmp( argv[i], "playback" ) == 0 )
        {
            printf( "playback\n" );
            playback = true;
        }
        else if ( strcmp( argv[i], "video" ) == 0 )
        {
            printf( "video\n" );
            video = true;
        }
    }

	net::InitializeSockets();
	while ( !net::IsInitialized() )
	{
		printf( "error: failed to initialize sockets\n" );
		net::ShutdownSockets();
		return 1;
	}

#ifndef PROFILE
	
	int displayWidth, displayHeight;
	GetDisplayResolution( displayWidth, displayHeight );

    #ifdef LETTERBOX
    displayWidth = 1280;
    displayHeight = 800;
    #endif

	printf( "display resolution is %d x %d\n", displayWidth, displayHeight );

	HideMouseCursor();
	
	if ( !OpenDisplay( "Networked Physics", displayWidth, displayHeight ) )
	{
		printf( "error: failed to open display" );
		return 1;
	}
	
#endif

	int currentDemo = 0;
	Demo * demo = CreateDemo( 0 );
	assert( demo );
	demo->InitializeWorld();
    renderInterface = new render::Interface( displayWidth, displayHeight );	
    #ifndef PROFILE
	demo->SetRenderInterface( renderInterface );
    #endif

	uint32_t frame = 0;

    // create 2 pixel buffer objects, you need to delete them when program exits.
    // glBufferDataARB with NULL pointer reserves only memory space.
    const int NumPBOs = 2;
    GLuint pboIds[NumPBOs];
    int index = 0;
    const int dataSize = displayWidth * displayHeight * 3;
    if ( video )
    {
        glGenBuffersARB( NumPBOs, pboIds );
        for ( int i = 0; i < NumPBOs; ++i )
        {
            glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[i] );
            glBufferDataARB( GL_PIXEL_UNPACK_BUFFER_ARB, dataSize, 0, GL_STREAM_DRAW_ARB );
        }
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );
    }

    // record input to a file
    // read it back in playback mode for recording video
    FILE * inputFile = fopen( "output/recordedInputs", playback ? "rb" : "wb" );
    if ( !inputFile )
    {
        printf( "failed to open input file\n" );
        return 1;
    }

    bool quit = false;
	while ( !quit )
	{
		#ifdef PROFILE
		printf( "profiling frame %d\n", frame );
		#endif
		
		platform::Input input;
        
        if ( !playback )
        {
            input = platform::Input::Sample();
            fwrite( &input, sizeof( platform::Input ), 1, inputFile );
            fflush( inputFile );
        }
        else
        {
            const int size = sizeof( platform::Input );
            if ( !fread( &input, size, 1, inputFile ) )
                quit = true;
        }

		#ifdef PROFILE
		if ( frame > 500 )
			input.left = frame % 2;
		else if ( frame > 100 && ( frame % 5 ) == 0 )
			input.left  = true;
		input.z = true;
		#endif
		
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

			static bool enterDownLastFrame = false;
			if ( input.enter && !enterDownLastFrame )
				shadows = !shadows;
			enterDownLastFrame = input.enter;
				
			if ( demoIndex != -1 )
			{
				Demo * newDemo = CreateDemo( demoIndex );
				if ( newDemo )
				{
					#ifndef PROFILE
                    renderInterface->ClearScreen();
                    #ifdef LETTERBOX
                    renderInterface->LetterBox( 80 );
                    #endif
					UpdateDisplay( 1 );
					#endif
                    
					delete demo;
					demo = newDemo;
					assert( demo );
					demo->InitializeWorld();
					#ifndef PROFILE
					demo->SetRenderInterface( renderInterface );
					#endif
					currentDemo = demoIndex;
				}
			}
		}
		
		static bool escapeDownLastFrame = false;		
		if ( input.escape && !escapeDownLastFrame )
		{
            #ifndef PROFILE
            renderInterface->ClearScreen();
            #ifdef LETTERBOX
            renderInterface->LetterBox( 80 );
            #endif
            UpdateDisplay( 1 );
            #endif

			delete demo;
			demo = CreateDemo( currentDemo );
			assert( demo );
			demo->InitializeWorld();
			#ifndef PROFILE
			demo->SetRenderInterface( renderInterface );
			#endif
		}
		escapeDownLastFrame = input.escape;
		
		demo->ProcessInput( !input.alt ? input : platform::Input() );

		demo->Update( DeltaTime );

        if ( video )
        {
            // "index" is used to read pixels from framebuffer to a PBO
            // "nextIndex" is used to update pixels in the other PBO
            index = ( index + 1 ) % NumPBOs;
            int prevIndex = ( index + NumPBOs - 1 ) % NumPBOs;

            // set the target framebuffer to read
            glReadBuffer( GL_FRONT );

            // read pixels from framebuffer to PBO
            // glReadPixels() should return immediately.
            glBindBufferARB( GL_PIXEL_PACK_BUFFER_ARB, pboIds[index] );
            glReadPixels( 0, 0, displayWidth, displayHeight, GL_BGR, GL_UNSIGNED_BYTE, 0 );
            if ( frame > (unsigned) NumPBOs )
            {
                // map the PBO to process its data by CPU
                glBindBufferARB( GL_PIXEL_PACK_BUFFER_ARB, pboIds[prevIndex] );
                GLubyte * ptr = (GLubyte*) glMapBufferARB( GL_PIXEL_PACK_BUFFER_ARB,
                                                           GL_READ_ONLY_ARB );
                if ( ptr )
                {
                    char filename[256];
                    sprintf( filename, "output/frame-%05d.tga", frame - NumPBOs );
                    #ifdef LETTERBOX
                    WriteTGA( filename, displayWidth, displayHeight - 80, ptr + displayWidth * 3 * 40 );
                    #else
                    WriteTGA( filename, displayWidth, displayHeight, ptr );
                    #endif
                    glUnmapBufferARB( GL_PIXEL_PACK_BUFFER_ARB );
                }
            }

            // back to conventional pixel operation
            glBindBufferARB( GL_PIXEL_PACK_BUFFER_ARB, 0 );
        }

        demo->WaitForSim();
		
		#ifndef PROFILE

    		demo->Render( DeltaTime, shadows );

            #ifdef LETTERBOX
            renderInterface->LetterBox( 80 );
            #endif

            UpdateDisplay( video ? 0 : 1 );
        
		#endif

		frame ++;
	}

    #ifndef PROFILE
	CloseDisplay();
    #endif

	delete demo;
    delete renderInterface;

	printf( "shutdown\n" );
	
	net::ShutdownSockets();

	return 0;
}
