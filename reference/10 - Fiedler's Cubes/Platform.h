/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef PLATFORM_H
#define PLATFORM_H

#include "Config.h"

#include <assert.h>
#include <stdio.h>

#if PLATFORM == PLATFORM_MAC
#include "CoreServices/CoreServices.h"
#include <stdint.h>
#include <unistd.h>
#include <mach/mach_time.h>
#include <pthread.h>
#include <OpenGl/gl.h>
#include <OpenGl/glu.h>
#include <OpenGL/glext.h>
#include <OpenGL/OpenGL.h>
#include <Carbon/Carbon.h>
#endif

#if PLATFORM == PLATFORM_UNIX
#include <time.h>
#include <errno.h>
#include <math.h>
#ifdef TIMER_RDTSC
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#endif
#endif

#if PLATFORM == PLATFORM_WINDOWS
#include <GL/GLee.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/sdl.h>
#undef main
#endif

namespace platform
{
#if PLATFORM == PLATFORM_MAC
	
	// worker thread 

	class WorkerThread
	{
	public:
	
		WorkerThread()
		{
			#ifdef MULTITHREADED
			thread = 0;
			#endif
		}
	
		virtual ~WorkerThread()
		{
			#ifdef MULTITHREADED
			thread = 0;
			#endif
		}
	
		bool Start()
		{
			#ifdef MULTITHREADED

				pthread_attr_t attr;	
				pthread_attr_init( &attr );
				pthread_attr_setstacksize( &attr, 32 * 1024 * 1024 );
				if ( pthread_create( &thread, &attr, StaticRun, (void*)this ) != 0 )
				{
					printf( "error: pthread_create failed\n" );
					return false;
				}
		
			#else
		
				Run();
			
			#endif
		
			return true;
		}
	
		bool Join()
		{
			#ifdef MULTITHREADED
			if ( pthread_join( thread, NULL ) != 0 )
			{
				printf( "error: pthread_join failed\n" );
				return false;
			}
			#endif
			return true;
		}
	
	protected:
	
		static void* StaticRun( void * data )
		{
			WorkerThread * self = (WorkerThread*) data;
			self->Run();
			return NULL;
		}
	
		virtual void Run() = 0;			// note: override this to implement your thread task
	
	private:

		#ifdef MULTITHREADED	
		pthread_t thread;
		#endif
	};

#endif
	
#if PLATFORM == PLATFORM_WINDOWS

	// worker thread 

	class WorkerThread
	{
	public:
	
		WorkerThread()
		{
			#ifdef MULTITHREADED
			thread = NULL;
			#endif
		}
	
		virtual ~WorkerThread()
		{
			#ifdef MULTITHREADED
			thread = NULL;
			#endif
		}
	
		bool Start()
		{
			#ifdef MULTITHREADED

				if ( NULL == (thread = SDL_CreateThread( StaticRun, (void*)this) ) )
				{
					printf( "error: SDL_CreateThread failed\n" );
					return false;
				}
		
			#else
		
				Run();
			
			#endif
		
			return true;
		}
	
		bool Join()
		{
			#ifdef MULTITHREADED
			SDL_WaitThread( thread, NULL );
			#endif
			return true;
		}
	
	protected:
	
		static int StaticRun( void * data )
		{
			WorkerThread * self = (WorkerThread*) data;
			self->Run();
			return 0;
		}
	
		virtual void Run() = 0;			// note: override this to implement your thread task
	
	private:

		#ifdef MULTITHREADED
		SDL_Thread *thread;
		#endif
	};

#endif


	// platform independent wait for n seconds

	#if PLATFORM == PLATFORM_WINDOWS

		inline void wait_seconds( float seconds )
		{
			Sleep( (int) ( seconds * 1000.0f ) );
		}

	#else

		inline void wait_seconds( float seconds ) { usleep( (int) ( seconds * 1000000.0f ) ); }

	#endif

	#if PLATFORM == PLATFORM_MAC

		// high resolution timer (mac)

		class Timer
		{
		public:
	
			Timer()
			{
				reset();
			}
	
			void reset()
			{
				_startTime = mach_absolute_time();
				_deltaTime = _startTime;
			}
	
			float time()
			{
				uint64_t counter = mach_absolute_time();
				float time = subtractTimes( counter, _startTime );
				return time;
			}
	
