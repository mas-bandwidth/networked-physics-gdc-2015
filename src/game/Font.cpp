/*
    Font Loader
    Copyright (c) 2014, The Network Protocol Company, Inc.
    Derived from public domain code: http://content.gpwiki.org/index.php/OpenGL:Tutorials:Font_System
*/  

#ifdef CLIENT

#include "core/Memory.h"
#include "Font.h"
#include "Global.h"
#include "Render.h"
#include "ShaderManager.h"

// todo: remove this BS
#include <iostream>
#include <fstream>

#include <GLUT/glut.h>
#include <OpenGL/OpenGL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using glm::mat4;
using glm::vec3;
using glm::vec4;

template<class T, class S> void ReadObject( T & to_read, S & in )
{
    in.read( reinterpret_cast<char*>( &to_read ), sizeof(T) );
}

struct GlyphBuffer
{
    unsigned char magic;
    unsigned char ascii; 
    unsigned short width;
    unsigned short x;
    unsigned short y;
};

struct Glyph
{
    float tex_x1, tex_y1, tex_x2;
    int advance;
};

struct FontAtlas
{
    Glyph * glyphs;
    Glyph * table[256];
    uint32_t texture;
    int width;
    int height;
    int line_height;
    float tex_line_height;
};

FontAtlas * LoadFontAtlas( core::Allocator & allocator, const char * filename )
{
    CORE_ASSERT( filename );

    printf( "%.2f: Loading font \"%s\"\n", global.timeBase.time, filename );

    // Open the file and check whether it is any good (a font file starts with "FONT")
    std::ifstream input(filename, std::ios::binary);

    if ( input.fail() )
    {
        printf( "error: failed to load font file \"%s\"\n", filename );
        return nullptr;
    }

    if ( input.get() != 'F' || input.get() != 'O' || input.get() != 'N' || input.get() != 'T' )
    {
        // todo: proper error log and don't exit on error, return an error code
        printf( "error: not a valid font file\n" );
        return nullptr;
    }

    FontAtlas * atlas = CORE_NEW( allocator, FontAtlas );

    int n_chars;

    ReadObject( atlas->width, input );
    ReadObject( atlas->height, input );
    ReadObject( atlas->line_height, input );
    ReadObject( n_chars, input );

    atlas->tex_line_height = static_cast<float>( atlas->line_height ) / atlas->height;

    atlas->glyphs = new Glyph[n_chars];          // todo: use the allocator
    for ( int i = 0; i != 256; ++i )
        atlas->table[i] = nullptr;

    for ( int i = 0; i < n_chars; ++i )
    {
        GlyphBuffer buffer;
        ReadObject( buffer, input );
        if ( buffer.magic != 23 )
        {
            // todo: add a proper log header with time
            printf( "error: glyph %d magic mismatch: expected %d, got %d\n", i, 23, (int) buffer.magic );
            CORE_DELETE( allocator, FontAtlas, atlas );
            return nullptr;
        }

        atlas->glyphs[i].tex_x1 = float( buffer.x ) / atlas->width;
        atlas->glyphs[i].tex_x2 = float( buffer.x + buffer.width ) / atlas->width;
        atlas->glyphs[i].tex_y1 = float( buffer.y ) / atlas->height;
        atlas->glyphs[i].advance = buffer.width;
        atlas->table[buffer.ascii] = atlas->glyphs + i;
    }

    Glyph * default_glyph = atlas->table[ (unsigned char)'\xFF' ];
    if ( !default_glyph )
    {
        // todo: add a proper log header with time
        printf( "error: font file contains no default glyph\n" );
        CORE_DELETE( allocator, FontAtlas, atlas );
        return nullptr;
    }
    
    for ( int i = 0; i != 256; ++i )
    {
        if ( atlas->table[i] == NULL )
            atlas->table[i] = default_glyph;
    }

    unsigned char * tex_data = new unsigned char[atlas->width * atlas->height];           // todo: use scratch allocator
    input.read( reinterpret_cast<char*>(tex_data), atlas->width * atlas->height );

    glGenTextures( 1, &atlas->texture );
    glBindTexture( GL_TEXTURE_2D, atlas->texture );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, atlas->width, atlas->height, 0, GL_RED, GL_UNSIGNED_BYTE, tex_data );

    delete [] tex_data;     // todo: use the allocator

    return atlas;
}

void DestroyFontAtlas( core::Allocator & allocator, FontAtlas * atlas )
{
    CORE_ASSERT( atlas );
    CORE_ASSERT( atlas->texture );
    glDeleteTextures( 1, &atlas->texture );
    delete [] atlas->glyphs; // todo: use allocator
    atlas->glyphs = nullptr;
    atlas->texture = 0;
    CORE_DELETE( allocator, FontAtlas, atlas );
}

const int MaxFontVertices = 4 * 1024;

struct FontVertex
{
    float x,y,z;
    // float r,g,b,a;
    float u,v;
};

struct FontRender
{
    bool active;
    GLuint vbo;
    GLuint vao;
    int currentFontVertex;
    FontVertex vertices[MaxFontVertices];

    FontRender()
    {
        active = false;
        vbo = 0;
        vao = 0;
        currentFontVertex = 0;
    }
};

Font::Font( core::Allocator & allocator, FontAtlas * atlas )
{
    CORE_ASSERT( atlas );
    m_allocator = &allocator;
    m_atlas = atlas;
    m_render = CORE_NEW( allocator, FontRender );
}

Font::~Font()
{
    CORE_ASSERT( m_atlas );
    CORE_ASSERT( m_render );
    CORE_ASSERT( m_allocator );

    DestroyFontAtlas( *m_allocator, m_atlas );

    glDeleteVertexArrays( 1, &m_render->vao );

    CORE_DELETE( *m_allocator, FontRender, m_render );

    m_atlas = nullptr;
    m_render = nullptr;
    m_allocator = nullptr;
}

