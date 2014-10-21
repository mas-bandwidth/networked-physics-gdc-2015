#ifdef CLIENT

#ifndef MESH_H
#define MESH_H

#include <stdint.h>

struct MeshVertex
{
    float x,y,z;
    uint32_t normal;
};

struct Mesh
{
    int numVertices;
    int numTriangles;
    uint32_t vbo;
    uint32_t ibo;
};

#endif // #ifndef MESH_H

#endif // #ifdef CLIENT