			float delta()
			{
				uint64_t counter = mach_absolute_time();
				float dt = subtractTimes( counter, _deltaTime );
				_deltaTime = counter;
				return dt;
			}
	
			float resolution()
			{
			    static double conversion = 0.0;
			    if ( conversion == 0.0 )
			    {
			        mach_timebase_info_data_t info;
			        kern_return_t err = mach_timebase_info( &info );
			        if( err == 0  )
						conversion = 1e-9 * (double) info.numer / (double) info.denom;
			    }
				return conversion;
			}
	
		private:
		
			double subtractTimes( uint64_t endTime, uint64_t startTime )
			{
			    uint64_t difference = endTime - startTime;
			    static double conversion = 0.0;
			    if ( conversion == 0.0 )
			    {
			        mach_timebase_info_data_t info;
			        kern_return_t err = mach_timebase_info( &info );
			        if( err == 0  )
						conversion = 1e-9 * (double) info.numer / (double) info.denom;
			    }
			    return conversion * (double) difference;
			}		

			uint64_t _startTime;		// start time (to avoid accumulating a float)
			uint64_t _deltaTime;		// last time delta was called
		};

	#endif

	#if PLATFORM == PLATFORM_UNIX

		namespace internal
		{
			void wait( double seconds )
			{
				const double floorSeconds = ::floor(seconds);
				const double fractionalSeconds = seconds - floorSeconds;
		
				timespec timeOut;
				timeOut.tv_sec = static_cast<time_t>(floorSeconds);
				timeOut.tv_nsec = static_cast<long>(fractionalSeconds * 1e9);
		
				// nanosleep may return earlier than expected if there's a signal
				// that should be handled by the calling thread.  If it happens,
				// sleep again. [Bramz]
				//
				timespec timeRemaining;
				while (true)
				{
					const int ret = nanosleep(&timeOut, &timeRemaining);
					if (ret == -1 && errno == EINTR)
					{
						// there was only an sleep interruption, go back to sleep.
						timeOut.tv_sec = timeRemaining.tv_sec;
						timeOut.tv_nsec = timeRemaining.tv_nsec;
					}
					else
					{
						// we're done, or error =)
						return; 
					}
				}
			}
		}

		#ifdef TIMER_RDTSC

		class Timer
		{
		public:
	
			Timer():
				resolution_(determineResolution())
			{
				reset();
			}
	
			void reset()
			{
				deltaStart_ = start_ = tick();
			}
	
			double time()
			{
				const uint64_t now = tick();
				return resolution_ * (now - start_);
			}
	
			double delta()
			{
				const uint64_t now = tick();
				const double dt = resolution_ * (now - deltaStart_);
				deltaStart_ = now;
				return dt;
			}
	
			double resolution()
			{
				return resolution_;
			}
	
			void wait( double seconds )
			{
				internal::wait(seconds);
			}	
	
		private:

			static inline uint64_t tick()
			{
		#ifdef TIMER_64BIT
				uint32_t a, d;
				__asm__ __volatile__("rdtsc": "=a"(a), "=d"(d));
				return (static_cast<uint64_t>(d) << 32) | static_cast<uint64_t>(a);
		#else
				uint64_t val;
				__asm__ __volatile__("rdtsc": "=A"(val));
				return val;
		#endif
			}
	
			static double determineResolution()
			{
				FILE* f = fopen("/proc/cpuinfo", "r");
				if (!f)
				{
					return 0.;
				}
				const int bufferSize = 256;
				char buffer[bufferSize];
				while (fgets(buffer, bufferSize, f))
				{
					float frequency;
					if (sscanf(buffer, "cpu MHz         : %f", &frequency) == 1)
					{
						fclose(f);
						return 1e-6 / static_cast<double>(frequency);
					}
				}
				fclose(f);
				return 0.;
			}

			uint64_t start_;
			uint64_t deltaStart_;
			double resolution_;
		};

		#else

		class Timer
		{
		public:
	
			Timer()
			{
				reset();
			}
	
			void reset()
			{
				deltaStart_ = start_ = realTime();
			}
	
			double time()
			{
				return realTime() - start_;
			}
	
			double delta()
			{
				const double now = realTime();
				const double dt = now - deltaStart_;
				deltaStart_ = now;
				return dt;
			}
	
			double resolution()
			{
				timespec res;
				if (clock_getres(CLOCK_REALTIME, &res) != 0)
				{
					return 0.;
				}
				return res.tv_sec + res.tv_nsec * 1e-9;
			}
	