int Font::GetLineHeight() const
{
    CORE_ASSERT( m_atlas );
    return m_atlas->line_height;
}

int Font::GetCharWidth( char c ) const
{
    CORE_ASSERT( m_atlas );
    return m_atlas->table[(int)c]->advance;
}
 
int Font::GetTextWidth( const char * text ) const
{
    int total = 0;
    while ( *text != '\0' )
        total += GetCharWidth( *text++ );
    return total;
}

void Font::Begin()
{
    CORE_ASSERT( m_atlas );
    CORE_ASSERT( m_atlas->texture );
    CORE_ASSERT( m_render );
    CORE_ASSERT( !m_render->active );
   
    m_render->active = true;

    glBindTexture( GL_TEXTURE_2D, m_atlas->texture );

    GLuint shader_program = global.shaderManager->GetShader( "Font" );
    if ( !shader_program )
        return;

    glUseProgram( shader_program );

    glBindAttribLocation( shader_program, 0, "VertexPosition" );
    glBindAttribLocation( shader_program, 1, "TexCoord" );

    // todo: pass this in per font render call instead, eg. vertex attribute
    vec4 textColor = vec4( 0,0,0,1 );

    mat4 modelViewProjection = glm::ortho( 0.0f, (float) global.displayWidth, (float) global.displayHeight, 0.0f, -1.0f, 1.0f );

    int location = glGetUniformLocation( shader_program, "TextColor" );
    if ( location < 0 )
        return;    
    glUniform4fv( location, 1, &textColor[0] );

    location = glGetUniformLocation( shader_program, "ModelViewProjection" );
    if ( location < 0 )
        return;
    glUniformMatrix4fv( location, 1, GL_FALSE, &modelViewProjection[0][0] );

    if ( m_render->vao == 0 )
    {
        glGenBuffers( 1, &m_render->vbo );
        glGenVertexArrays( 1, &m_render->vao );
        glBindVertexArray( m_render->vao );
        glEnableVertexAttribArray( 0 );
        glEnableVertexAttribArray( 1 );
        glBindBuffer( GL_ARRAY_BUFFER, m_render->vbo );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (GLubyte*)0 );
        // todo: add color data r,g,b,a
        glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (GLubyte*)(3*4) );
    }
    else
    {
        glBindVertexArray( m_render->vao );
        glBindBuffer( GL_ARRAY_BUFFER, m_render->vbo );
    }
}

void Font::DrawAtlas( float x, float y )
{
    CORE_ASSERT( m_render );
    CORE_ASSERT( m_render->active );

    // a b
    // d c

    FontVertex a,b,c,d;

    const float w = m_atlas->width;
    const float h = m_atlas->height;

    a.x = 0;
    a.y = 0;
    a.z = 0;
    a.u = 0;
    a.v = 0;

    b.x = w;
    b.y = 0;
    b.z = 0;
    b.u = 1;
    b.v = 0;

    c.x = w;
    c.y = h;
    c.z = 0;
    c.u = 1;
    c.v = 1;

    d.x = 0;
    d.y = h;
    d.z = 0;
    d.u = 0;
    d.v = 1;

    CORE_ASSERT( m_render->currentFontVertex + 6 < MaxFontVertices );

    FontVertex * vertex = m_render->vertices + m_render->currentFontVertex;

    vertex[0] = a;
    vertex[1] = b;
    vertex[2] = c;
    vertex[3] = a;
    vertex[4] = c;
    vertex[5] = d;

    m_render->currentFontVertex += 6;
}

void Font::DrawText( float x, float y, const char * text )
{
    CORE_ASSERT( m_render );
    CORE_ASSERT( m_render->active );

    const char * p = text;

    while ( *p != '\0' )
    {
        int character = *p++;

        Glyph * glyph = m_atlas->table[character];

        // a b
        // d c

        FontVertex a,b,c,d;

        const float w = m_atlas->width;
        const float h = m_atlas->height;

        a.x = x;
        a.y = y;
        a.z = 0;
        a.u = glyph->tex_x1;
        a.v = glyph->tex_y1;

        b.x = x + glyph->advance;
        b.y = y;
        b.z = 0;
        b.u = glyph->tex_x2;
        b.v = glyph->tex_y1;

        c.x = x + glyph->advance;
        c.y = y + m_atlas->line_height;
        c.z = 0;
        c.u = glyph->tex_x2;
        c.v = glyph->tex_y1 + m_atlas->tex_line_height;

        d.x = x;
        d.y = y + m_atlas->line_height;
        d.z = 0;
        d.u = glyph->tex_x1;
        d.v = glyph->tex_y1 + m_atlas->tex_line_height;

        CORE_ASSERT( m_render->currentFontVertex + 6 < MaxFontVertices );

        FontVertex * vertex = m_render->vertices + m_render->currentFontVertex;

        vertex[0] = a;
        vertex[1] = b;
        vertex[2] = c;
        vertex[3] = a;
        vertex[4] = c;
        vertex[5] = d;

        m_render->currentFontVertex += 6;

        x += glyph->advance;
    }
}

void Font::End()
{
    CORE_ASSERT( m_render );
    CORE_ASSERT( m_render->active );

    glBufferData( GL_ARRAY_BUFFER, sizeof( FontVertex ) * m_render->currentFontVertex, m_render->vertices, GL_STREAM_DRAW );

    glDrawArrays( GL_TRIANGLES, 0, m_render->currentFontVertex );

    m_render->active = false;
    m_render->currentFontVertex = 0;
}

#endif // #ifdef CLIENT

