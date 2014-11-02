/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef CONFIG_H
#define CONFIG_H

#define LETTERBOX
#define WIDESCREEN
#define SHADOWS

const int MaxPlayers = 2;
const float ColorChangeTightness = 0.1f;
const float ShadowAlphaThreshold = 0.25f;
const float PositionSmoothingTightness = 0.075f;
const float OrientationSmoothingTightness = 0.2f;
const int MaxViewObjects = 1024;
const int MaxShadowVertices = ( 6 * 4 ) * MaxViewObjects;
const int MaxDynamicVertices = 24 * MaxViewObjects;

#endif
