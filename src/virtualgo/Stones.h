#ifndef VIRTUALGO_STONE_H
#define VIRTUALGO_STONE_H

#include "vectorial/vec3f.h"

namespace core { class Allocator; }

namespace virtualgo
{
    using namespace vectorial;

    enum StoneSize
    {
        STONE_SIZE_22,
        STONE_SIZE_25,
        STONE_SIZE_28,
        STONE_SIZE_30,
        STONE_SIZE_31,
        STONE_SIZE_32,
        STONE_SIZE_33,
        STONE_SIZE_34,
        STONE_SIZE_35,
        STONE_SIZE_36,
        STONE_SIZE_37,
        STONE_SIZE_38,
        STONE_SIZE_39,
        STONE_SIZE_40,
        NUM_STONE_SIZES
    };

    enum StoneColor
    {
        STONE_COLOR_BLACK,
        STONE_COLOR_WHITE
    };

    struct StoneData
    {
        StoneSize size;
        StoneColor color;
        int subdivisions;
        float width;
        float height;
        float bevel;
        float mass;
        vec3f inertia;
        char mesh_filename[256];
    };

    class StoneManager
    {
    public:

        StoneManager( core::Allocator & allocator );

        ~StoneManager();

        const StoneData & GetStoneData( StoneSize size, StoneColor color ) const;

    private:

        bool Load();

        core::Allocator * m_allocator;
        int m_numStones;
        StoneData * m_stoneData;
    };
}

#endif // #ifndef VIRTUALGO_STONES_H
