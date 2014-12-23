#ifndef GAME_CUBES_H
#define GAME_CUBES_H

#include "core/Memory.h"
#include "cubes/Game.h"
#include "cubes/View.h"
#include "cubes/Hypercube.h"

const int CubeSteps = 30;
const int MaxCubes = 1024 * 4;
const int MaxViews = 4;
const int MaxSimulations = 4;

typedef game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> GameInstance;

struct CubeInstance
{
    float r,g,b,a;
    vectorial::mat4f model;
    vectorial::mat4f modelView;
    vectorial::mat4f modelViewProjection;
};

#ifdef CLIENT

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

const int MaxCubeShadowVertices = 1024 * 32;

#define DEBUG_CUBE_SHADOWS 0

class CubesRender
{
public:

    CubesRender();
    ~CubesRender();

    void ResizeDisplay( int displayWidth, int displayHeight );
    
    void SetLightPosition( const math::Vector & position );

    void SetCamera( const math::Vector & position, const math::Vector & lookAt, const math::Vector & up );

    void ClearScreen();

    void BeginScene( float x1, float y1, float x2, float y2 );
 
    void RenderCubes( const view::Cubes & cubes );
    
    void RenderCubeShadows( const view::Cubes & cubes );

    void RenderShadowQuad();

    void EndScene();
                
private:

    void Initialize();

    bool initialized;

    int displayWidth;
    int displayHeight;

    math::Vector cameraPosition;
    math::Vector cameraLookAt;
    math::Vector cameraUp;

    math::Vector lightPosition;

    uint32_t cubes_vao;
    uint32_t cubes_vbo;
    uint32_t cubes_ibo;
    uint32_t cubes_instance_buffer;

    uint32_t shadow_vao;
    uint32_t shadow_vbo;

    uint32_t mask_vao;
    uint32_t mask_vbo;

    vectorial::vec3f shadow_vertices[MaxCubeShadowVertices];

    CubeInstance instance_data[MaxCubes];
};

#endif // #ifdef CLIENT

struct CubesConfig
{
    int num_simulations = 1;
    int num_views = 1;
};

struct CubesSimulation
{
    uint32_t frame = 0;
    GameInstance * game_instance = nullptr;
};

struct CubesUpdateConfig
{
    bool run_update[MaxSimulations];
    game::Input input[MaxSimulations];
};

#ifdef CLIENT

struct CubesView
{
    view::Camera camera;
    view::Packet packet;
    view::ObjectManager objects;
    view::ObjectUpdate updates[MaxViewObjects];
    view::Cubes cubes;
};

enum CubesRenderMode
{
    CUBES_RENDER_FULLSCREEN,
    CUBES_RENDER_SPLITSCREEN,
    CUBES_RENDER_QUADSCREEN
};

struct CubesRenderConfig
{
    CubesRenderConfig()
    {
        render_mode = CUBES_RENDER_FULLSCREEN;
    }

    CubesRenderMode render_mode;
};

#endif

struct CubesInternal
{
    CubesConfig config;

    CubesSimulation * simulation;

#ifdef CLIENT

    CubesView * view;
    CubesRender render;
    game::Input input;

#endif // #ifdef CLIENT

    void Initialize( core::Allocator & allocator, const CubesConfig & config );

    void Free( core::Allocator & allocator );

    void AddCube( GameInstance * gameInstance, int player, const math::Vector & position );

    void Update( const CubesUpdateConfig & update_config );

#ifdef CLIENT

    bool Clear();

    void Render( const CubesRenderConfig & render_config );

    bool KeyEvent( int key, int scancode, int action, int mods );

    const game::Input GetLocalInput() const { return input; }

#endif // #ifdef CLIENT
};

#endif // #ifndef GAME_CUBES_H
