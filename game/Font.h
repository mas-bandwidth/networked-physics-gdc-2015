/*
    Font Loader
    Copyright (c) 2014, The Network Protocol Company, Inc.
    Derived from public domain code: http://content.gpwiki.org/index.php/OpenGL:Tutorials:Font_System
*/  

#ifndef FONT_H
#define FONT_H

#ifdef CLIENT
 
class Font
{
public:

    Font();
    ~Font();

    bool Load( const char * filename );

    int GetLineHeight() const;

    int GetCharWidth( unsigned char c ) const;

    int GetStringWidth( const char * str ) const;
    
    void DrawString( float x, float y, const char * str );

private:

    // Information about a glyph. Tex_y2 can be calculated from tex_y1
    // and _tex_line_height (see below). Advance is the width of the
    // glyph in screen space.
    struct Glyph
    {
        float tex_x1, tex_y1, tex_x2;
        int advance;
    };

    // An array to store the glyphs.
    Glyph * m_glyphs;

    // A table to quickly get the glyph belonging to a character.
    Glyph* m_table[256];

    // The line height, in screen space and in texture space, and the
    // OpenGL id of the font texture.
    unsigned int m_texture;
    int m_line_height;
    float m_tex_line_height;
};

#endif

#endif