			void wait( double seconds )
			{
				internal::wait(seconds);
			}
	
		private:

			static inline double realTime()
			{
				timespec time;
				if (clock_gettime(CLOCK_REALTIME, &time) != 0)
				{
					return 0.;
				}
				return time.tv_sec + time.tv_nsec * 1e-9;		
			}
	
			double start_;
			double deltaStart_;
		};

	#endif

	#endif

	#if PLATFORM == PLATFORM_WINDOWS

		class Timer
		{
		public:
	
			Timer()
			{
				reset();
			}
	
			void reset()
			{
				QueryPerformanceCounter( reinterpret_cast<LARGE_INTEGER*>(&_startTime) );
				_deltaTime = _startTime;
			}
	
			float time()
			{
				uint64_t counter;
				QueryPerformanceCounter( reinterpret_cast<LARGE_INTEGER*>(&counter) );
				float time = subtractTimes( counter, _startTime );
				return time;
			}
	
			float delta()
			{
				uint64_t counter;
				QueryPerformanceCounter( reinterpret_cast<LARGE_INTEGER*>(&counter) );
				float dt = subtractTimes( counter, _deltaTime );
				_deltaTime = counter;
				return dt;
			}
	
			float resolution()
			{
				uint64_t freq;
				QueryPerformanceFrequency( reinterpret_cast<LARGE_INTEGER*>(&freq) );
				return 1.0 / static_cast<double>(freq);
			}
	
		private:
		
			double subtractTimes( uint64_t endTime, uint64_t startTime )
			{
				uint64_t difference = endTime - startTime;
				static double conversion = 0.0;
				if ( conversion == 0.0 )
				{
					uint64_t freq;
					QueryPerformanceFrequency( reinterpret_cast<LARGE_INTEGER*>(&freq) );
					conversion = 1.0 / static_cast<double>(freq);
				}
				return conversion * static_cast<double>(difference);
			}		

			uint64_t _startTime;		// start time (to avoid accumulating a float)
			uint64_t _deltaTime;		// last time delta was called
		};

	#endif


	// basic keyboard input

	static bool spaceKeyDown = false;
	static bool backSlashKeyDown = false;
	static bool enterKeyDown = false;
	static bool delKeyDown = false;
	static bool escapeKeyDown = false;
	static bool tabKeyDown = false;
	static bool pageUpKeyDown = false;
	static bool pageDownKeyDown = false;
	static bool upKeyDown = false;
	static bool downKeyDown = false;
	static bool leftKeyDown = false;
	static bool rightKeyDown = false;
	static bool qKeyDown = false;
	static bool wKeyDown = false;
	static bool eKeyDown = false;
	static bool aKeyDown = false;
	static bool sKeyDown = false;
	static bool dKeyDown = false;
	static bool zKeyDown = false;
	static bool tildeKeyDown = false;
	static bool oneKeyDown = false;
	static bool twoKeyDown = false;
	static bool threeKeyDown = false;
	static bool fourKeyDown = false;
	static bool fiveKeyDown = false;
	static bool sixKeyDown = false;
	static bool sevenKeyDown = false;
	static bool eightKeyDown = false;
	static bool nineKeyDown = false;
	static bool zeroKeyDown = false;
	static bool f1KeyDown = false;
	static bool f2KeyDown = false;
	static bool f3KeyDown = false;
	static bool f4KeyDown = false;
	static bool f5KeyDown = false;
	static bool f6KeyDown = false;
	static bool f7KeyDown = false;
	static bool f8KeyDown = false;
	static bool controlKeyDown = false;
	static bool altKeyDown = false;


