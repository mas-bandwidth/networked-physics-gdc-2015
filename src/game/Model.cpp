#ifdef CLIENT

#include "Model.h"
#include "Mesh.h"
#include "Render.h"
#include <GL/glew.h>
#include "core/Core.h"
#include "core/Memory.h"

Model * CreateModel( core::Allocator & allocator, uint32_t shader, Mesh * mesh )
{
    check_opengl_error( "before create model" );

    CORE_ASSERT( shader );
    CORE_ASSERT( mesh );
    CORE_ASSERT( mesh->vbo > 0 );
    CORE_ASSERT( mesh->ibo > 0 );

    Model * model = CORE_NEW( allocator, Model );

    memset( model, 0, sizeof( Model ) );

    model->numVertices = mesh->numVertices;
    model->numTriangles = mesh->numTriangles;
    model->shader = shader;

    glUseProgram( shader );

    glGenVertexArrays( 1, &model->vertexArrayObject );
    glBindVertexArray( model->vertexArrayObject );

    // bind the index buffer to the VAO

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->ibo );

    // setup the vertex buffer

    glBindBuffer( GL_ARRAY_BUFFER, mesh->vbo );

    const int vertex_position_location = glGetAttribLocation( shader, "VertexPosition" );
    if ( vertex_position_location >= 0 )
    {
        glEnableVertexAttribArray( vertex_position_location );
        glVertexAttribPointer( vertex_position_location, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (GLubyte*)0 );
    }

    const int vertex_normal_location = glGetAttribLocation( shader, "VertexNormal" );
    if ( vertex_normal_location >= 0 )
    {
        glEnableVertexAttribArray( vertex_normal_location );
        glVertexAttribPointer( vertex_normal_location, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(MeshVertex), (GLubyte*)(3*4) );
    }

    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    
    // setup the per-instance buffer

    glGenBuffers( 1, &model->instanceBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, model->instanceBuffer );

    const int model_location = glGetAttribLocation( shader, "Model" );
    const int model_view_location = glGetAttribLocation( shader, "ModelView" );
    const int model_view_projection_location = glGetAttribLocation( shader, "ModelViewProjection" );

    for ( unsigned int i = 0; i < 4 ; ++i )
    {
        if ( model_location >= 0 )
        {
            glEnableVertexAttribArray( model_location + i );
            glVertexAttribPointer( model_location + i, 4, GL_FLOAT, GL_FALSE, sizeof( ModelInstanceData ), (void*) ( offsetof( ModelInstanceData, model ) + ( sizeof(float) * 4 * i ) ) );
            glVertexAttribDivisor( model_location + i, 1 );
        }

        if ( model_view_location >= 0 )
        {
            glEnableVertexAttribArray( model_view_location + i );
            glVertexAttribPointer( model_view_location + i, 4, GL_FLOAT, GL_FALSE, sizeof( ModelInstanceData ), (void*) ( offsetof( ModelInstanceData, modelView ) + ( sizeof(float) * 4 * i ) ) );
            glVertexAttribDivisor( model_view_location + i, 1 );
        }

        if ( model_view_projection_location >= 0 )
        {
            glEnableVertexAttribArray( model_view_projection_location + i );
            glVertexAttribPointer( model_view_projection_location + i, 4, GL_FLOAT, GL_FALSE, sizeof( ModelInstanceData ), (void*) ( offsetof( ModelInstanceData, modelViewProjection ) + ( sizeof(float) * 4 * i ) ) );
            glVertexAttribDivisor( model_view_projection_location + i, 1 );
        }
    }

    const int base_color_location = glGetAttribLocation( shader, "VertexBaseColor" );
    if ( base_color_location >= 0 )
    {
        glEnableVertexAttribArray( base_color_location );
        glVertexAttribPointer( base_color_location, 3, GL_FLOAT, GL_FALSE, sizeof( ModelInstanceData ), (void*) ( offsetof( ModelInstanceData, baseColor ) ) );
        glVertexAttribDivisor( base_color_location, 1 );
    }

    const int specular_color_location = glGetAttribLocation( shader, "VertexSpecularColor" );
    if ( specular_color_location >= 0 )
    {
        glEnableVertexAttribArray( specular_color_location );
        glVertexAttribPointer( specular_color_location, 3, GL_FLOAT, GL_FALSE, sizeof( ModelInstanceData ), (void*) ( offsetof( ModelInstanceData, specularColor ) ) );
        glVertexAttribDivisor( specular_color_location, 1 );
    }

    const int metallic_location = glGetAttribLocation( shader, "VertexMetallic" );
    if ( metallic_location >= 0 )
    {
        glEnableVertexAttribArray( metallic_location );
        glVertexAttribPointer( metallic_location, 1, GL_FLOAT, GL_FALSE, sizeof( ModelInstanceData ), (void*) ( offsetof( ModelInstanceData, metallic ) ) );
        glVertexAttribDivisor( metallic_location, 1 );
    }

    const int roughness_location = glGetAttribLocation( shader, "VertexRoughness" );
    if ( roughness_location >= 0 )
    {
        glEnableVertexAttribArray( roughness_location );
        glVertexAttribPointer( roughness_location, 1, GL_FLOAT, GL_FALSE, sizeof( ModelInstanceData ), (void*) ( offsetof( ModelInstanceData, roughness ) ) );
        glVertexAttribDivisor( roughness_location, 1 );
    }

    const int gloss_location = glGetAttribLocation( shader, "VertexGloss" );
    if ( gloss_location >= 0 )
    {
        glEnableVertexAttribArray( gloss_location );
        glVertexAttribPointer( gloss_location, 1, GL_FLOAT, GL_FALSE, sizeof( ModelInstanceData ), (void*) ( offsetof( ModelInstanceData, gloss ) ) );
        glVertexAttribDivisor( gloss_location, 1 );
    }

    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    glBindVertexArray( 0 );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    glUseProgram( 0 );

    check_opengl_error( "after create model" );

    return model;
}

void DestroyModel( core::Allocator & allocator, Model * model )
{
    CORE_ASSERT( model );

    glDeleteBuffers( 1, &model->instanceBuffer );
    glDeleteVertexArrays( 1, &model->vertexArrayObject );

    memset( model, 0, sizeof( Model ) );

    CORE_DELETE( allocator, Model, model );
}

void DrawModels( Model & model, int numInstances, const ModelInstanceData * instanceData )
{
    check_opengl_error( "before draw models" );

    CORE_ASSERT( model.shader > 0 );
    CORE_ASSERT( model.numTriangles > 0 );
    CORE_ASSERT( model.vertexArrayObject > 0 );
    CORE_ASSERT( model.instanceBuffer > 0 );
    CORE_ASSERT( numInstances > 0 );
    CORE_ASSERT( instanceData );

    glBindBuffer( GL_ARRAY_BUFFER, model.instanceBuffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(ModelInstanceData) * numInstances, instanceData, GL_STREAM_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    glBindVertexArray( model.vertexArrayObject );

    glDrawElementsInstanced( GL_TRIANGLES, model.numTriangles * 3, GL_UNSIGNED_SHORT, nullptr, numInstances );

    glBindVertexArray( 0 );

    check_opengl_error( "after draw models" );
}

#endif // #ifdef CLIENT
