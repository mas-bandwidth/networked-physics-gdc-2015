#ifndef RENDER_H
#define RENDER_H

#ifdef CLIENT

#include <GL/glew.h>             // todo: maybe we don't want to expose OpenGL stuff to the bulk of the client
#include <GLFW/glfw3.h>

void clear_opengl_error();

void check_opengl_error( const char * message );

GLuint load_shader( const char * vertex_file_path, const char * fragment_file_path );

#endif // #ifdef CLIENT

#endif // #ifndef RENDER_H
 