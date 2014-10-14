#include "StoneRender.h"
#include "MeshManager.h"
#include "ShaderManager.h"
#include "Render.h"
#include "Global.h"
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

void RenderStone( MeshData & mesh )
{   
    GLuint shader = global.shaderManager->GetShader( "Stone" );
    if ( !shader )
        return;

    glUseProgram( shader );

    mat4 projectionMatrix = glm::perspective( 50.0f, (float) global.displayWidth / (float) global.displayHeight, 0.1f, 100.0f );
     
    mat4 modelViewMatrix = glm::lookAt( glm::vec3(0.0f, -5.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) );

    mat4 MVP = projectionMatrix * modelViewMatrix;
         
    int location = glGetUniformLocation( shader, "MVP" );
    if ( location < 0 )
        return;

    glUniformMatrix4fv( location, 1, GL_FALSE, &MVP[0][0] );

    glBindVertexArray( mesh.vertex_array_object );

    glDrawElements( GL_TRIANGLES, mesh.numTriangles * 3, GL_UNSIGNED_SHORT, nullptr );

    glBindVertexArray( 0 );

    glUseProgram( 0 );
}

void RenderStones( MeshData & mesh )
{
    GLuint shader = global.shaderManager->GetShader( "StoneInstanced" );
    if ( !shader )
        return;

    glUseProgram( shader );

    int location = glGetAttribLocation( shader, "MVP" );
    if ( location < 0 )
        return;

    glBindVertexArray( mesh.vertex_array_object );

    mat4 projectionMatrix = glm::perspective( 50.0f, (float) global.displayWidth / (float) global.displayHeight, 0.1f, 250.0f );
     
    mat4 viewMatrix = glm::lookAt( glm::vec3( 0.0f, 0.0f, 10.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );

    const int NumInstances = 19 * 19;

    mat4 MVP[NumInstances];

    int instance = 0;

    for ( int i = 0; i < 19; ++i )
    {
        for ( int j = 0; j < 19; ++j )
        {  
            const float x = -19.8f + 2.2f * i;
            const float y = -19.8f + 2.2f * j;

            mat4 modelMatrix = glm::translate( mat4(1), vec3( x, y, 0.0f ) );

            mat4 rotation = glm::rotate( mat4(1), -(float)global.timeBase.time * 20, glm::vec3(0.0f,0.0f,1.0f));

            MVP[instance] = projectionMatrix * viewMatrix * rotation * modelMatrix;

            instance++;
        }
    }

    glBindBuffer( GL_ARRAY_BUFFER, mesh.instance_buffer );    
    glBufferData( GL_ARRAY_BUFFER, sizeof(float) * 16 * NumInstances, MVP, GL_STREAM_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    glDrawElementsInstanced( GL_TRIANGLES, mesh.numTriangles * 3, GL_UNSIGNED_SHORT, nullptr, NumInstances );

    glBindVertexArray( 0 );

    glUseProgram( 0 );
}
