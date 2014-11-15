#version 410

uniform mat4 ModelViewProjection;

in vec4 VertexBaseColor;
in vec3 VertexPosition;

flat out vec4 BaseColor;

void main()
{
    BaseColor = VertexBaseColor;
    gl_Position = ModelViewProjection * vec4( VertexPosition, 1.0 );
}
