#ifndef RENDER_H
#define RENDER_H

#ifdef CLIENT

#include <stdint.h>

void clear_opengl_error();

void check_opengl_error( const char * message );

uint32_t load_shader( const char * vertex_file_path, const char * fragment_file_path );

void delete_shader( uint32_t shader );

#endif // #ifdef CLIENT

#endif // #ifndef RENDER_H
 