	struct Input
	{
		static Input Sample()
		{
			Input input;
		
			input.left = leftKeyDown;
			input.right = rightKeyDown;
			input.up = upKeyDown;
			input.down = downKeyDown;
			input.space = spaceKeyDown;
			input.escape = escapeKeyDown;
			input.tab = tabKeyDown;
			input.backslash = backSlashKeyDown;
			input.enter = enterKeyDown;
			input.del = delKeyDown;
			input.pageUp = pageUpKeyDown;
			input.pageDown = pageDownKeyDown;
			input.q = qKeyDown;
			input.w = wKeyDown;
			input.e = eKeyDown;
			input.a = aKeyDown;
			input.s = sKeyDown;
			input.d = dKeyDown;
			input.z = zKeyDown;
			input.tilde = tildeKeyDown;
			input.one = oneKeyDown;
			input.two = twoKeyDown;
			input.three = threeKeyDown;
			input.four = fourKeyDown;
			input.five = fiveKeyDown;
			input.six = sixKeyDown;
			input.seven = sevenKeyDown;
			input.eight = eightKeyDown;
			input.nine = nineKeyDown;
			input.zero = zeroKeyDown;
			input.f1 = f1KeyDown;
			input.f2 = f2KeyDown;
			input.f3 = f3KeyDown;
			input.f4 = f4KeyDown;
			input.f5 = f5KeyDown;
			input.f6 = f6KeyDown;
			input.f7 = f7KeyDown;
			input.f8 = f8KeyDown;
			input.control = controlKeyDown;
			input.alt = altKeyDown;

			return input;
		}
	
