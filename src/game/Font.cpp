/*
    Font Loader
    Copyright (c) 2008-2015, Glenn Fiedler
*/  

#ifdef CLIENT

#include "core/Memory.h"
#include "Font.h"
#include "Global.h"
#include "Render.h"
#include "ShaderManager.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdio.h>
#include <stdlib.h>

using glm::mat4;
using glm::vec3;
using glm::vec4;

template <class T> bool ReadObject( FILE * file, const T & object )
{
    return fread( (char*) &object, sizeof(object), 1, file ) == 1;
}

struct FontGlyphBuffer
{
    unsigned char ascii; 
    unsigned short width;
    unsigned short x;
    unsigned short y;
};

struct FontGlyph
{
    float tex_x1;
    float tex_y1;
    float tex_x2;
    int advance;
};

struct FontAtlas
{
     FontAtlas()
    {
        glyphs = nullptr;
        num_glyphs = 0;
        texture = 0;
        width = 0;
        height = 0;
        line_height = 0;
        tex_line_height = 0;
    }

    FontGlyph * glyphs;
    FontGlyph * table[256];
    uint32_t texture;
    int num_glyphs;
    int width;
    int height;
    int line_height;
    float tex_line_height;
};

void DestroyFontAtlas( core::Allocator & allocator, FontAtlas * atlas )
{
    CORE_ASSERT( atlas );
    if ( atlas->texture != 0 )
        glDeleteTextures( 1, &atlas->texture );
    CORE_DELETE_ARRAY( allocator, atlas->glyphs, atlas->num_glyphs );
    atlas->glyphs = nullptr;
    atlas->texture = 0;
    CORE_DELETE( allocator, FontAtlas, atlas );
}

FontAtlas * LoadFontAtlas( core::Allocator & allocator, const char * filename )
{
    CORE_ASSERT( filename );

    printf( "%.3f: Loading font \"%s\"\n", global.timeBase.time, filename );

    FILE * file = fopen( filename, "rb" );
    if ( !file )
    {
        printf( "%.3f: error: failed to load font file \"%s\"\n", global.timeBase.time, filename );
        return nullptr;
    }

    char header[4];
    if ( fread( header, 4, 1, file ) != 1 || header[0] != 'F' || header[1] != 'O' || header[2] != 'N' || header[3] != 'T' )
    {
        printf( "%.3f: error: not a valid font file\n", global.timeBase.time );
        fclose( file );
        return nullptr;
    }

    FontAtlas * atlas = CORE_NEW( allocator, FontAtlas );

    ReadObject( file, atlas->width );
    ReadObject( file, atlas->height );
    ReadObject( file, atlas->line_height );
    ReadObject( file, atlas->num_glyphs );

    if ( ferror( file ) || feof( file ) )
    {
        printf( "%.3f: error: failed to read font info\n", global.timeBase.time );
        DestroyFontAtlas( allocator, atlas );
        fclose( file );
        return nullptr;
    }

    atlas->tex_line_height = static_cast<float>( atlas->line_height ) / atlas->height;

    atlas->glyphs = CORE_NEW_ARRAY( allocator, FontGlyph, atlas->num_glyphs );
    for ( int i = 0; i != 256; ++i )
        atlas->table[i] = nullptr;

    for ( int i = 0; i < atlas->num_glyphs; ++i )
    {
        FontGlyphBuffer buffer;

        if ( !ReadObject( file, buffer ) )
            break;

        atlas->glyphs[i].tex_x1 = float( buffer.x ) / atlas->width;
        atlas->glyphs[i].tex_x2 = float( buffer.x + buffer.width ) / atlas->width;
        atlas->glyphs[i].tex_y1 = float( buffer.y ) / atlas->height;
        atlas->glyphs[i].advance = buffer.width;
        atlas->table[buffer.ascii] = atlas->glyphs + i;
    }

    if ( ferror( file ) || feof( file ) )
    {
        printf( "%.3f: error: failed to read font glyph\n", global.timeBase.time );
        DestroyFontAtlas( allocator, atlas );
        fclose( file );
        return nullptr;
    }

    FontGlyph * default_glyph = atlas->table[ (unsigned char)'\xFF' ];
    if ( !default_glyph )
    {
        printf( "%.3f: error: font file contains no default glyph\n", global.timeBase.time );
        DestroyFontAtlas( allocator, atlas );
        fclose( file );
        return nullptr;
    }
    
    for ( int i = 0; i != 256; ++i )
    {
        if ( atlas->table[i] == NULL )
            atlas->table[i] = default_glyph;
    }

    const int buffer_size = atlas->width * atlas->height;

    uint8_t * tex_data = (uint8_t*) CORE_NEW_ARRAY( core::memory::scratch_allocator(), uint8_t, buffer_size );

    if ( fread( tex_data, buffer_size, 1, file ) != 1 )
    {
        printf( "%.3f: error: failed to read atlas texture data\n", global.timeBase.time );
        CORE_DELETE_ARRAY( allocator, tex_data, buffer_size );
        DestroyFontAtlas( allocator, atlas );
        fclose( file );
        return nullptr;
    }

    glGenTextures( 1, &atlas->texture );

    glBindTexture( GL_TEXTURE_2D, atlas->texture );

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, atlas->width, atlas->height, 0, GL_RED, GL_UNSIGNED_BYTE, tex_data );

    glBindTexture( GL_TEXTURE_2D, 0 );

    CORE_DELETE_ARRAY( allocator, tex_data, buffer_size );

    fclose( file );

    return atlas;
}

