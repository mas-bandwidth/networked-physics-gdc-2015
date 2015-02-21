#ifndef GAME_CUBES_H
#define GAME_CUBES_H

#include "core/Memory.h"
#include "cubes/Game.h"
#include "cubes/View.h"
#include "cubes/Hypercube.h"

const int CubeSteps = 30;
const int MaxCubes = 1024;
const int MaxViews = 4;
const int MaxSimulations = 4;
const int MaxSimFrames = 4;

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

//#define DEBUG_CUBE_SHADOWS 1

class CubesRender
{
public:

    CubesRender();
    ~CubesRender();

    void ResizeDisplay( int displayWidth, int displayHeight );
    
    void SetLightPosition( const vectorial::vec3f & position );

    void SetCamera( const vectorial::vec3f & position, const vectorial::vec3f & lookAt, const vectorial::vec3f & up );

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

    vectorial::vec3f cameraPosition;
    vectorial::vec3f cameraLookAt;
    vectorial::vec3f cameraUp;

    vectorial::vec3f lightPosition;

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
    bool soften_simulation = false;
};

struct CubesSimulation
{
    uint32_t frame = 0;
    GameInstance * game_instance = nullptr;
};

struct CubesUpdateConfigPerSim
{
    CubesUpdateConfigPerSim()
    {
        num_frames = 0;
    }

    int num_frames;
    game::Input frame_input[MaxSimFrames];
};

struct CubesUpdateConfig
{
    CubesUpdateConfigPerSim sim[MaxSimulations];
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

struct CubesRenderConfigPerView
{
    vectorial::vec3f * position_error = nullptr;
    vectorial::quat4f * orientation_error = nullptr;
};

struct CubesRenderConfig
{
    CubesRenderConfig()
    {
        render_mode = CUBES_RENDER_FULLSCREEN;
    }

    CubesRenderMode render_mode;
    CubesRenderConfigPerView view[MaxViews];
};

#endif

struct CubesSettings
{
    bool deterministic = true;
};

struct CubesInternal
{
    CubesConfig config;

    const CubesSettings * settings;

    CubesSimulation * simulation;

#ifdef CLIENT

    CubesView * view;
    CubesRender render;
    game::Input input;

#endif // #ifdef CLIENT

    void Initialize( core::Allocator & allocator, const CubesConfig & config, const CubesSettings * settigs );

    void Free( core::Allocator & allocator );

    void AddCube( GameInstance * gameInstance, int player, const vectorial::vec3f & position );

    void Update( const CubesUpdateConfig & update_config );

#ifdef CLIENT

    bool Clear();

    void Render( const CubesRenderConfig & render_config );

    bool KeyEvent( int key, int scancode, int action, int mods );

    GameInstance * GetGameInstance( int simulation_index );

    const game::Input GetLocalInput() const { return input; }

#endif // #ifdef CLIENT
};

#endif // #ifndef GAME_CUBES_H
