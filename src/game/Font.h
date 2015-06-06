/*
    Font Loader
    Copyright (c) 2008-2015, Glenn Fiedler
    Derived from public domain code: http://content.gpwiki.org/index.php/OpenGL:Tutorials:Font_System
*/  

#ifndef FONT_H
#define FONT_H

#ifdef CLIENT

#include "Common.h"
#include <stdint.h>
#include <glm/glm.hpp>

namespace core { class Allocator; }

struct FontAtlas;
struct FontRender;

class Font
{
public:

    Font( core::Allocator & allocator, FontAtlas * atlas );

    ~Font();

    int GetLineHeight() const;

    int GetCharWidth( char c ) const;

    int GetTextWidth( const char * text ) const;
    
    void Begin();

    void DrawAtlas( float x, float y, const Color & color = Color(0,0,0,1) );

    void DrawText( float x, float y, const char * text, const Color & color = Color(0,0,0,1) );

    void End();

private:

    FontAtlas * m_atlas;
    FontRender * m_render;
    core::Allocator * m_allocator;
};

#endif // #ifdef CLIENT

#endif // #ifndef FONT_H