const int MaxFontVertices = 10 * 1024;

struct FontVertex
{
    float x,y,z;
    float r,g,b,a;
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
    m_render->currentFontVertex = 0;

    GLuint shader_program = global.shaderManager->GetShader( "Font" );
    if ( !shader_program )
        return;

    glUseProgram( shader_program );

    mat4 modelViewProjection = glm::ortho( 0.0f, (float) global.displayWidth, (float) global.displayHeight, 0.0f, -1.0f, 1.0f );
    int location = glGetUniformLocation( shader_program, "ModelViewProjection" );
    if ( location < 0 )
        return;

    glUniformMatrix4fv( location, 1, GL_FALSE, &modelViewProjection[0][0] );

    if ( m_render->vao == 0 )
    {
        glGenVertexArrays( 1, &m_render->vao );
        glBindVertexArray( m_render->vao );
        glEnableVertexAttribArray( 0 );
        glEnableVertexAttribArray( 1 );
        glEnableVertexAttribArray( 2 );
        glGenBuffers( 1, &m_render->vbo );
        glBindBuffer( GL_ARRAY_BUFFER, m_render->vbo );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (GLubyte*)0 );
        glVertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (GLubyte*)(3*4) );
        glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (GLubyte*)(7*4) );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
    }

    glEnable( GL_BLEND );

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glBindVertexArray( m_render->vao );

    glBindBuffer( GL_ARRAY_BUFFER, m_render->vbo );

    glBindTexture( GL_TEXTURE_2D, m_atlas->texture );
}

void Font::DrawAtlas( float x, float y, const Color & color )
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
    a.r = color.r;
    a.g = color.g;
    a.b = color.b;
    a.a = color.a;
    a.u = 0;
    a.v = 0;

    b.x = w;
    b.y = 0;
    b.z = 0;
    b.r = color.r;
    b.g = color.g;
    b.b = color.b;
    b.a = color.a;
    b.u = 1;
    b.v = 0;

    c.x = w;
    c.y = h;
    c.z = 0;
    c.r = color.r;
    c.g = color.g;
    c.b = color.b;
    c.a = color.a;
    c.u = 1;
    c.v = 1;

    d.x = 0;
    d.y = h;
    d.z = 0;
    d.r = color.r;
    d.g = color.g;
    d.b = color.b;
    d.a = color.a;
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

void Font::DrawText( float x, float y, const char * text, const Color & color )
{
    CORE_ASSERT( m_render );
    CORE_ASSERT( m_render->active );

    const char * p = text;

    while ( *p != '\0' )
    {
        char character = *p++;

        FontGlyph * glyph = m_atlas->table[(int)character];

        // a b
        // d c

        FontVertex a,b,c,d;

        a.x = x;
        a.y = y;
        a.z = 0;
        a.r = color.r;
        a.g = color.g;
        a.b = color.b;
        a.a = color.a;
        a.u = glyph->tex_x1;
        a.v = glyph->tex_y1;

        b.x = x + glyph->advance;
        b.y = y;
        b.z = 0;
        b.r = color.r;
        b.g = color.g;
        b.b = color.b;
        b.a = color.a;
        b.u = glyph->tex_x2;
        b.v = glyph->tex_y1;

        c.x = x + glyph->advance;
        c.y = y + m_atlas->line_height;
        c.z = 0;
        c.r = color.r;
        c.g = color.g;
        c.b = color.b;
        c.a = color.a;
        c.u = glyph->tex_x2;
        c.v = glyph->tex_y1 + m_atlas->tex_line_height;

        d.x = x;
        d.y = y + m_atlas->line_height;
        d.z = 0;
        d.r = color.r;
        d.g = color.g;
        d.b = color.b;
        d.a = color.a;
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

    glBindTexture( GL_TEXTURE_2D, 0 );

    glBindVertexArray( 0 );

    glUseProgram( 0 );

    glDisable( GL_BLEND );
}

#endif // #ifdef CLIENT
