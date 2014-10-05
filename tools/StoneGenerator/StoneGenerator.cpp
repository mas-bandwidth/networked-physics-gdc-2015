#include "virtualgo/Biconvex.h"
#include "core/Core.h"
#include "core/File.h"
#include "Mesh.h"
#include <stdio.h>

// stone sizes from http://www.kurokigoishi.co.jp/english/seihin/goishi/index.html

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
    STONE_SIZE_NumValues
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

const float StoneHeight[] = 
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

int main( int argc, char * argv[] )
{
    const char * stoneDirectory = "data/stones";
        
    const float width = 2.2f;
    const float bevel = 0.1f;
    const int subdivisions = 6;

    for ( int i = 0; i < STONE_SIZE_NumValues; ++i )
    {
        char filename[1024];
        
        sprintf( filename, "%s/Stone-%s.obj", stoneDirectory, StoneSizeNames[i] );
        
        printf( "%s\n", filename );

        const float height = StoneHeight[i];

        virtualgo::Biconvex biconvex( width, height, bevel );

        Mesh<Vertex> mesh;
        GenerateBiconvexMesh( mesh, biconvex, subdivisions );
        
        // todo: optimize mesh

        if ( !WriteMeshToObjFile( mesh, filename ) )
            exit( 1 );
    }

    return 0;
}
