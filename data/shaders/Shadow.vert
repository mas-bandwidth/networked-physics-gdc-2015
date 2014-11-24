#version 410

layout (location = 0) in vec3 VertexPosition;

uniform mat4 ModelViewProjection;

void main()
{
    gl_Position = ModelViewProjection * vec4( VertexPosition, 1.0 );
}
