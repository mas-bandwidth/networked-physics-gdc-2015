#include "StoneDemo.h"

#ifdef CLIENT

#include "Global.h"
#include "Mesh.h"
#include "MeshManager.h"
#include "StoneManager.h"
#include "ShaderManager.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

const int NumStoneSizes = 14;

static const char * white_stones[] = 
{
    "White-22",
    "White-25",
    "White-28",
    "White-30",
    "White-31",
    "White-32",
    "White-33",
    "White-34",
    "White-35",
    "White-36",
    "White-37",
    "White-38",
    "White-39",
    "White-40"
};

static const char * black_stones[] = 
{
    "Black-22",
    "Black-25",
    "Black-28",
    "Black-30",
    "Black-31",
    "Black-32",
    "Black-33",
    "Black-34",
    "Black-35",
    "Black-36",
    "Black-37",
    "Black-38",
    "Black-39",
    "Black-40"
};

enum StoneColor
{
    BLACK,
    WHITE
};

enum StoneMode
{
    SINGLE_STONE,
    ROTATING_STONES
};

static bool stoneColor = WHITE;
static int stoneSize = NumStoneSizes - 1;
static int stoneMode = SINGLE_STONE;

StoneDemo::StoneDemo()
{
    // ...
}

StoneDemo::~StoneDemo()
{
    // ...
}

bool StoneDemo::Initialize()
{
    return true;
}

void StoneDemo::Update()
{
    // ...
}

void StoneDemo::Render()
{
    const char * stoneName = ( stoneColor == WHITE ) ? white_stones[stoneSize] : black_stones[stoneSize];
    const StoneData * stoneData = global.stoneManager->GetStoneData( stoneName );
    if ( !stoneData )
        return;

    MeshData * stoneMesh = global.meshManager->GetMeshData( stoneData->mesh_filename );
    if ( !stoneMesh )
    {
        global.meshManager->LoadMesh( stoneData->mesh_filename );
        stoneMesh = global.meshManager->GetMeshData( stoneData->mesh_filename );
        if ( !stoneMesh )
            return;
    }

    GLuint shader = global.shaderManager->GetShader( "Stone" );
    if ( !shader )
        return;

    glUseProgram( shader );

    switch ( stoneMode )
    {
        case SINGLE_STONE:        
        {
            mat4 projectionMatrix = glm::perspective( 50.0f, (float) global.displayWidth / (float) global.displayHeight, 0.1f, 100.0f );

            mat4 viewMatrix = glm::lookAt( vec3(0.0f, -5.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) );
            
            mat4 modelMatrix(1);

            vec4 lightPosition = viewMatrix * vec4(0,0,5,1);

            int location = glGetUniformLocation( shader, "LightPosition" );
            if ( location >= 0 )
                glUniform4fv( location, 1, &lightPosition[0] );

            const int NumInstances = 1;

            MeshInstanceData instanceData[NumInstances];

            instanceData[0].modelView = viewMatrix * modelMatrix;
            instanceData[0].modelViewProjection = projectionMatrix * viewMatrix * modelMatrix;

            DrawMeshInstances( *stoneMesh, NumInstances, instanceData );
        }
        break;

        case ROTATING_STONES:
        {
            mat4 projectionMatrix = glm::perspective( 50.0f, (float) global.displayWidth / (float) global.displayHeight, 0.1f, 250.0f );
             
            mat4 viewMatrix = glm::lookAt( vec3( 0.0f, 0.0f, 10.0f ), vec3( 0.0f, 0.0f, 0.0f ), vec3( 0.0f, 1.0f, 0.0f ) );
            
            const int NumInstances = 19 * 19;

            MeshInstanceData instanceData[NumInstances];

            int instance = 0;

            vec4 lightPosition = viewMatrix * vec4(0,0,10,1);

            int location = glGetUniformLocation( shader, "LightPosition" );
            if ( location >= 0 )
                glUniform4fv( location, 1, &lightPosition[0] );

            for ( int i = 0; i < 19; ++i )
            {
                for ( int j = 0; j < 19; ++j )
                {  
                    const float x = -19.8f + 2.2f * i;
                    const float y = -19.8f + 2.2f * j;

                    mat4 rotation = glm::rotate( mat4(1), -(float)global.timeBase.time * 20, vec3(0.0f,0.0f,1.0f));

                    mat4 modelMatrix = rotation * glm::translate( mat4(1), vec3( x, y, 0.0f ) );

                    mat4 modelViewMatrix = viewMatrix * modelMatrix;

                    instanceData[instance].modelView = modelViewMatrix;
                    instanceData[instance].modelViewProjection = projectionMatrix * modelViewMatrix;

                    instance++;
                }
            }

            DrawMeshInstances( *stoneMesh, NumInstances, instanceData );
        }
        break;
    }

    glUseProgram( 0 );
}

bool StoneDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    if ( ( action == GLFW_PRESS || action == GLFW_REPEAT ) && mods == 0 )
    {
        if ( key == GLFW_KEY_UP )
        {
            stoneSize++;
            if ( stoneSize > NumStoneSizes - 1 )
                stoneSize = NumStoneSizes - 1;
            return true;
        }        
        else if ( key == GLFW_KEY_DOWN )
        {
            stoneSize--;
            if ( stoneSize < 0 )
                stoneSize = 0;
        }
        else if ( key == GLFW_KEY_LEFT )
        {
            stoneColor = BLACK;
        }
        else if ( key == GLFW_KEY_RIGHT )
        {
            stoneColor = WHITE;
        }
        else if ( key == GLFW_KEY_1 )
        {
            stoneMode = SINGLE_STONE;
        }
        else if ( key == GLFW_KEY_2 )
        {
            stoneMode = ROTATING_STONES;
        }
    }

    return false;
}

bool StoneDemo::CharEvent( unsigned int code )
{
    // ...

    return false;
}

#endif // #ifdef CLIENT
