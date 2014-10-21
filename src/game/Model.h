#ifdef CLIENT

#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include <glm/glm.hpp>

namespace core { class Allocator; }

enum ModelType
{
    MODEL_STONE
};

struct Model
{
    ModelType type;
    int numVertices;
    int numTriangles;
    uint32_t vertexArrayObject;
    uint32_t instanceBuffer;
};

struct Mesh;

Model * CreateModel( core::Allocator & allocator, uint32_t shader, Mesh * mesh );

void DestroyModel( core::Allocator & allocator, Model * model );

struct ModelInstanceData
{
    glm::mat4 model;
    glm::mat4 modelView;
    glm::mat4 modelViewProjection;
    glm::vec4 baseColor;
    float metallic;
    float specular; 
    float roughness;
};

extern void DrawModels( Model & model, int numInstances, const ModelInstanceData * instanceData );

#endif // #ifdef CLIENT

#endif // #ifndef MODEL_H
