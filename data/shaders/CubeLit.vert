#version 410

uniform mat4 Model;
uniform mat4 ModelView;
uniform mat4 ModelViewProjection;

in vec3 VertexPosition;
in vec3 VertexNormal;
in vec4 VertexBaseColor;

out vec3 Position;
out vec3 Normal;
out vec4 BaseColor;

void main()
{
    Normal = mat3( Model ) * VertexNormal;
    Position = vec3( Model * vec4( VertexPosition, 1.0 ) );
    BaseColor = VertexBaseColor;
    gl_Position = ModelViewProjection * vec4( VertexPosition, 1.0 );
}
