#version 410

out vec3 Normal;

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in mat4 MVP;

void main()
{
    Normal = VertexNormal / 1023 + 0.5;
    gl_Position = MVP * vec4( VertexPosition, 1.0 );
}