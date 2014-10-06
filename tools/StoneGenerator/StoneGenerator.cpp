/*
    Stone Generator Tool
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/  

#include "virtualgo/Biconvex.h"
#include "virtualgo/InertiaTensor.h"
#include "core/Core.h"
#include "core/File.h"
#include "Mesh.h"

using namespace virtualgo;

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

const char * StoneSizeNames[]
{
    "22",
    "25",
    "28",
    "30",
    "31",
    "32",
    "33",
    "34",
    "35",
    "36",
    "37",
    "38",
    "39",
    "40"
};

enum StoneColor
{
    STONE_COLOR_BLACK,
    STONE_COLOR_WHITE
};

const char * StoneColorNames[]
{
    "Black",
    "White"
};

const float StoneHeight[]
{
    0.63f,
    0.70f,
    0.75f,
    0.80f,
    0.84f,
    0.88f,
    0.92f,
    0.95f,
    0.98f,
    1.01f,
    1.04f,
    1.07f,
    1.10f,
    1.13f
};

const float StoneBevel[]
{
    0.06f,      // 22
    0.08f,      // 25
    0.08f,      // 28
    0.10f,      // 30
    0.11f,      // 31
    0.11f,      // 32
    0.1125f,    // 33
    0.1130f,    // 34
    0.1150f,    // 35
    0.1200f,    // 36
    0.1200f,    // 37
    0.1200f,    // 38
    0.1450f,    // 39
    0.15f       // 40
};

inline float GetStoneWidth( StoneSize size, StoneColor color )
{
    return 2.2f + ( color == STONE_COLOR_BLACK ? 0.1f : 0.0f );
}

inline float GetStoneHeight( StoneSize size )
{
    return StoneHeight[size];
}

inline float GetStoneBevel( StoneSize size )
{
    return StoneBevel[size];
}

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

bool WriteStoneFile( const char * filename, const StoneData & stoneData )
{
    FILE * file = fopen( filename, "wb" );
    if ( !file )
    {
        printf( "failed to open stone file for writing: \"%s\"\n", filename );
        return false;
    }

    fwrite( "STONE", 5, 1, file );

    core::WriteObject( file, stoneData );

    fclose( file );

    return true;
}

int main( int argc, char * argv[] )
{
    const char * stoneDirectory = "data/stones";
        
    // setup stone definitions for black and white stones of all sizes

    struct StoneDefinition
    {
        StoneSize size;
        StoneColor color;
    };

    const int NumStones = NUM_STONE_SIZES * 2;

    StoneDefinition stoneDefinition[NumStones];

    for ( int i = 0; i < NUM_STONE_SIZES; ++i )
    {
        stoneDefinition[i].size = (StoneSize) i;
        stoneDefinition[i].color = STONE_COLOR_BLACK;
    }

    for ( int i = 0; i < NUM_STONE_SIZES; ++i )
    {
        stoneDefinition[i+NUM_STONE_SIZES].size = (StoneSize) i;
        stoneDefinition[i+NUM_STONE_SIZES].color = STONE_COLOR_WHITE;
    }

    // generate stone data and stone meshes

    for ( int i = 0; i < NumStones; ++i )
    {
        StoneSize size = stoneDefinition[i].size;
        StoneColor color = stoneDefinition[i].color;

        char mesh_filename[256];
        sprintf( mesh_filename, "%s/%s-%s.mesh", stoneDirectory, StoneColorNames[color], StoneSizeNames[size] );

        char stone_filename[256];
        sprintf( stone_filename, "%s/%s-%s.stone", stoneDirectory, StoneColorNames[color], StoneSizeNames[size] );

        printf( "Generating %s -> %s\n", stone_filename, mesh_filename );

        const float width = GetStoneWidth( size, color );
        const float height = GetStoneHeight( size );
        const float bevel = GetStoneBevel( size );
        const int subdivisions = 6;

        virtualgo::Biconvex biconvex( width, height, bevel );

        const float mass = 1.0f;
        vec3f inertia;
        mat4f inertiaTensor;
        mat4f inverseInertiaTensor;
        virtualgo::CalculateBiconvexInertiaTensor( mass, biconvex, inertia, inertiaTensor, inverseInertiaTensor );

        Mesh<Vertex> mesh;
        GenerateBiconvexMesh( mesh, biconvex, subdivisions );

        if ( !WriteMeshFile( mesh, mesh_filename ) )
            exit( 1 );

        StoneData stoneData;
        stoneData.width = width;
        stoneData.height = height;
        stoneData.bevel = bevel;
        stoneData.mass = mass;
        stoneData.inertia_x = inertia.x();
        stoneData.inertia_y = inertia.y();
        stoneData.inertia_z = inertia.z();
        strcpy( stoneData.mesh_filename, mesh_filename );

        if ( !WriteStoneFile( stone_filename, stoneData ) )
            exit( 1 );
    }

    return 0;
}
