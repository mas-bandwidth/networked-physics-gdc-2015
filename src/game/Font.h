/*
    Font Loader
    Copyright (c) 2014, The Network Protocol Company, Inc.
    Derived from public domain code: http://content.gpwiki.org/index.php/OpenGL:Tutorials:Font_System
*/  

#ifndef FONT_H
#define FONT_H

#ifdef CLIENT

#include <stdint.h>
 
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

    void DrawAtlas( float x, float y );

    void DrawText( float x, float y, const char * text );

    void End();

private:

    FontAtlas * m_atlas;
    FontRender * m_render;
    core::Allocator * m_allocator;
};

#endif // #ifdef CLIENT

#endif // #ifndef FONT_H

