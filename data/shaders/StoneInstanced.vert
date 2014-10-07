#version 410

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in mat4 MVP;

void main()
{
    gl_Position = MVP * vec4( VertexPosition, 1.0 );
}