/*
    Font Runtime
    Copyright (c) 2014, The Network Protocol Company, Inc.
    Derived from public domain code: http://content.gpwiki.org/index.php/OpenGL:Tutorials:Font_System
*/  

#include "Fonts.h"

#include <iostream>
#include <fstream>

#include <GLUT/glut.h>
#include <OpenGL/OpenGL.h>

// Helper function to read a piece of data from a stream.
template<class T, class S>
void Read_Object(T& to_read, S& in)
{
    in.read(reinterpret_cast<char*>(&to_read), sizeof(T));
}
 
// This is how glyphs are stored in the file.
struct Glyph_Buffer
{
    unsigned char magic;
    unsigned char ascii; 
    unsigned short width;
    unsigned short x;
    unsigned short y;
};

Font::Font( const char * filename )
    : m_texture(0), m_line_height(0), m_tex_line_height(0)
{
    // Open the file and check whether it is any good (a font file starts with "FONT")
    std::ifstream input(filename, std::ios::binary);
    if ( input.fail() || input.get() != 'F' || input.get() != 'O' || input.get() != 'N' || input.get() != 'T' )
    {
        printf( "error: not a valid font file\n" );
        exit( 1 );
    }

    // Get the texture size, the number of glyphs and the line height.
    int width, height, n_chars;
    Read_Object( width, input );
    Read_Object( height, input );
    Read_Object( m_line_height, input );
    Read_Object( n_chars, input );
    m_tex_line_height = static_cast<float>( m_line_height ) / height;

    /*
    printf( "width = %d\n", width );
    printf( "height = %d\n", height );
    printf( "m_line_height = %d\n", (int) m_line_height );
    printf( "n_chars = %d\n", (int) n_chars );
    printf( "m_tex_line_height = %f\n", m_tex_line_height );
    */

    // Make the glyph table.
    m_glyphs = new Glyph[n_chars];
    for ( int i = 0; i != 256; ++i )
        m_table[i] = NULL;

    // Read every glyph, store it in the glyph array and set the right
    // pointer in the table.
    Glyph_Buffer buffer;
    for ( int i = 0; i < n_chars; ++i )
    {
        Read_Object( buffer, input );

        if ( buffer.magic != 23 )
        {
            printf( "error: glyph %d magic mismatch: expected %d, got %d\n", i, 23, (int) buffer.magic );
            exit(1);
        }

//        printf( "found glyph: %c\n", buffer.ascii );

        m_glyphs[i].tex_x1 = static_cast<float>(buffer.x) / width;
        m_glyphs[i].tex_x2 = static_cast<float>(buffer.x + buffer.width) / width;
        m_glyphs[i].tex_y1 = static_cast<float>(buffer.y) / height;
        m_glyphs[i].advance = buffer.width;

        m_table[buffer.ascii] = m_glyphs + i;
    }

    // All chars that do not have their own glyph are set to point to
    // the default glyph.
    Glyph * default_glyph = m_table[ (unsigned char)'\xFF' ];

    // We must have the default character (stored under '\xFF')
    if ( default_glyph == NULL )
    {
        printf( "error: font file contains no default glyph\n" );
        exit( 1 );
    }
    
    for ( int i = 0; i != 256; ++i )
    {
        if ( m_table[i] == NULL )
            m_table[i] = default_glyph;
    }

    // Store the actual texture in an array.
    unsigned char* tex_data = new unsigned char[width * height];
    input.read( reinterpret_cast<char*>(tex_data), width * height );

    // Generate an alpha texture with it.
    glGenTextures( 1, &m_texture );
    glBindTexture( GL_TEXTURE_2D, m_texture );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA8, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, tex_data );
    glGenerateMipmap( GL_TEXTURE_2D );

    // And delete the texture memory block
    delete[] tex_data;
}

Font::~Font()
{
    // Release texture object and glyph array.
    glDeleteTextures( 1, &m_texture );
    delete [] m_glyphs;
}

int Font::GetLineHeight() const
{
    return m_line_height;
}

int Font::GetCharWidth( unsigned char c ) const
{
    return m_table[c]->advance;
}
 
int Font::GetStringWidth( const char * str ) const
{
    int total = 0;
    while ( *str != '\0' )
        total += GetCharWidth( *str++ );
    return total;
}

void Font::DrawString( float x, float y, const char * str )
{
    glBindTexture( GL_TEXTURE_2D, m_texture );
 
//    glScalef( 0.5f, 0.5f, 0.5f );

    glBegin( GL_QUADS );
    
    const char * p = str;

    while ( *p != '\0' )
    {
        int c = *p++;

        Glyph * glyph = m_table[c];

        glTexCoord2f( glyph->tex_x1, glyph->tex_y1 + m_tex_line_height );
        glVertex2f( x, y );

        glTexCoord2f( glyph->tex_x1, glyph->tex_y1 );
        glVertex2f( x, y + m_line_height );
        
        glTexCoord2f( glyph->tex_x2, glyph->tex_y1 );
        glVertex2f( x + glyph->advance, y + m_line_height );
        
        glTexCoord2f( glyph->tex_x2, glyph->tex_y1 + m_tex_line_height );
        glVertex2f( x + glyph->advance, y );

        x += glyph->advance;
    }

    glEnd();

//    glScalef( 1, 1, 1 );
}
