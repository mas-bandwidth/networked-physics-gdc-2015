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
    // setup shader

    GLuint shader = global.shaderManager->GetShader( "Stone" );
    if ( !shader )
        return;

    glUseProgram( shader );

    // -------------------------------

    // setup projection matrix
     
    mat4 projectionMatrix = glm::perspective( 50.0f, (float) global.displayWidth / (float) global.displayHeight, 0.1f, 100.0f );
     
    mat4 modelViewMatrix = glm::lookAt( glm::vec3(0.0f, -5.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) );

    mat4 MVP = projectionMatrix * modelViewMatrix;
     
    // -------------------------------

    /*
    // todo: construct projection matrix, model view matrix, normal matrix, MVP

    glm::mat4 projectionMatrix = glm::perspective( 35.0f, 1.0f, 0.1f, 100.0f );
    glm::mat4 modelViewMatrix = glm::translate( mat4(1.0f), vec3(0.0f,1.0f, 0.0f) );

    mat3 normalMatrix( 1 );

    mat4 MVP = projectionMatrix * modelViewMatrix;
    */

    vec4 lightPosition( 1, 1, 10, 1 );
    vec4 lightIntensity( 1, 1, 1, 1 );

    vec3 kd( 0.7, 0.7, 0.7 );
    vec3 ka( 0.2, 0.2, 0.2 );
    vec3 ks( 1.0, 1.0, 1.0 );
    
//    float shininess = 10;

    // ========================================================================

    int location;
    
    /*
    location = glGetUniformLocation( shader, "ModelViewMatrix" );
    if ( location < 0 )
        return;
    glUniformMatrix4fv( location, 1, GL_FALSE, &modelViewMatrix[0][0] );

    location = glGetUniformLocation( shader, "NormalMatrix" );
    if ( location < 0 )
        return;
    glUniformMatrix3fv( location, 1, GL_FALSE, &normalMatrix[0][0] );
    */

    location = glGetUniformLocation( shader, "MVP" );
    if ( location < 0 )
        return;
    glUniformMatrix4fv( location, 1, GL_FALSE, &MVP[0][0] );

    /*
    location = glGetUniformLocation( shader, "LightPosition" );
    if ( location < 0 )
        return;
    glUniform4fv( location, 4, &lightPosition[0] );

    location = glGetUniformLocation( shader, "LightIntensity" );
    if ( location < 0 )
        return;
    glUniform4fv( location, 4, &lightIntensity[0] );

    location = glGetUniformLocation( shader, "Kd" );
    if ( location < 0 )
        return;
    glUniform3fv( location, 3, &kd[0] );

    location = glGetUniformLocation( shader, "Ka" );
    if ( location < 0 )
        return;
    glUniform3fv( location, 3, &ka[0] );

    location = glGetUniformLocation( shader, "Ks" );
    if ( location < 0 )
        return;
    glUniform3fv( location, 3, &ks[0] );

    location = glGetUniformLocation( shader, "Shininess" );
    if ( location < 0 )
        return;
    glUniform1f( location, shininess );
    */

    // ========================================================================

    glBindVertexArray( mesh.vao );

    glDrawElements( GL_TRIANGLES, mesh.numTriangles * 3, GL_UNSIGNED_SHORT, nullptr );

    glBindVertexArray( 0 );
}
