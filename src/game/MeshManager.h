#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#ifdef CLIENT

#include <stdint.h>
#include "core/Types.h"

struct Mesh;

class MeshManager
{
public:

    MeshManager( core::Allocator & allocator );
    
    ~MeshManager();

    void Clear();

    void Reload();

    void LoadMesh( const char * filename );

    Mesh * GetMesh( const char * filename );

private:

    core::Hash<Mesh*> m_meshes;
    core::Allocator * m_allocator;
};

#endif // #ifdef CLIENT

#endif // #ifndef MESH_MANAGER_H
