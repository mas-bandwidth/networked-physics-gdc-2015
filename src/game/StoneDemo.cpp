#include "StoneDemo.h"

#ifdef CLIENT

#include "Global.h"
#include "Mesh.h"
#include "Model.h"
#include "Render.h"
#include "MeshManager.h"
#include "StoneManager.h"
#include "ShaderManager.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "nvImage/nvImage.h"

using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;

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

static GLuint cubemap_texture = 0;

static bool stoneDirty = true;
static Model * stoneModel = nullptr;

static bool rotating = false;
static float rotationSpeed = 1.0f;
static double rotationAngle = 0.0;

StoneDemo::StoneDemo( core::Allocator & allocator )
{
    stoneDirty = true;
    m_allocator = &allocator;
}

StoneDemo::~StoneDemo()
{
    if ( stoneModel )
    {
        DestroyModel( *m_allocator, stoneModel );
        stoneModel = nullptr;
    }

    if ( cubemap_texture )
    {
        glDeleteTextures( 1, &cubemap_texture );
        cubemap_texture = 0;
    }
}

bool StoneDemo::Initialize()
{
#if 0
    const char * filename = "data/cubemaps/uffizi.dds";

    // todo: check if the filename doesn't exist. if not don't try to load it

    CDDSImage image;
    image.load( filename );    
    if ( !image.is_valid() || !image.is_cubemap() )
    {
        printf( "%.3f: error - cubemap image \"%s\" is not valid, or is not a cubemap\n", global.timeBase.time, filename );
        return false;
    }

    printf( "is_compressed = %d\n", image.is_compressed() );
    printf( "is_cubemap = %d\n", image.is_cubemap() );
    printf( "is_valid = %d\n", image.is_valid() );
    printf( "width = %d\n", image.get_width() );
    printf( "height = %d\n", image.get_height() );
    printf( "depth = %d\n", image.get_depth() );
    printf( "num_mipmaps = %d\n", image.get_num_mipmaps() );

    glGenTextures( 1, &cubemap_texture );

    glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
    
    glBindTexture( GL_TEXTURE_CUBE_MAP, cubemap_texture );

    for ( int n = 0; n < 6; n++ )
    {
        int target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + n;

        auto & cubemap_face = image.get_cubemap_face( n );

        const int num_mipmaps = cubemap_face.get_num_mipmaps();

        for ( int i = 0; i <= num_mipmaps; ++i )
        {
            const int format = GL_RGBA;
            const int width = (i==0) ? cubemap_face.get_width() : image.get_mipmap(i-1).get_width();
            const int height = (i==0) ? cubemap_face.get_height() : image.get_mipmap(i-1).get_height();
            const uint8_t * data = (i==0) ? cubemap_face : cubemap_face.get_mipmap(i-1);
            glTexImage2D( target, i, format, width, height, 0, format, GL_UNSIGNED_BYTE, data );
        }
    }

    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );

    check_opengl_error( "after load cubemap dds" );

#endif

    return true;
}

void StoneDemo::Update()
{
    const float targetRotationSpeed = rotating ? 1.0f : 0.0f;

    rotationSpeed += ( targetRotationSpeed - rotationSpeed ) * 0.999f;

    rotationAngle += global.timeBase.deltaTime * rotationSpeed * 20;
}

bool StoneDemo::Clear()
{
    // let the game framework handle the clear (default)
    return false;
}

