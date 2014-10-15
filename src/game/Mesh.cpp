#include "Mesh.h"

#ifdef CLIENT

#include "core/Core.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdio.h>
#include <stdlib.h>

using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;

void DrawMeshInstances( MeshData & mesh, uint32_t shader, int numInstances, const MeshInstanceData * instanceData )
{
    CORE_ASSERT( shader );

    glUseProgram( shader );

    glBindVertexArray( mesh.vertex_array_object );

    glBindBuffer( GL_ARRAY_BUFFER, mesh.instance_buffer );    
    glBufferData( GL_ARRAY_BUFFER, sizeof(MeshInstanceData) * numInstances, instanceData, GL_STREAM_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    glDrawElementsInstanced( GL_TRIANGLES, mesh.numTriangles * 3, GL_UNSIGNED_SHORT, nullptr, numInstances );

    glBindVertexArray( 0 );

    glUseProgram( 0 );
}

#endif // #ifdef CLIENT
