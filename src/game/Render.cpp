#include "Render.h"

#ifdef CLIENT

#include "Global.h"
#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

void clear_opengl_error()
{
    while ( glGetError() != GL_NO_ERROR );
}

void check_opengl_error( const char * message )
{
    int error = glGetError();
    if ( error != GL_NO_ERROR )
    {
        printf( "%.3f: opengl error - %s (%s)\n", global.timeBase.time, gluErrorString( error ), message );
        exit( 1 );
    }    
}

bool load_text_file( const char * filename, char * buffer, int buffer_size )
{
    FILE * file = fopen( filename, "r" );
    if ( !file )
        return false;

    fseek( file, 0L, SEEK_END );
    uint64_t file_size = ftell( file );
    fseek( file, 0L, SEEK_SET );

    if ( file_size >= buffer_size )
    {
        fclose( file );
        return false;
    }

    fread( buffer, 1, file_size, file );

    fclose( file );

    buffer[file_size] = '\0';

    return true;
}

uint32_t load_shader( const char * filename, int type )
{
    GLuint shader_id = glCreateShader( type );

    static const int BufferSize = 256 * 1024;
    char buffer[BufferSize];
    if ( !load_text_file( filename, buffer, BufferSize ) )
    {
        printf( "error: failed to load shader %s\n", filename );
        return 0;
    }

    char const * source = buffer;
    glShaderSource( shader_id, 1, &source, NULL );
    glCompileShader( shader_id );
 
    GLint result = GL_FALSE;
    glGetShaderiv( shader_id, GL_COMPILE_STATUS, &result );
    if ( result == GL_FALSE )
    {
        int info_log_length;
        glGetShaderiv( shader_id, GL_INFO_LOG_LENGTH, &info_log_length );
        char info_log[info_log_length];
        glGetShaderInfoLog( shader_id, info_log_length, NULL, info_log );
        printf( "=================================================================\n"
                "failed to compile shader: %s\n"
                "=================================================================\n"
                "%s"
                "=================================================================\n", filename, info_log );
        glDeleteShader( shader_id );
        return 0;
    }

    return shader_id;
}

uint32_t load_shader( const char * vertex_file_path, const char * fragment_file_path )
{
    uint32_t vertex_shader_id = load_shader( vertex_file_path, GL_VERTEX_SHADER );
    if ( !vertex_shader_id )
    {
        return 0;
    }

    uint32_t fragment_shader_id = load_shader( fragment_file_path, GL_FRAGMENT_SHADER );
    if ( !fragment_shader_id )
    {
        glDeleteShader( vertex_shader_id );
        return 0;
    }

    GLuint program_id = glCreateProgram();
    glAttachShader( program_id, vertex_shader_id );
    glAttachShader( program_id, fragment_shader_id );
    glLinkProgram( program_id );
 

    GLint result = GL_FALSE;
    glGetProgramiv( program_id, GL_LINK_STATUS, &result );
    if ( result == GL_FALSE )
    {
        int info_log_length;
        glGetShaderiv( program_id, GL_INFO_LOG_LENGTH, &info_log_length );
        char info_log[info_log_length];
        glGetShaderInfoLog( program_id, info_log_length, NULL, info_log );
        printf( "=================================================================\n" \
                "failed to link shader: %s - %s\n"
                "=================================================================\n"
                "%s"
                "=================================================================\n", vertex_file_path, fragment_file_path, info_log );
        glDeleteShader( vertex_shader_id );
        glDeleteShader( fragment_shader_id );
        glDeleteProgram( program_id );
        return 0;
    }
 
    glDeleteShader( vertex_shader_id );
    glDeleteShader( fragment_shader_id );
 
    return program_id;
}

void delete_shader( uint32_t shader )
{
    glDeleteProgram( shader );
}

#endif // #ifdef CLIENT
