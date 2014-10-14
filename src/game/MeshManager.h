#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#ifdef CLIENT

#include <stdint.h>
#include "core/Types.h"

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

class MeshManager
{
public:

    MeshManager( core::Allocator & allocator );
    
    ~MeshManager();

    void Clear();

    void Reload();

    void LoadMesh( const char * filename );

    MeshData * GetMeshData( const char * filename );

private:

    core::Hash<MeshData*> m_meshes;
    core::Allocator * m_allocator;
};

#endif // #ifdef CLIENT

#endif // #ifndef MESH_MANAGER_H
