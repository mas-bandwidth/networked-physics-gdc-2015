#ifdef CLIENT

#ifndef STONE_MANAGER_H
#define STONE_MANAGER_H

#include "core/Types.h"

namespace core { class Allocator; }

struct StoneData
{
    float width;
    float height;
    float bevel;
    float mass;
    float inertia_x;
    float inertia_y;
    float inertia_z;
    char mesh_filename[256];
};

class StoneManager
{
public:

    StoneManager( core::Allocator & allocator );

    ~StoneManager();

    const StoneData * GetStoneData( const char * name ) const;

    void Reload();

private:

    void Load();
    void Unload();

    core::Hash<StoneData*> m_stones;
    core::Allocator * m_allocator;
};

#endif // #ifndef STONE_MANAGER_H

#endif // #ifdef CLIENT
