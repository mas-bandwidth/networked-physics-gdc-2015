/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef PLATFORM_H
#define PLATFORM_H

// platform detection

#define PLATFORM_WINDOWS  1
#define PLATFORM_MAC      2
#define PLATFORM_LINUX    3
#define PLATFORM_UNIX     4
#define PLATFORM_PS3	  5

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#elif defined(linux)
#define PLATFORM PLATFORM_LINUX
#else
#define PLATFORM PLATFORM_UNIX
#endif

#if PLATFORM == PLATFORM_WINDOWS || PLATFORM == PLATFORM_MAC
#define HAS_OPENGL
#endif

#ifndef PLATFORM
#error unknown platform!
#endif

#if PLATFORM == PLATFORM_MAC
#include <pthread.h>
#endif

#if PLATFORM == PLATFORM_LINUX
#include <stdint.h>
#endif

namespace platform
{
	// display stuff

	void HideMouseCursor();
	void ShowMouseCursor();
	void GetDisplayResolution( int & width, int & height );
	bool OpenDisplay( const char title[], int width, int height, int refresh = 60 );
	void UpdateDisplay( int interval = 1 );
	void CloseDisplay();

	// basic keyboard input

	struct Input
	{
		static Input Sample();
	
		Input()
		{
			memset( this, 0, sizeof( Input ) );
		}

		bool left;
		bool right;
		bool up;
		bool down;
		bool space;
		bool escape;
		bool tab;
		bool backslash;
		bool enter;
		bool del;
		bool pageUp;
		bool pageDown;
		bool q;
		bool w;
		bool e;
		bool a;
		bool s;
		bool d;
		bool z;
		bool tilde;
		bool one;
		bool two;
		bool three;
		bool four;
		bool five;
		bool six;
		bool seven;
		bool eight;
		bool nine;
		bool zero;
		bool f1;
		bool f2;
		bool f3;
		bool f4;
		bool f5;
		bool f6;
		bool f7;
		bool f8;
		bool control;
		bool alt;
	};

	// timing related stuff

	void wait_seconds( float seconds );

	class Timer
	{
	public:

		Timer();

		void reset();
		float time();
		float delta();
		float resolution();

	private:
	
		uint64_t _startTime;		// start time (to avoid accumulating a float)
		uint64_t _deltaTime;		// last time delta was called
	};
	
	// worker thread 

	class WorkerThread
	{
	public:
	
		WorkerThread();
		virtual ~WorkerThread();
			
		bool Start();
		bool Join();
	
	protected:
	
		static void * StaticRun( void * data );
	
		virtual void Run() = 0;			// note: override this to implement your thread task
	
	private:

		pthread_t thread;
	};
}

#endif
