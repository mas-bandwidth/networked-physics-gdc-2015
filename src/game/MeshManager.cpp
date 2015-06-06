#include "MeshManager.h"
#include "Render.h"

#ifdef CLIENT

#include "Mesh.h"
#include "Global.h"
#include "Console.h"
#include "core/Core.h"
#include "core/Hash.h"
#include "core/File.h"
#include "core/Memory.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stddef.h>

template <class T> bool ReadObject( FILE * file, const T & object )
{
    return fread( (char*) &object, sizeof(object), 1, file ) == 1;
}

Mesh * LoadMesh( core::Allocator & allocator, const char * filename )
{
    CORE_ASSERT( filename );

    printf( "%.3f: Loading mesh \"%s\"\n", global.timeBase.time, filename );

    FILE * file = fopen( filename, "rb" );
    if ( !file )
    {
        printf( "%.3f: error: failed to load mesh file \"%s\"\n", global.timeBase.time, filename );
        return nullptr;
    }

    char header[4];
    if ( fread( header, 4, 1, file ) != 1 || header[0] != 'M' || header[1] != 'E' || header[2] != 'S' || header[3] != 'H' )
    {
        printf( "%.3f: error: not a valid mesh file\n", global.timeBase.time );
        fclose( file );
        return nullptr;
    }

    Mesh * mesh = CORE_NEW( allocator, Mesh );

    memset( mesh, 0, sizeof(Mesh ) );

    ReadObject( file, mesh->numVertices );
    ReadObject( file, mesh->numTriangles );

    const int numIndices = mesh->numTriangles * 3;

    MeshVertex * vertices = CORE_NEW_ARRAY( core::memory::scratch_allocator(), MeshVertex, mesh->numVertices );
    uint16_t * indices = CORE_NEW_ARRAY( core::memory::scratch_allocator(), uint16_t, numIndices );

    if ( fread( &vertices[0], sizeof(MeshVertex) * mesh->numVertices, 1, file ) != 1 )
    {
        printf( "%.3f: error: failed to read vertices from mesh file\n", global.timeBase.time );
        CORE_DELETE( allocator, Mesh, mesh );
        CORE_DELETE_ARRAY( core::memory::scratch_allocator(), vertices, mesh->numVertices );
        CORE_DELETE_ARRAY( core::memory::scratch_allocator(), indices, numIndices );
        fclose( file );
        return nullptr;
    }

    if ( fread( &indices[0], 2 * numIndices, 1, file ) != 1 )
    {
        printf( "%.3f: error: failed to read indices from mesh file\n", global.timeBase.time );
        CORE_DELETE( allocator, Mesh, mesh );
        CORE_DELETE_ARRAY( core::memory::scratch_allocator(), vertices, mesh->numVertices );
        CORE_DELETE_ARRAY( core::memory::scratch_allocator(), indices, numIndices );
        fclose( file );
        return nullptr;
    }

    glGenBuffers( 1, &mesh->vbo );
    glBindBuffer( GL_ARRAY_BUFFER, mesh->vbo );
    glBufferData( GL_ARRAY_BUFFER, mesh->numTriangles * sizeof(MeshVertex), vertices, GL_STATIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
 
    glGenBuffers( 1, &mesh->ibo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->ibo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 2*numIndices, indices, GL_STATIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    CORE_DELETE_ARRAY( core::memory::scratch_allocator(), vertices, mesh->numVertices );
    CORE_DELETE_ARRAY( core::memory::scratch_allocator(), indices, numIndices );

    fclose( file );

    return mesh;
}

void DestroyMesh( core::Allocator & allocator, Mesh * mesh )
{
    CORE_ASSERT( mesh );

    glDeleteBuffers( 1, &mesh->vbo );
    glDeleteBuffers( 1, &mesh->ibo );

    memset( mesh, 0, sizeof(Mesh ) );

    CORE_DELETE( allocator, Mesh, mesh );
}

MeshManager::MeshManager( core::Allocator & allocator )
    : m_meshes( allocator )
{
    m_allocator = &allocator;
    core::hash::reserve( m_meshes, 256 );
}

MeshManager::~MeshManager()
{
    Clear();
}

void MeshManager::Clear()
{
    for ( auto itor = core::hash::begin( m_meshes ); itor != core::hash::end( m_meshes ); ++itor )
    {
        Mesh * mesh = itor->value;
//        printf( "%.3f: Delete mesh %p\n", global.timeBase.time, mesh );
        DestroyMesh( *m_allocator, mesh );
    }
 
    core::hash::clear( m_meshes );
}

void MeshManager::Reload()
{
    printf( "%.3f: Reloading meshes\n", global.timeBase.time );
}

void MeshManager::LoadMesh( const char * filename )
{
    Mesh * existing = GetMesh( filename );
    if ( existing != nullptr )
        return;

    Mesh * mesh = ::LoadMesh( *m_allocator, filename );
    if ( !mesh )
        return;

    uint32_t key = core::hash_string( filename );

    core::hash::set( m_meshes, key, mesh );
}

Mesh * MeshManager::GetMesh( const char * filename )
{
    const uint64_t key = core::hash_string( filename );
    return core::hash::get( m_meshes, key, (Mesh*)nullptr );
}

CONSOLE_FUNCTION( reload_meshes )
{
    CORE_ASSERT( global.meshManager );

    global.meshManager->Reload();
}

#endif // #ifdef CLIENT
