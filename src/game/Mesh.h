#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <stdio.h>
#include <stdlib.h>

using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;

struct MeshVertex
{
    float x,y,z;
    uint32_t normal;
};

struct MeshData
{
    MeshData()
    {
        numVertices = 0;
        numTriangles = 0;
        vertex_array_object = 0;
        vertex_buffer = 0;
        index_buffer = 0;
        instance_buffer = 0;
    }

    int numVertices;
    int numTriangles;
    uint32_t vertex_array_object;
    uint32_t vertex_buffer;
    uint32_t index_buffer;
    uint32_t instance_buffer;
};

struct MeshInstanceData
{
    mat4 model;
    mat4 modelView;
    mat4 modelViewProjection;
    vec4 baseColor;
    float metallic;
    float specular; 
    float roughness;
};

extern void DrawMeshInstances( MeshData & mesh, int numInstances, const MeshInstanceData * instanceData );

#endif // #ifndef MESH_H