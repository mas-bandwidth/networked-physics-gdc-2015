#include "Mesh.h"

#ifdef CLIENT

#include "core/Core.h"
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>

void DrawMeshInstances( MeshData & mesh, int numInstances, const MeshInstanceData * instanceData )
{
    glBindVertexArray( mesh.vertex_array_object );

    glBindBuffer( GL_ARRAY_BUFFER, mesh.instance_buffer );    
    glBufferData( GL_ARRAY_BUFFER, sizeof(MeshInstanceData) * numInstances, instanceData, GL_STREAM_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    glDrawElementsInstanced( GL_TRIANGLES, mesh.numTriangles * 3, GL_UNSIGNED_SHORT, nullptr, numInstances );

    glBindVertexArray( 0 );
}

#endif // #ifdef CLIENT
