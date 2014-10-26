#version 410

in mat4 Model;
in mat4 ModelView;
in mat4 ModelViewProjection;

in vec3 VertexPosition;
in vec3 VertexNormal;
in vec3 VertexBaseColor;
in vec3 VertexSpecularColor;
in float VertexMetallic;
in float VertexRoughness;
in float VertexGloss;

out vec3 Position;
out vec3 Normal;
out vec3 BaseColor;
out vec3 SpecularColor;
out float Metallic;             // [0,1]
out float Roughness;            // [0,1]
out float SpecularPower;        // [0,~8k] - derived from gloss input

void main()
{
    Normal = mat3( Model ) * VertexNormal;
    Position = vec3( Model * vec4( VertexPosition, 1.0 ) );
    BaseColor = VertexBaseColor;
    SpecularColor = VertexSpecularColor;
    Metallic = VertexMetallic;
    Roughness = VertexRoughness;
    SpecularPower = exp2( 10 * VertexGloss + 1 );
    gl_Position = ModelViewProjection * vec4( VertexPosition, 1.0 );
}