void StoneDemo::Render()
{
    glEnable( GL_DEPTH_TEST );

    if ( stoneDirty )
    {
        if ( stoneModel )
        {
            DestroyModel( *m_allocator, stoneModel );
            stoneModel = nullptr;
        }

        const char * stoneName = ( stoneColor == WHITE ) ? white_stones[stoneSize] : black_stones[stoneSize];
        const StoneData * stoneData = global.stoneManager->GetStoneData( stoneName );
        if ( !stoneData )
            return;

        Mesh * stoneMesh = global.meshManager->GetMesh( stoneData->mesh_filename );
        if ( !stoneMesh )
        {
            global.meshManager->LoadMesh( stoneData->mesh_filename );
            stoneMesh = global.meshManager->GetMesh( stoneData->mesh_filename );
            if ( !stoneMesh )
                return;
        }

        GLuint shader = global.shaderManager->GetShader( "Stone" );
        if ( !shader )
            return;

        stoneModel = CreateModel( *m_allocator, shader, stoneMesh );

        if ( stoneModel )
            stoneDirty = false;
    }

    if ( !stoneModel )
        return;

    glUseProgram( stoneModel->shader );

    glActiveTexture( GL_TEXTURE0 );

    glBindTexture( GL_TEXTURE_CUBE_MAP, cubemap_texture );

    int location = glGetUniformLocation( stoneModel->shader, "CubeMap" );
    if ( location >= 0 )
        glUniform1i( location, 0 );

    switch ( stoneMode )
    {
        case SINGLE_STONE:        
        {
            mat4 projectionMatrix = glm::perspective( 50.0f, (float) global.displayWidth / (float) global.displayHeight, 0.1f, 100.0f );

            vec3 eyePosition( 0.0f, -2.5f, 0.0f );

            mat4 viewMatrix = glm::lookAt( eyePosition, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) );
            
            mat4 modelMatrix(1);

            vec4 lightPosition = vec4(0,0,10,1);

            int location = glGetUniformLocation( stoneModel->shader, "EyePosition" );
            if ( location >= 0 )
                glUniform3fv( location, 1, &eyePosition[0] );

            location = glGetUniformLocation( stoneModel->shader, "LightPosition" );
            if ( location >= 0 )
                glUniform3fv( location, 1, &lightPosition[0] );

            const int NumInstances = 1;

            ModelInstanceData instanceData[NumInstances];

            instanceData[0].model = modelMatrix;
            instanceData[0].modelView = viewMatrix * modelMatrix;
            instanceData[0].modelViewProjection = projectionMatrix * viewMatrix * modelMatrix;
            instanceData[0].baseColor = vec3( 0.18f, 0.18f, 0.9f );               // blue
            instanceData[0].specularColor = vec3( 1.0f, 0.765557, 0.336057 );     // gold
            instanceData[0].roughness = 0.5f;
            instanceData[0].metallic = 1.0f;
            instanceData[0].gloss = 0.5f;

            DrawModels( *stoneModel, NumInstances, instanceData );
        }
        break;

        case ROTATING_STONES:
        {
            mat4 projectionMatrix = glm::perspective( 40.0f, (float) global.displayWidth / (float) global.displayHeight, 0.1f, 250.0f );
             
            //vec3 eyePosition( 0.0f, 0.0f, 10.0f );
            //vec3 eyePosition( 0.0f, -40.0f, 7.0f );
            vec3 eyePosition( 0.0f, -25.0f, 7.0f );

            mat4 viewMatrix = glm::lookAt( eyePosition, vec3( 0.0f, -2.0f, 0.0f ), vec3( 0.0f, 1.0f, 0.0f ) );
            
            const int Size = 9;

            const int NumInstances = Size * Size;

            ModelInstanceData instanceData[NumInstances];

            int instance = 0;

            vec4 lightPosition = vec4(0,0,100,1);

            int location = glGetUniformLocation( stoneModel->shader, "EyePosition" );
            if ( location >= 0 )
                glUniform3fv( location, 1, &eyePosition[0] );

            location = glGetUniformLocation( stoneModel->shader, "LightPosition" );
            if ( location >= 0 )
                glUniform3fv( location, 1, &lightPosition[0] );

            float metallic = 0.0f;

            for ( int i = 0; i < Size; ++i )
            {
                float roughness = 0.0f;

                for ( int j = 0; j < Size; ++j )
                {  
                    const float x = -( (Size-1)/2.0 * 2.2f ) + 2.2f * i;
                    const float y = -( (Size-1)/2.0 * 2.2f ) + 2.2f * j;

                    mat4 rotation = glm::rotate( mat4(1), -(float)rotationAngle, vec3(0.0f,0.0f,1.0f));

                    mat4 modelMatrix = rotation * glm::translate( mat4(1), vec3( x, y, 0.0f ) );

                    mat4 modelViewMatrix = viewMatrix * modelMatrix;

                    instanceData[instance].model = modelMatrix;
                    instanceData[instance].modelView = modelViewMatrix;
                    instanceData[instance].modelViewProjection = projectionMatrix * modelViewMatrix;
                    instanceData[instance].baseColor = vec3( 0.18f, 0.18f, 0.9f );             // blue
                    instanceData[instance].specularColor = vec3( 1.0f, 0.765557, 0.336057 );   // gold
                    instanceData[instance].roughness = roughness;
                    instanceData[instance].metallic = metallic;
                    instanceData[instance].gloss = 0.5f;

                    instance++;

                    roughness += 1.0f / float(Size-1);
                }

                metallic += 1.0f / float(Size-1);
            }

            DrawModels( *stoneModel, NumInstances, instanceData );
        }
        break;
    }

    glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );

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
            stoneDirty = true;
            return true;
        }        
        else if ( key == GLFW_KEY_DOWN )
        {
            stoneSize--;
            if ( stoneSize < 0 )
                stoneSize = 0;
            stoneDirty = true;
            return true;
        }
        else if ( key == GLFW_KEY_LEFT )
        {
            stoneColor = BLACK;
            stoneDirty = true;
            return true;
        }
        else if ( key == GLFW_KEY_RIGHT )
        {
            stoneColor = WHITE;
            stoneDirty = true;
            return true;
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
    if ( code == GLFW_KEY_SPACE )
    {
        rotating = !rotating;
        return true;
    }

    return false;
}

#endif // #ifdef CLIENT
