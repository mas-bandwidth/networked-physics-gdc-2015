#ifndef GAME_COMMON_H
#define GAME_COMMON_H

struct Color
{
    float r,g,b,a;
    Color( float _r, float _g, float _b, float _a = 1.0f ) : r(_r), g(_g), b(_b), a(_a) {}
};

#endif // #ifndef GAME_COMMON_H