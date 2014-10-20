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

MeshData * load_mesh_data( core::Allocator & allocator, const char * filename )
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

    MeshData * meshData = CORE_NEW( allocator, MeshData );

    ReadObject( file, meshData->numVertices );
    ReadObject( file, meshData->numTriangles );

    const int numIndices = meshData->numTriangles * 3;

    MeshVertex * vertices = CORE_NEW_ARRAY( core::memory::scratch_allocator(), MeshVertex, meshData->numVertices );
    uint16_t * indices = CORE_NEW_ARRAY( core::memory::scratch_allocator(), uint16_t, numIndices );

    if ( fread( &vertices[0], sizeof(MeshVertex) * meshData->numVertices, 1, file ) != 1 )
    {
        printf( "%.3f: error: failed to read vertices from mesh file\n", global.timeBase.time );
        CORE_DELETE( allocator, MeshData, meshData );
        CORE_DELETE_ARRAY( core::memory::scratch_allocator(), vertices, meshData->numVertices );
        CORE_DELETE_ARRAY( core::memory::scratch_allocator(), indices, numIndices );
        fclose( file );
        return nullptr;
    }

    if ( fread( &indices[0], 2 * numIndices, 1, file ) != 1 )
    {
        printf( "%.3f: error: failed to read indices from mesh file\n", global.timeBase.time );
        CORE_DELETE( allocator, MeshData, meshData );
        CORE_DELETE_ARRAY( core::memory::scratch_allocator(), vertices, meshData->numVertices );
        CORE_DELETE_ARRAY( core::memory::scratch_allocator(), indices, numIndices );
        fclose( file );
        return nullptr;
    }

    const int LOCATION_VERTEX_POSITION = 0;
    const int LOCATION_VERTEX_NORMAL = 1;
    const int LOCATION_VERTEX_MODEL = 2;
    const int LOCATION_VERTEX_MODEL_VIEW = 6;
    const int LOCATION_VERTEX_MODEL_VIEW_PROJECTION = 10;

    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    glEnableVertexAttribArray( 0 );
    glEnableVertexAttribArray( 1 );

    GLuint vbo;
    glGenBuffers( 1, &vbo );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBufferData( GL_ARRAY_BUFFER, meshData->numTriangles * sizeof(MeshVertex), vertices, GL_STATIC_DRAW );
    glVertexAttribPointer( LOCATION_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (GLubyte*)0 );
    glVertexAttribPointer( LOCATION_VERTEX_NORMAL, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(MeshVertex), (GLubyte*)(3*4) );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
 
    GLuint ibo;
    glGenBuffers( 1, &ibo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 2*numIndices, indices, GL_STATIC_DRAW );

    meshData->vertex_buffer = vbo;
    meshData->index_buffer = ibo;

    // ============================================================================================

    // todo: mesh should probably just be the vbo and ibo, and perhaps include a mesh type
    // eg. specifying if it has texture coordinates per-vertex or not. the actual binding
    // of vertex

    GLuint instance_buffer;
    glGenBuffers( 1, &instance_buffer );
    glBindBuffer( GL_ARRAY_BUFFER, instance_buffer );
    
    for ( unsigned int i = 0; i < 4 ; ++i )
    {
        glEnableVertexAttribArray( LOCATION_VERTEX_MODEL + i );
        glVertexAttribPointer( LOCATION_VERTEX_MODEL + i, 4, GL_FLOAT, GL_FALSE, sizeof( MeshInstanceData ), (void*) ( offsetof( MeshInstanceData, model ) + ( sizeof(float) * 4 * i ) ) );
        glVertexAttribDivisor( LOCATION_VERTEX_MODEL + i, 1 );

        glEnableVertexAttribArray( LOCATION_VERTEX_MODEL_VIEW + i );
        glVertexAttribPointer( LOCATION_VERTEX_MODEL_VIEW + i, 4, GL_FLOAT, GL_FALSE, sizeof( MeshInstanceData ), (void*) ( offsetof( MeshInstanceData, modelView ) + ( sizeof(float) * 4 * i ) ) );
        glVertexAttribDivisor( LOCATION_VERTEX_MODEL_VIEW + i, 1 );

        glEnableVertexAttribArray( LOCATION_VERTEX_MODEL_VIEW_PROJECTION + i );
        glVertexAttribPointer( LOCATION_VERTEX_MODEL_VIEW_PROJECTION + i, 4, GL_FLOAT, GL_FALSE, sizeof( MeshInstanceData ), (void*) ( offsetof( MeshInstanceData, modelViewProjection ) + ( sizeof(float) * 4 * i ) ) );
        glVertexAttribDivisor( LOCATION_VERTEX_MODEL_VIEW_PROJECTION + i, 1 );
    }

    /*
    int location = glGetAttribLocation( shader, "BaseColor" );
    if ( location >= 0 )
    {
        glEnableVertexAttribArray( location );
        glVertexAttribPointer( location, 1, GL_FLOAT, GL_FALSE, sizeof( MeshInstanceData ), (void*) ( offsetof( MeshInstanceData, baseColor ) );
    }
    */

    /*
layout ( location = 15 ) in float Metallic;
layout ( location = 16 ) in float Specular;
layout ( location = 17 ) in float Roughness;
    */

    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    meshData->vertex_array_object = vao;
    meshData->instance_buffer = instance_buffer;

    // ============================================================================================

    glBindVertexArray( 0 );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    CORE_DELETE_ARRAY( core::memory::scratch_allocator(), vertices, meshData->numVertices );
    CORE_DELETE_ARRAY( core::memory::scratch_allocator(), indices, numIndices );

    fclose( file );

    return meshData;
}

void destroy_mesh_data( core::Allocator & allocator, MeshData * mesh )
{
    CORE_ASSERT( mesh );

    glDeleteVertexArrays( 1, &mesh->vertex_array_object );
    glDeleteBuffers( 1, &mesh->vertex_buffer );
    glDeleteBuffers( 1, &mesh->index_buffer );
    glDeleteBuffers( 1, &mesh->instance_buffer );

    CORE_DELETE( allocator, MeshData, mesh );
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
        MeshData * meshData = itor->value;
//        printf( "%.3f: Delete mesh %p\n", global.timeBase.time, meshData );
        destroy_mesh_data( *m_allocator, meshData );
    }
 
    core::hash::clear( m_meshes );
}

void MeshManager::Reload()
{
    printf( "%.3f: Reloading meshes\n", global.timeBase.time );

    // todo: reload currently loaded meshes from disk
}

void MeshManager::LoadMesh( const char * filename )
{
    MeshData * existing = GetMeshData( filename );
    if ( existing != nullptr )
        return;

    MeshData * meshData = load_mesh_data( *m_allocator, filename );
    if ( !meshData )
        return;

    uint32_t key = core::hash_string( filename );

    core::hash::set( m_meshes, key, meshData );
}

MeshData * MeshManager::GetMeshData( const char * filename )
{
    const uint64_t key = core::hash_string( filename );
    return core::hash::get( m_meshes, key, (MeshData*)nullptr );
}

CONSOLE_FUNCTION( reload_meshes )
{
    CORE_ASSERT( global.meshManager );

    global.meshManager->Reload();
}

#endif // #ifdef CLIENT
