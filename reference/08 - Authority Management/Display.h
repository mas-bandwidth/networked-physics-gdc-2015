/**
	Simple OpenGL Display for Win32 and MacOS X
	Glenn Fiedler <gaffer@gaffer.org>
**/

#ifndef OPENGL_DISPLAY_H
#define OPENGL_DISPLAY_H

#include "Platform.h"

// portable display

#if PLATFORM == PLATFORM_MAC
#include <OpenGl/gl.h>
#include <OpenGl/glu.h>
#include <OpenGL/glext.h>
#elif PLATFORM == PLATFORM_PC
#include <gl/gl.h>
#include <gl/glu.h>
#pragma comment( lib, "opengl32.lib" )
#endif

// win32 display

#if PLATFORM == PLATFORM_PC

HWND window = 0;
HDC device = 0;
HGLRC context = 0;
bool active = true;

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch (uMsg)
    {
        case WM_PAINT:
            ValidateRect(hWnd, NULL);
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALFARPROC)( int );
PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;

bool OpenDisplay( const char title[], int width, int height )
{
    HINSTANCE instance = GetModuleHandle(0);

    WNDCLASSEX windowClass;
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_OWNDC;
    windowClass.lpfnWndProc = &WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = instance;
    windowClass.hIcon = 0;
    windowClass.hCursor = LoadCursor(NULL,IDC_ARROW);
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = title;
    windowClass.hIconSm = NULL;

    UnregisterClass(title, instance);

    if (!RegisterClassEx(&windowClass)) 
        return false;

    // create window

    RECT rect;
	rect.left = 0;
	rect.top = 0;
    rect.right = width;
    rect.bottom = height;
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, 0);
    rect.right -= rect.left;
    rect.bottom -= rect.top;

    int x = (GetSystemMetrics(SM_CXSCREEN) - rect.right) >> 1;
    int y = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) >> 1;

    window = CreateWindow( title, title, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME, 
                           x, y, rect.right, rect.bottom,
                           NULL, NULL, instance, NULL );

    if (!window)
        return false;

    // initialize wgl

    PIXELFORMATDESCRIPTOR descriptor;
    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.nVersion = 1;
    descriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    descriptor.cDepthBits = 32;
    descriptor.iLayerType = PFD_MAIN_PLANE;

    device = GetDC(window);
    if (!device)
        return false;

    GLuint format = ChoosePixelFormat(device, &descriptor);
    if (!format)
        return false;

    if (!SetPixelFormat(device, format, &descriptor))
        return false;

    context = wglCreateContext(device);
    if (!context)
        return false;

    if (!wglMakeCurrent(device, context))
        return false;

	// query swap interval extension

	const char * extensions = (const char*) glGetString( GL_EXTENSIONS );

	if ( strstr( extensions, "WGL_EXT_swap_control" ) != 0 )
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress( "wglSwapIntervalEXT" );

    // show window

    ShowWindow(window, SW_NORMAL);

    return true;
}

