
#include "virtualgo/Stones.h"
#include "virtualgo/Biconvex.h"
#include "virtualgo/InertiaTensor.h"
#include "core/Core.h"
#include "core/File.h"
#include "Mesh.h"

using namespace virtualgo;

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

inline float GetStoneWidth( StoneSize size, StoneColor color )
{
    return 2.2f + ( color == STONE_COLOR_BLACK ? 0.1f : 0.0f );
}

inline float GetStoneHeight( StoneSize size )
{
    return StoneHeight[size];
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

    StoneData stoneData[NumStones];

    for ( int i = 0; i < NumStones; ++i )
    {
        StoneSize size = stoneDefinition[i].size;
        StoneColor color = stoneDefinition[i].color;

        char filename[256];
        
        sprintf( filename, "%s/Stone-%s-%s.mesh", stoneDirectory, StoneColorNames[color], StoneSizeNames[size] );

        printf( "%s\n", filename );        

        const float width = GetStoneWidth( size, color );
        const float height = GetStoneHeight( size );
        const float bevel = 0.1f;
        const int subdivisions = 6;

        virtualgo::Biconvex biconvex( width, height, bevel );

        const float mass = 1.0f;
        vec3f inertia;
        mat4f inertiaTensor;
        mat4f inverseInertiaTensor;
        virtualgo::CalculateBiconvexInertiaTensor( mass, biconvex, inertia, inertiaTensor, inverseInertiaTensor );

        Mesh<Vertex> mesh;
        GenerateBiconvexMesh( mesh, biconvex, subdivisions );

        if ( !WriteMeshFile( mesh, filename ) )
            exit( 1 );

        stoneData[i].size = size;
        stoneData[i].color = color;
        stoneData[i].subdivisions = subdivisions;
        stoneData[i].width = width;
        stoneData[i].height = height;
        stoneData[i].bevel = bevel;
        stoneData[i].mass = mass;
        stoneData[i].inertia = inertia;
        strcpy( stoneData[i].mesh_filename, filename );
    }

    // write the stone data out the "Stones.bin" file

    char stones_bin_filename[256];
    sprintf( stones_bin_filename, "%s/Stones.bin", stoneDirectory );
    FILE * file = fopen( stones_bin_filename, "wb" );
    if ( !file )
    {
        printf( "failed to open stones file for writing: \"%s\"\n", stones_bin_filename );
        exit( 1 );
    }

    fwrite( "STONES", 6, 1, file );

    core::WriteObject( file, NumStones );

    for ( int i = 0; i < NumStones; ++i )
        core::WriteObject( file, stoneData[i] );

    fclose( file );

    return 0;
}