		Input()
		{
			left = false;
			right = false;
			up = false;
			down = false;
			space = false;
			escape = false;
			tab = false;
			backslash = false;
			enter = false;
			del = false;
			pageUp = false;
			pageDown = false;
			q = false;
			w = false;
			e = false;
			a = false;
			s = false;
			d = false;
			z = false;
			tilde = false;
			one = false;
			two = false;
			three = false;
			four = false;
			five = false;
			six = false;
			seven = false;
			eight = false;
			nine = false;
			zero = false;
			f1 = false;
			f2 = false;
			f3 = false;
			f4 = false;
			f5 = false;
			f6 = false;
			f7 = false;
			f8 = false;
			control = false;
			alt = false;
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


	#if PLATFORM == PLATFORM_MAC

	static pascal OSErr quitEventHandler( const AppleEvent *appleEvt, AppleEvent *reply, void * something )
	{
		exit( 0 );
		return false;
	}

	#define QZ_ESCAPE		0x35
	#define QZ_TAB			0x30
	#define QZ_PAGEUP		0x74
	#define QZ_PAGEDOWN		0x79
	#define QZ_RETURN		0x24
	#define QZ_BACKSLASH	0x2A
	#define QZ_DELETE		0x33
	#define QZ_UP			0x7E
	#define QZ_SPACE		0x31
	#define QZ_LEFT			0x7B
	#define QZ_DOWN			0x7D
	#define QZ_RIGHT		0x7C
	#define QZ_Q			0x0C
	#define QZ_W			0x0D
	#define QZ_E			0x0E
	#define QZ_A		    0x00
	#define QZ_S			0x01
	#define QZ_D			0x02
	#define QZ_Z			0x06
	#define QZ_TILDE		0x32
	#define QZ_ONE			0x12
	#define QZ_TWO			0x13
	#define QZ_THREE		0x14
	#define QZ_FOUR			0x15
	#define QZ_FIVE			0x17
	#define QZ_SIX			0x16
	#define QZ_SEVEN		0x1A
	#define QZ_EIGHT		0x1C
	#define QZ_NINE			0x19
	#define QZ_ZERO			0x1D
	#define QZ_F1			0x7A
	#define QZ_F2			0x78
	#define QZ_F3			0x63
	#define QZ_F4			0x76
	#define QZ_F5			0x60
	#define QZ_F6			0x61
	#define QZ_F7			0x62
	#define QZ_F8			0x64

	pascal OSStatus keyboardEventHandler( EventHandlerCallRef nextHandler, EventRef event, void * userData )
	{
		UInt32 eventClass = GetEventClass( event );
		UInt32 eventKind = GetEventKind( event );
	
		if ( eventClass == kEventClassKeyboard )
		{
			char macCharCodes;
			UInt32 macKeyCode;
			UInt32 macKeyModifiers;

			GetEventParameter( event, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(macCharCodes), NULL, &macCharCodes );
			GetEventParameter( event, kEventParamKeyCode, typeUInt32, NULL, sizeof(macKeyCode), NULL, &macKeyCode );
			GetEventParameter( event, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(macKeyModifiers), NULL, &macKeyModifiers );

			controlKeyDown = ( macKeyModifiers & (1<<controlKeyBit) ) != 0 ? true : false;
			altKeyDown = ( macKeyModifiers & (1<<optionKeyBit) ) != 0 ? true : false;
		
			if ( eventKind == kEventRawKeyDown )
			{
				switch ( macKeyCode )
				{
					case QZ_SPACE: spaceKeyDown = true; break;
					case QZ_RETURN: enterKeyDown = true; break;
					case QZ_BACKSLASH: backSlashKeyDown = true; break;
					case QZ_DELETE: delKeyDown = true; break;
					case QZ_ESCAPE: escapeKeyDown = true; break;
					case QZ_TAB: tabKeyDown = true; break;
					case QZ_PAGEUP: pageUpKeyDown = true; break;
					case QZ_PAGEDOWN: pageDownKeyDown = true; break;
					case QZ_UP: upKeyDown = true; break;
					case QZ_DOWN: downKeyDown = true; break;
					case QZ_LEFT: leftKeyDown = true; break;
					case QZ_RIGHT: rightKeyDown = true; break;
					case QZ_Q: qKeyDown = true; break;
					case QZ_W: wKeyDown = true; break;
					case QZ_E: eKeyDown = true; break;
					case QZ_A: aKeyDown = true; break;
					case QZ_S: sKeyDown = true; break;
					case QZ_D: dKeyDown = true; break;
					case QZ_Z: zKeyDown = true; break;
					case QZ_TILDE: tildeKeyDown = true; break;
					case QZ_ONE: oneKeyDown = true; break;
					case QZ_TWO: twoKeyDown = true; break;
					case QZ_THREE: threeKeyDown = true; break;
					case QZ_FOUR: fourKeyDown = true; break;
					case QZ_FIVE: fiveKeyDown = true; break;
					case QZ_SIX: sixKeyDown = true; break;
					case QZ_SEVEN: sevenKeyDown = true; break;
					case QZ_EIGHT: eightKeyDown = true; break;
					case QZ_NINE: nineKeyDown = true; break;
					case QZ_ZERO: zeroKeyDown = true; break;
					case QZ_F1: f1KeyDown = true; break;
					case QZ_F2: f2KeyDown = true; break;
					case QZ_F3: f3KeyDown = true; break;
					case QZ_F4: f4KeyDown = true; break;
					case QZ_F5: f5KeyDown = true; break;
					case QZ_F6: f6KeyDown = true; break;
					case QZ_F7: f7KeyDown = true; break;
					case QZ_F8: f8KeyDown = true; break;
				
					default:
					{
						#ifdef DISCOVER_KEY_CODES
						// note: for "discovering" keycodes for new keys :)
						char title[] = "Message";
						char text[64];
						sprintf( text, "key=%x", (int) macKeyCode );
						Str255 msg_title;
						Str255 msg_text;
						c2pstrcpy( msg_title, title );
						c2pstrcpy( msg_text, text );
						StandardAlert( kAlertStopAlert, (ConstStr255Param) msg_title, (ConstStr255Param) msg_text, NULL, NULL);
						#endif
						return eventNotHandledErr;
					}
				}
			}
			else if ( eventKind == kEventRawKeyUp )
			{
				switch ( macKeyCode )
				{
					case QZ_SPACE: spaceKeyDown = false; break;
					case QZ_BACKSLASH: backSlashKeyDown = false; break;
					case QZ_RETURN: enterKeyDown = false; break;
					case QZ_DELETE: delKeyDown = false; break;
					case QZ_ESCAPE: escapeKeyDown = false; break;
					case QZ_TAB: tabKeyDown = false; break;
					case QZ_PAGEUP: pageUpKeyDown = false; break;
					case QZ_PAGEDOWN: pageDownKeyDown = false; break;
					case QZ_UP: upKeyDown = false; break;
					case QZ_DOWN: downKeyDown = false; break;
					case QZ_LEFT: leftKeyDown = false; break;
					case QZ_RIGHT: rightKeyDown = false; break;
					case QZ_Q: qKeyDown = false; break;
					case QZ_W: wKeyDown = false; break;
					case QZ_E: eKeyDown = false; break;
					case QZ_A: aKeyDown = false; break;
					case QZ_S: sKeyDown = false; break;
					case QZ_D: dKeyDown = false; break;
					case QZ_Z: zKeyDown = false; break;
					case QZ_TILDE: tildeKeyDown = false; break;
					case QZ_ONE: oneKeyDown = false; break;
					case QZ_TWO: twoKeyDown = false; break;
					case QZ_THREE: threeKeyDown = false; break;
					case QZ_FOUR: fourKeyDown = false; break;
					case QZ_FIVE: fiveKeyDown = false; break;
					case QZ_SIX: sixKeyDown = false; break;
					case QZ_SEVEN: sevenKeyDown = false; break;
					case QZ_EIGHT: eightKeyDown = false; break;
					case QZ_NINE: nineKeyDown = false; break;
					case QZ_ZERO: zeroKeyDown = false; break;
					case QZ_F1: f1KeyDown = false; break;
					case QZ_F2: f2KeyDown = false; break;
					case QZ_F3: f3KeyDown = false; break;
					case QZ_F4: f4KeyDown = false; break;
					case QZ_F5: f5KeyDown = false; break;
					case QZ_F6: f6KeyDown = false; break;
					case QZ_F7: f7KeyDown = false; break;
					case QZ_F8: f8KeyDown = false; break;

					default: return eventNotHandledErr;
				}
			}
		}

		return noErr;
	}

	CGLContextObj contextObj;

	void HideMouseCursor()
	{
		CGDisplayHideCursor( kCGNullDirectDisplay );
		CGAssociateMouseAndMouseCursorPosition( false );
		CGDisplayMoveCursorToPoint( kCGDirectMainDisplay, CGPointZero );
	}

	void ShowMouseCursor()
	{
		CGAssociateMouseAndMouseCursorPosition( true );
		CGDisplayShowCursor( kCGNullDirectDisplay );
	}

	bool OpenDisplay( const char title[], int & width, int & height )
	{
		// install keyboard handler
	
		EventTypeSpec eventTypes[2];

		eventTypes[0].eventClass = kEventClassKeyboard;
		eventTypes[0].eventKind  = kEventRawKeyDown;

		eventTypes[1].eventClass = kEventClassKeyboard;
		eventTypes[1].eventKind  = kEventRawKeyUp;

		EventHandlerUPP handlerUPP = NewEventHandlerUPP( keyboardEventHandler );

		InstallApplicationEventHandler( handlerUPP, 2, eventTypes, NULL, NULL );

		// initialize fullscreen CGL
	
		CGDirectDisplayID displayId = kCGDirectMainDisplay;
	
		#ifdef USE_SECONDARY_DISPLAY_IF_EXISTS
		const CGDisplayCount maxDisplays = 2;
		CGDirectDisplayID activeDisplayIds[maxDisplays];
		CGDisplayCount displayCount;
		CGGetActiveDisplayList( 2, activeDisplayIds, &displayCount );
		if ( displayCount == 2 )
			displayId = activeDisplayIds[1];
		#endif
	
	 	width = CGDisplayPixelsWide( displayId );
	 	height = CGDisplayPixelsHigh( displayId );

		CGDisplayCapture( displayId );
		CGLPixelFormatAttribute attribs[] = 
		{ 
			kCGLPFANoRecovery,
			kCGLPFADoubleBuffer,
		    kCGLPFAFullScreen,
			kCGLPFAStencilSize, ( CGLPixelFormatAttribute ) 8,
		    kCGLPFADisplayMask, ( CGLPixelFormatAttribute ) CGDisplayIDToOpenGLDisplayMask( displayId ),
		    ( CGLPixelFormatAttribute ) NULL
		};
	
		CGLPixelFormatObj pixelFormatObj;
		GLint numPixelFormats;
		CGLChoosePixelFormat( attribs, &pixelFormatObj, &numPixelFormats );

		CGLCreateContext( pixelFormatObj, NULL, &contextObj );

		CGLDestroyPixelFormat( pixelFormatObj );

		CGLSetCurrentContext( contextObj );
		CGLSetFullScreenOnDisplay( contextObj, 1 );

		// install quit handler (via apple events)

		AEInstallEventHandler( kCoreEventClass, kAEQuitApplication, NewAEEventHandlerUPP(quitEventHandler), 0, false );

		return true;
	}

	void UpdateDisplay( int interval = 1 )
	{
		// set swap interval
		
		CGLSetParameter( contextObj, kCGLCPSwapInterval, &interval );
		CGLFlushDrawable( contextObj );

		// process events
	
		EventRef event = 0; 
		OSStatus status = ReceiveNextEvent( 0, NULL, 0.0f, kEventRemoveFromQueue, &event ); 
		if ( status == noErr && event )
		{ 
			bool sendEvent = true;

			if ( sendEvent )
				SendEventToEventTarget( event, GetEventDispatcherTarget() ); 
		
			ReleaseEvent( event );
		}
	}

	void CloseDisplay()
	{	
		CGLSetCurrentContext( NULL );
		CGLClearDrawable( contextObj );
		CGLDestroyContext( contextObj );
		CGReleaseAllDisplays();
	}

#endif

#if PLATFORM == PLATFORM_WINDOWS
	void HideMouseCursor()
	{
		SDL_ShowCursor( 0 );
	}

	void ShowMouseCursor()
	{
		SDL_ShowCursor( 1 );
	}

	bool OpenDisplay( const char title[], int & width, int & height )
	{
		SDL_Init( SDL_INIT_EVERYTHING );

		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
		const SDL_VideoInfo* info = SDL_GetVideoInfo();
		width = info->current_w;
		height = info->current_h;
		SDL_SetVideoMode( width, height, info->vfmt->BitsPerPixel, SDL_OPENGL | SDL_FULLSCREEN );
		SDL_WM_SetCaption( title, NULL );

		return true;
	}

	void UpdateDisplay( int interval = 1 )
	{
		SDL_Event event;

		// handle all SDL events that we might've received in this loop iteration
		while (SDL_PollEvent(&event))
		{
			bool keyDown = false;

			switch (event.type)
			{
				case SDL_QUIT:
					SDL_Quit();
					exit( 0 );

				case SDL_KEYDOWN:
					keyDown ^= true;
					// fall-through

				case SDL_KEYUP:
					controlKeyDown = ( event.key.keysym.mod & KMOD_CTRL ) != 0 ? true : false;
					altKeyDown = ( event.key.keysym.mod & KMOD_ALT ) != 0 ? true : false;

					switch(event.key.keysym.sym)
					{
						case SDLK_SPACE: spaceKeyDown = keyDown; break;
						case SDLK_RETURN: enterKeyDown = keyDown; break;
						case SDLK_BACKSLASH: backSlashKeyDown = keyDown; break;
						case SDLK_DELETE: delKeyDown = keyDown; break;
						case SDLK_ESCAPE: escapeKeyDown = keyDown; break;
						case SDLK_TAB: tabKeyDown = keyDown; break;
						case SDLK_PAGEUP: pageUpKeyDown = keyDown; break;
						case SDLK_PAGEDOWN: pageDownKeyDown = keyDown; break;
						case SDLK_UP: upKeyDown = keyDown; break;
						case SDLK_DOWN: downKeyDown = keyDown; break;
						case SDLK_LEFT: leftKeyDown = keyDown; break;
						case SDLK_RIGHT: rightKeyDown = keyDown; break;
						case SDLK_q: qKeyDown = keyDown; break;
						case SDLK_w: wKeyDown = keyDown; break;
						case SDLK_e: eKeyDown = keyDown; break;
						case SDLK_a: aKeyDown = keyDown; break;
						case SDLK_s: sKeyDown = keyDown; break;
						case SDLK_d: dKeyDown = keyDown; break;
						case SDLK_z: zKeyDown = keyDown; break;
						case SDLK_BACKQUOTE: tildeKeyDown = keyDown; break;
						case SDLK_1: oneKeyDown = keyDown; break;
						case SDLK_2: twoKeyDown = keyDown; break;
						case SDLK_3: threeKeyDown = keyDown; break;
						case SDLK_4: fourKeyDown = keyDown; break;
						case SDLK_5: fiveKeyDown = keyDown; break;
						case SDLK_6: sixKeyDown = keyDown; break;
						case SDLK_7: sevenKeyDown = keyDown; break;
						case SDLK_8: eightKeyDown = keyDown; break;
						case SDLK_9: nineKeyDown = keyDown; break;
						case SDLK_0: zeroKeyDown = keyDown; break;
						case SDLK_F1: f1KeyDown = keyDown; break;
						case SDLK_F2: f2KeyDown = keyDown; break;
						case SDLK_F3: f3KeyDown = keyDown; break;
						case SDLK_F4: f4KeyDown = keyDown; break;
						case SDLK_F5: f5KeyDown = keyDown; break;
						case SDLK_F6: f6KeyDown = keyDown; break;
						case SDLK_F7: f7KeyDown = keyDown; break;
						case SDLK_F8: f8KeyDown = keyDown; break;
						default: break;
					}
					break;

				default:
					break;
			}
		}

		wglSwapIntervalEXT( interval );
		SDL_GL_SwapBuffers();
	}

	void CloseDisplay()
	{
		SDL_Quit();
	}
#endif

}

#endif