void UpdateDisplay( int interval = 0 )		// 0 = no vertical sync, >= 1 is vsync interval
{
	ASSERT( interval >= 0 );

    if ( wglSwapIntervalEXT )
      wglSwapIntervalEXT( interval );

    // process window messages

    MSG message;

    while (true)
    {
        if (active)
        {
            if (!PeekMessage(&message, window, 0, 0, PM_REMOVE)) 
                break;

            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        else
        {
            if (!GetMessage(&message, window, 0, 0)) 
                break;

            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    // show rendering

    SwapBuffers(device);
}

void CloseDisplay()
{	
	ReleaseDC(window, device);
    device = 0;

    DestroyWindow(window);
    window = 0;

	wglSwapIntervalEXT = 0;
}

#elif PLATFORM == PLATFORM_MAC

// apple display

#include <Carbon/Carbon.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/glext.h>
#include <AGL/agl.h>

CFDictionaryRef gOldDisplayMode;
bool gOldDisplayModeValid = false;

static AGLContext setupAGL( WindowRef window, int width, int height )
{	
	if ( (Ptr) kUnresolvedCFragSymbolAddress == (Ptr) aglChoosePixelFormat )
		return 0;
	
	GLint attributes[] = { AGL_RGBA, 
		                   AGL_DOUBLEBUFFER, 
		                   AGL_DEPTH_SIZE, 32, 
						   AGL_NO_RECOVERY,
		                   AGL_NONE,
	                   	   AGL_NONE };	

	AGLPixelFormat format = NULL;

	format = aglChoosePixelFormat( NULL, 0, attributes );

	if ( !format ) 
		return 0;

	AGLContext context = aglCreateContext( format, 0 );

	if ( !context )
		return 0;

	aglDestroyPixelFormat( format );

	aglSetWindowRef( context, window );
	
	aglSetCurrentContext( context );

	return context;
}

static void cleanupAGL( AGLContext context )
{
	int display = 0;
	
	if ( gOldDisplayModeValid )
	{
    	CGDisplaySwitchToMode( display, gOldDisplayMode );
		gOldDisplayModeValid = false;
	}

	aglDestroyContext( context );
}

WindowRef window;
AGLContext context;

static pascal OSErr quitEventHandler( const AppleEvent *appleEvt, AppleEvent *reply, SInt32 refcon )
{
	exit( 0 );
	return false;
}

#define QZ_ESCAPE		0x35
#define QZ_PAGEUP		0x74
#define QZ_PAGEDOWN		0x79
#define QZ_RETURN		0x24
#define QZ_UP			0x7E
#define QZ_SPACE		0x31
#define QZ_LEFT			0x7B
#define QZ_DOWN			0x7D
#define QZ_RIGHT		0x7C
#define QZ_W			0x0D
#define QZ_A		    0x00
#define QZ_S			0x01
#define QZ_D			0x02
#define QZ_TAB			0x30
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

static bool spaceKeyDown = false;
static bool enterKeyDown = false;
static bool escapeKeyDown = false;
static bool pageUpKeyDown = false;
static bool pageDownKeyDown = false;
static bool upKeyDown = false;
static bool downKeyDown = false;
static bool leftKeyDown = false;
static bool rightKeyDown = false;
static bool wKeyDown = false;
static bool aKeyDown = false;
static bool sKeyDown = false;
static bool dKeyDown = false;
static bool tabKeyDown = false;
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

		if ( eventKind == kEventRawKeyDown )
		{
			switch ( macKeyCode )
			{
				case QZ_SPACE: spaceKeyDown = true; break;
				case QZ_RETURN: enterKeyDown = true; break;
				case QZ_ESCAPE: escapeKeyDown = true; break;
				case QZ_PAGEUP: pageUpKeyDown = true; break;
				case QZ_PAGEDOWN: pageDownKeyDown = true; break;
				case QZ_UP: upKeyDown = true; break;
				case QZ_DOWN: downKeyDown = true; break;
				case QZ_LEFT: leftKeyDown = true; break;
				case QZ_RIGHT: rightKeyDown = true; break;
				case QZ_W: wKeyDown = true; break;
				case QZ_A: aKeyDown = true; break;
				case QZ_S: sKeyDown = true; break;
				case QZ_D: dKeyDown = true; break;
				case QZ_TAB: tabKeyDown = true; break;
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
				
				default:
				{
					/*
					char title[] = "Message";
					char text[64];
					sprintf( text, "key=%x", (int) macKeyCode );
					Str255 msg_title;
					Str255 msg_text;
					c2pstrcpy( msg_title, title );
					c2pstrcpy( msg_text, text );
					StandardAlert( kAlertStopAlert, (ConstStr255Param) msg_title, (ConstStr255Param) msg_text, NULL, NULL);
					*/
					return eventNotHandledErr;
				}
			}
		}
		else if ( eventKind == kEventRawKeyUp )
		{
			switch ( macKeyCode )
			{
				case QZ_SPACE: spaceKeyDown = false; break;
				case QZ_RETURN: enterKeyDown = false; break;
				case QZ_ESCAPE: escapeKeyDown = false; break;
				case QZ_PAGEUP: pageUpKeyDown = false; break;
				case QZ_PAGEDOWN: pageDownKeyDown = false; break;
				case QZ_UP: upKeyDown = false; break;
				case QZ_DOWN: downKeyDown = false; break;
				case QZ_LEFT: leftKeyDown = false; break;
				case QZ_RIGHT: rightKeyDown = false; break;
				case QZ_W: wKeyDown = false; break;
				case QZ_A: aKeyDown = false; break;
				case QZ_S: sKeyDown = false; break;
				case QZ_D: dKeyDown = false; break;
				case QZ_TAB: tabKeyDown = false; break;
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

				default: return eventNotHandledErr;
			}
		}
	}

	return noErr;
}

bool OpenDisplay( const char title[], int width, int height )
{
	Rect rect;
	rect.left = 240;			// todo: center window
	rect.top = 110;
	rect.right = rect.left + width;
	rect.bottom = rect.top + height;
	
	OSErr result = CreateNewWindow( kDocumentWindowClass, 
					                (kWindowStandardDocumentAttributes | 
									 kWindowStandardHandlerAttribute) &
									~kWindowResizableAttribute,
					                &rect, &window );
	
	if ( result != noErr )
		return false;

	SetWindowTitleWithCFString( window, CFStringCreateWithCString( 0, title, CFStringGetSystemEncoding() ) );

	context = setupAGL( window, width, height );

	if ( !context )
		return false;
	
	ShowWindow( window );
	
	SelectWindow( window );

	// install standard event handlers
	
    InstallStandardEventHandler( GetWindowEventTarget( window ) );

	// install keyboard handler
	
	EventTypeSpec eventTypes[2];

	eventTypes[0].eventClass = kEventClassKeyboard;
	eventTypes[0].eventKind  = kEventRawKeyDown;

	eventTypes[1].eventClass = kEventClassKeyboard;
	eventTypes[1].eventKind  = kEventRawKeyUp;

	EventHandlerUPP handlerUPP = NewEventHandlerUPP( keyboardEventHandler );

	InstallApplicationEventHandler( handlerUPP, 2, eventTypes, NULL, NULL );

	// install quit handler (via apple events)

	AEInstallEventHandler( kCoreEventClass, kAEQuitApplication, NewAEEventHandlerUPP(quitEventHandler), 0, false );

	return true;
}

void UpdateDisplay( int interval = 0 )
{
	// update display
	
	GLint swapInterval = interval;
	aglSetInteger( context, AGL_SWAP_INTERVAL, &swapInterval );
	aglSwapBuffers( context );

	// process events
	
	EventRef event = 0; 
	OSStatus status = ReceiveNextEvent( 0, NULL, 0.0f, kEventRemoveFromQueue, &event ); 
	if ( status == noErr && event )
	{ 
		bool sendEvent = true;

		// note: required for menu bar to work properly
		if ( GetEventClass( event ) == kEventClassMouse && GetEventKind( event ) == kEventMouseDown )
		{
			WindowRef window;
			Point location;
			GetEventParameter( event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &location );
			if ( MacFindWindow( location, &window ) == inMenuBar )
			{
				sendEvent = false;
				MenuSelect( location );
			}
		}

		if ( sendEvent )
			SendEventToEventTarget( event, GetEventDispatcherTarget() ); 
		
		ReleaseEvent( event );
	}
}

void CloseDisplay()
{	
    cleanupAGL( context );
	context = 0;
	
	DisposeWindow( (WindowPtr) window );
	window = 0;
}

#endif

// high resolution timer (win32)

#if PLATFORM == PLATFORM_PC

class Timer
{
public:
	
	Timer()
	{
		QueryPerformanceFrequency( (LARGE_INTEGER*) &_frequency );
		reset();
	}
	
	void reset()
	{
		QueryPerformanceCounter( (LARGE_INTEGER*) &_timeCounter );
		_deltaCounter = _timeCounter;
		_time = 0.0;
	}
	
	float time()
	{
		__int64 counter;
		QueryPerformanceCounter( (LARGE_INTEGER*) &counter );
		_time += ( counter - _timeCounter ) / (double) _frequency;
		_timeCounter = counter;
		return (float) _time;
	}
	
	float delta()
	{
		__int64 counter;
		QueryPerformanceCounter( (LARGE_INTEGER*) &counter );
		double delta = (counter - _deltaCounter) / (double) _frequency;
		_deltaCounter = counter;
		return (float) delta;
	}
	
	float resolution()
	{
		return (float) ( 1.0 / (double) _frequency );
	}
									  
	void wait( float seconds )
	{
		__int64 start;
		QueryPerformanceCounter( (LARGE_INTEGER*) &start );
		__int64 finish = start + (__int64) ( seconds * _frequency );
		while ( true )
		{
			__int64 current;
			QueryPerformanceCounter( (LARGE_INTEGER*) &current );
			if ( current >= finish )
				break;						// note: this doesnt handle wrap
		}
	}
	
private:

	double _time;               ///< current time in seconds
	__int64 _timeCounter;       ///< raw 64bit timer counter for time
	__int64 _deltaCounter;      ///< raw 64bit timer counter for delta
	__int64 _frequency;         ///< raw 64bit timer frequency
};

#endif

// high resolution timer (mac)

#if PLATFORM == PLATFORM_MAC

#include "CoreServices/CoreServices.h"

class Timer
{
public:
	
	Timer()
	{
		reset();
	}
	
	void reset()
	{
		Microseconds( (UnsignedWide*) &_timeCounter );
		_deltaCounter = _timeCounter;
		_time = 0;
	}
	
	float time()
	{
		UInt64 counter;
		Microseconds( (UnsignedWide*) &counter );
		UInt64 delta = counter - _timeCounter;			// todo: handle wrap around
		_timeCounter = counter;
		_time += delta / 1000000.0;
		return (float) _time;
	}
	
	float delta()
	{
		UInt64 counter;
		Microseconds( (UnsignedWide*) &counter );
		UInt64 delta = counter - _deltaCounter;
		_deltaCounter = counter;
		return (float) ( delta / 1000000.0 );
	}
	
	float resolution()
	{
		return 1.0f / 1000000.0f;
	}
	
	void wait( double seconds )
	{
		// note: doesnt handle wrap around 64bit
		UInt64 start;
		Microseconds( (UnsignedWide*) &start );
		UInt64 finish = start + UInt64( 1000000.0f * seconds );
		while ( true )
		{
			UInt64 current;
			Microseconds( (UnsignedWide*) &current );
			if ( current >= finish )
				break;
		}
	}
	
private:
	
	double _time;               ///< current time in seconds
	UInt64 _timeCounter;        ///< time counter in microseconds
	UInt64 _deltaCounter;		///< delta counter in microseconds
};

#endif

// basic keyboard input

struct Input
{
	static Input Sample()
	{
		Input input;
		
		#if PLATFORM == PLATFORM_PC

			#define VK_ONE 49
			#define VK_TWO 50
			#define VK_THREE 51
			#define VK_FOUR 52
			#define VK_FIVE 53
			#define VK_SIX 54
			#define VK_SEVEN 55
			#define VK_EIGHT 56
			#define VK_NINE 57
			#define VK_ZERO 58

			input.shift = GetAsyncKeyState( VK_LSHIFT ) || GetAsyncKeyState( VK_RSHIFT );
			input.left = GetAsyncKeyState( VK_LEFT ) != 0;
			input.right = GetAsyncKeyState( VK_RIGHT ) != 0;
			input.up = GetAsyncKeyState( VK_UP ) != 0;
			input.down = GetAsyncKeyState( VK_DOWN ) != 0;
			input.space = GetAsyncKeyState( VK_SPACE ) != 0;
			input.escape = GetAsyncKeyState( VK_ESCAPE ) != 0;
			input.enter = GetAsyncKeyState( VK_RETURN ) != 0;

			input.pageUp = GetAsyncKeyState( VK_PRIOR ) != 0;
			input.pageDown = GetAsyncKeyState( VK_NEXT ) != 0;
			input.home = GetAsyncKeyState( VK_HOME ) != 0;
			input.end = GetAsyncKeyState( VK_END ) != 0;
            input.ins = GetAsyncKeyState( VK_INSERT ) != 0;
            input.del = GetAsyncKeyState( VK_DELETE ) != 0;

			input.one = GetAsyncKeyState( VK_ONE ) != 0;
			input.two = GetAsyncKeyState( VK_TWO ) != 0;
			input.three = GetAsyncKeyState( VK_THREE ) != 0;
			input.four = GetAsyncKeyState( VK_FOUR ) != 0;
			input.five = GetAsyncKeyState( VK_FIVE ) != 0;
			input.six = GetAsyncKeyState( VK_SIX ) != 0;
			input.seven = GetAsyncKeyState( VK_SEVEN ) != 0;
			input.eight = GetAsyncKeyState( VK_EIGHT ) != 0;
			input.nine = GetAsyncKeyState( VK_NINE ) != 0;
			input.zero = GetAsyncKeyState( VK_ZERO ) != 0;
		
		#elif PLATFORM == PLATFORM_MAC

			#ifndef DONT_NEED_DISPLAY
			input.shift = false;
			input.left = leftKeyDown;
			input.right = rightKeyDown;
			input.up = upKeyDown;
			input.down = downKeyDown;
			input.space = spaceKeyDown;
			input.escape = escapeKeyDown;
			input.enter = enterKeyDown;
			input.pageUp = pageUpKeyDown;
			input.pageDown = pageDownKeyDown;
			input.w = wKeyDown;
			input.a = aKeyDown;
			input.s = sKeyDown;
			input.d = dKeyDown;
			input.tab = tabKeyDown;
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
			input.home = false;
			input.end = false;
            input.ins = false;
            input.del = false;
			#endif

		#endif

		return input;
	}
	
	Input()
	{
		shift = false;
		left = false;
		right = false;
		up = false;
		down = false;
		space = false;
		escape = false;
		enter = false;
		pageUp = false;
		pageDown = false;
		home = false;
		w = false;
		a = false;
		s = false;
		d = false;
		tab = false;
		end = false;
        ins = false;
        del = false;
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
	}

	bool shift;
	bool left;
	bool right;
	bool up;
	bool down;
	bool space;
	bool escape;
	bool enter;
	bool pageUp;
	bool pageDown;
	bool home;
	bool w;
	bool a;
	bool s;
	bool d;
	bool tab;
	bool end;
    bool ins;
    bool del;
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
};

#endif
