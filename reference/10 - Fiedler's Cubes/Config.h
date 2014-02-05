/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef CONFIG_H
#define CONFIG_H

// platform detection

#define PLATFORM_WINDOWS  1
#define PLATFORM_MAC      2
#define PLATFORM_UNIX     3
#define PLATFORM_PS3	  4

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#else
#define PLATFORM PLATFORM_UNIX
#endif

#ifndef PLATFORM
#error unknown platform!
#endif

// compile time configuration

#define MULTITHREADED
//#define VISUALIZE_SHADOW_VOLUMES
//#define FRUSTUM_CULLING
//#define USE_SECONDARY_DISPLAY_IF_EXISTS
//#define DISCOVER_KEY_CODES

const int MaxPlayers = 4;

const float MaxLinearVelocity = 100;
const float MaxAngularVelocity = 100;
const float PositionResolution = 0.001f;
const float OrientationResolution = 0.001f;
const float LinearVelocityResolution = 0.001f;
const float AngularVelocityResolution = 0.001f;
const float RelativePositionBound = 10.0f;

#endif
