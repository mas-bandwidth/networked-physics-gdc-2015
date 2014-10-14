#include "StoneDemo.h"

#ifdef CLIENT

#include "Global.h"
#include "StoneRender.h"
#include "MeshManager.h"
#include "StoneManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

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

    switch ( stoneMode )
    {
        case SINGLE_STONE:        
            RenderStone( *stoneMesh );
            break;

        case ROTATING_STONES:
            RenderStones( *stoneMesh );
    }
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
