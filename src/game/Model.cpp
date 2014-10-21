#ifdef CLIENT

#include "Model.h"
#include "Mesh.h"
#include "Render.h"
#include <GL/glew.h>
#include "core/Core.h"
#include "core/Memory.h"

Model * CreateModel( core::Allocator & allocator, uint32_t shader, Mesh * mesh )
{
    CORE_ASSERT( shader );
    CORE_ASSERT( mesh );

    Model * model = CORE_NEW( allocator, Model );

    memset( model, 0, sizeof( Model ) );

    model->numVertices = mesh->numVertices;
    model->numTriangles = mesh->numTriangles;

    glGenVertexArrays( 1, &model->vertexArrayObject );
    
    glGenBuffers( 1, &model->instanceBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, model->instanceBuffer );

    // todo: should probably just look these guys up in the shader instead
    const int LOCATION_MODEL = 2;
    const int LOCATION_MODEL_VIEW = 6;
    const int LOCATION_MODEL_VIEW_PROJECTION = 10;

    for ( unsigned int i = 0; i < 4 ; ++i )
    {
        glEnableVertexAttribArray( LOCATION_MODEL + i );
        glVertexAttribPointer( LOCATION_MODEL + i, 4, GL_FLOAT, GL_FALSE, sizeof( ModelInstanceData ), (void*) ( offsetof( ModelInstanceData, model ) + ( sizeof(float) * 4 * i ) ) );
        glVertexAttribDivisor( LOCATION_MODEL + i, 1 );

        glEnableVertexAttribArray( LOCATION_MODEL_VIEW + i );
        glVertexAttribPointer( LOCATION_MODEL_VIEW + i, 4, GL_FLOAT, GL_FALSE, sizeof( ModelInstanceData ), (void*) ( offsetof( ModelInstanceData, modelView ) + ( sizeof(float) * 4 * i ) ) );
        glVertexAttribDivisor( LOCATION_MODEL_VIEW + i, 1 );

        glEnableVertexAttribArray( LOCATION_MODEL_VIEW_PROJECTION + i );
        glVertexAttribPointer( LOCATION_MODEL_VIEW_PROJECTION + i, 4, GL_FLOAT, GL_FALSE, sizeof( ModelInstanceData ), (void*) ( offsetof( ModelInstanceData, modelViewProjection ) + ( sizeof(float) * 4 * i ) ) );
        glVertexAttribDivisor( LOCATION_MODEL_VIEW_PROJECTION + i, 1 );
    }

    // todo: hook up the various shader specific attributes
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

    glBindVertexArray( 0 );

    return model;
}

void DestroyModel( core::Allocator & allocator, Model * model )
{
    CORE_ASSERT( model );

    glDeleteVertexArrays( 1, &model->vertexArrayObject );

    glDeleteBuffers( 1, &model->instanceBuffer );

    memset( model, 0, sizeof( Model ) );

    CORE_DELETE( allocator, Model, model );
}

void DrawModels( Model & model, int numInstances, const ModelInstanceData * instanceData )
{
    glBindVertexArray( model.vertexArrayObject );

    glBindBuffer( GL_ARRAY_BUFFER, model.instanceBuffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(ModelInstanceData) * numInstances, instanceData, GL_STREAM_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    glDrawElementsInstanced( GL_TRIANGLES, model.numVertices, GL_UNSIGNED_SHORT, nullptr, numInstances );

    glBindVertexArray( 0 );
}

#endif // #ifdef CLIENT
