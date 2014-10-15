/*
    Stone Tool
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/  

#include "virtualgo/Biconvex.h"
#include "virtualgo/InertiaTensor.h"
#include "core/Core.h"
#include "core/File.h"
#include "ToolMesh.h"

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
    0.08f,      // 22
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
    return 2.2f;// + ( color == STONE_COLOR_BLACK ? 0.1f : 0.0f );
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

void SubdivideBiconvexMesh( Mesh<Vertex> & mesh,
                            const virtualgo::Biconvex & biconvex, 
                            bool i, bool j, bool k,
                            vec3f a, vec3f b, vec3f c,
                            vec3f an, vec3f bn, vec3f cn,
                            vec3f sphereCenter,
                            bool clockwise,
                            float h, int depth, int subdivisions,
                            float texture_a, float texture_b )
{
    // edges: i = c -> a
    //        j = a -> b
    //        k = b -> c

    // vertices:
    //
    //        a
    //
    //     e     d 
    //
    //  b     f     c

    if ( depth < subdivisions )
    {
        vec3f d = ( a + c ) / 2;
        vec3f e = ( a + b ) / 2;
        vec3f f = ( b + c ) / 2;

        vec3f dn, en, fn;

        const float sphereRadius = biconvex.GetSphereRadius();
        const float bevelCircleRadius = biconvex.GetBevelCircleRadius();

        vec3f bevelOffset( 0, 0, h );

        if ( i )
        {
            d = normalize( d - bevelOffset ) * bevelCircleRadius + bevelOffset;
            dn = normalize( d - sphereCenter );
        }
        else
        {
            dn = normalize( d - sphereCenter );
            d = sphereCenter + dn * sphereRadius;
        }

        if ( j )
        {
            e = normalize( e - bevelOffset ) * bevelCircleRadius + bevelOffset;
            en = normalize( e - sphereCenter );
        }
        else
        {
            en = normalize( e - sphereCenter );
            e = sphereCenter + en * sphereRadius;
        }

        if ( k )
        {
            f = normalize( f - bevelOffset ) * bevelCircleRadius + bevelOffset;
            fn = normalize( f - sphereCenter );
        }
        else
        {
            fn = normalize( f - sphereCenter );
            f = sphereCenter + fn * sphereRadius;
        }

        depth++;

        // vertices:
        //
        //        a
        //
        //     e     d 
        //
        //  b     f     c

        SubdivideBiconvexMesh( mesh, biconvex, i, j, false, a, e, d, an, en, dn, sphereCenter, clockwise, h, depth, subdivisions, texture_a, texture_b );
        SubdivideBiconvexMesh( mesh, biconvex, false, j, k, e, b, f, en, bn, fn, sphereCenter, clockwise, h, depth, subdivisions, texture_a, texture_b );
        SubdivideBiconvexMesh( mesh, biconvex, i, false, k, d, f, c, dn, fn, cn, sphereCenter, clockwise, h, depth, subdivisions, texture_a, texture_b );
        SubdivideBiconvexMesh( mesh, biconvex, false, false, false, d, e, f, dn, en, fn, sphereCenter, clockwise, h, depth, subdivisions, texture_a, texture_b );
    }
    else
    {
        Vertex v1,v2,v3;

        v1.position = a;
        v1.normal = an;

        v2.position = b;
        v2.normal = bn;

        v3.position = c;
        v3.normal = cn;

        if ( clockwise )
            mesh.AddTriangle( v1, v3, v2 );
        else
            mesh.AddTriangle( v1, v2, v3 );
    }
}

void GenerateBiconvexMesh( Mesh<Vertex> & mesh, const virtualgo::Biconvex & biconvex, int subdivisions = 5, int numTriangles = 5, int numBevelRings = 8, float epsilon = 0.001f )
{
    const float bevelCircleRadius = biconvex.GetBevelCircleRadius();

    const float deltaAngle = 360.0f / numTriangles;

    const float h = biconvex.GetBevel() / 2;

    const float texture_a = 1 / biconvex.GetWidth();
    const float texture_b = 0.5f;

    // top

    for ( int i = 0; i < numTriangles; ++i )
    {
        mat4f r1 = mat4f::axisRotation( deltaAngle * i, vec3f(0,0,1) );
        mat4f r2 = mat4f::axisRotation( deltaAngle * ( i + 1 ), vec3f(0,0,1) );

        vec3f a = vec3f( 0, 0, -biconvex.GetSphereOffset() + biconvex.GetSphereRadius() );
        vec3f b = transformPoint( r1, vec3f( 0, bevelCircleRadius, h ) );
        vec3f c = transformPoint( r2, vec3f( 0, bevelCircleRadius, h ) );

        const vec3f sphereCenter = vec3f( 0, 0, -biconvex.GetSphereOffset() );

        vec3f an = normalize( a - sphereCenter );
        vec3f bn = normalize( b - sphereCenter );
        vec3f cn = normalize( c - sphereCenter );

        SubdivideBiconvexMesh( mesh, biconvex, false, false, true, a, b, c, an, bn, cn, sphereCenter, true, h, 0, subdivisions, texture_a, texture_b);
    }

    // bevel

    const float bevel = biconvex.GetBevel();

    if ( bevel > 0.001f )
    {
        // find bottom circle edge of biconvex top

        std::vector<float> circleAngles;

        int numVertices = mesh.GetNumVertices();
        Vertex * vertices = mesh.GetVertexBuffer();
        for ( int i = 0; i < numVertices; ++i )
        {
            if ( vertices[i].position.z() < h + epsilon )
                circleAngles.push_back( atan2( vertices[i].position.y(), vertices[i].position.x() ) );
        }

        std::sort( circleAngles.begin(), circleAngles.end() );

        // tesselate the bevel

        const float torusMajorRadius = biconvex.GetBevelTorusMajorRadius();
        const float torusMinorRadius = biconvex.GetBevelTorusMinorRadius();

        const float delta_z = biconvex.GetBevel() / numBevelRings;

        for ( int i = 0; i < numBevelRings; ++i )
        {
            const float z1 = bevel / 2 - i * delta_z;
            const float z2 = bevel / 2 - (i+1) * delta_z;

            for ( int j = 0; j < circleAngles.size(); ++j )
            {
                const float angle1 = circleAngles[j];
                const float angle2 = circleAngles[(j+1)%circleAngles.size()];

                vec3f circleCenter1 = vec3f( cos( angle1 ), sin( angle1 ), 0 ) * torusMajorRadius;
                vec3f circleCenter2 = vec3f( cos( angle2 ), sin( angle2 ), 0 ) * torusMajorRadius;

                vec3f circleUp( 0, 0, 1 );

                vec3f circleRight1 = normalize( circleCenter1 );
                vec3f circleRight2 = normalize( circleCenter2 );

                const float circleX1 = sqrt( torusMinorRadius*torusMinorRadius - z1*z1 );
                const float circleX2 = sqrt( torusMinorRadius*torusMinorRadius - z2*z2 );

                vec3f a = circleCenter1 + circleX1 * circleRight1 + z1 * circleUp;
                vec3f b = circleCenter1 + circleX2 * circleRight1 + z2 * circleUp;
                vec3f c = circleCenter2 + circleX2 * circleRight2 + z2 * circleUp;
                vec3f d = circleCenter2 + circleX1 * circleRight2 + z1 * circleUp;

                vec3f an = normalize( a - circleCenter1 );
                vec3f bn = normalize( b - circleCenter1 );
                vec3f cn = normalize( c - circleCenter2 );
                vec3f dn = normalize( d - circleCenter2 );

                Vertex v1,v2,v3;

                v1.position = a;
                v1.normal = an;

                v2.position = c;
                v2.normal = cn;

                v3.position = b;
                v3.normal = bn;

                mesh.AddTriangle( v1, v2, v3 );

                v1.position = a;
                v1.normal = an;

                v2.position = d;
                v2.normal = dn;

                v3.position = c;
                v3.normal = cn;

                mesh.AddTriangle( v1, v2, v3 );
            }
        }
    }

    // bottom

    for ( int i = 0; i < numTriangles; ++i )
    {
        mat4f r1 = mat4f::axisRotation( deltaAngle * i, vec3f(0,0,1) );
        mat4f r2 = mat4f::axisRotation( deltaAngle * ( i + 1 ), vec3f(0,0,1) );

        vec3f a = vec3f( 0, 0, biconvex.GetSphereOffset() - biconvex.GetSphereRadius() );
        vec3f b = transformPoint( r1, vec3f( 0, bevelCircleRadius, -h ) );
        vec3f c = transformPoint( r2, vec3f( 0, bevelCircleRadius, -h ) );

        const vec3f sphereCenter = vec3f( 0, 0, biconvex.GetSphereOffset() );

        vec3f an = normalize( a - sphereCenter );
        vec3f bn = normalize( b - sphereCenter );
        vec3f cn = normalize( c - sphereCenter );

        SubdivideBiconvexMesh( mesh, biconvex, false, false, true, a, b, c, an, bn, cn, sphereCenter, false, -h, 0, subdivisions, texture_a, texture_b );//0, 0 );
    }

    // IMPORTANT: Make sure we haven't blown past 16 bit indices
    const int numIndices = mesh.GetNumTriangles() * 3;
    CORE_ASSERT( numIndices <= 65535 );
}

void FindBiconvexWithRealWidth( virtualgo::Biconvex & biconvex, float real_width, float height, float bevel, float threshold = 0.0001f )
{
    if ( bevel < 0.0001f )
    {
        biconvex = virtualgo::Biconvex( real_width, height, bevel );
        return;
    }

    float width_a = real_width;
    float width_b = real_width * 2;

    Biconvex biconvex_a;
    Biconvex biconvex_b;

    int iteration = 0;

    while ( true )
    {
        biconvex_a = Biconvex( width_a, height, bevel );
        biconvex_b = Biconvex( width_b, height, bevel );

        const float real_width_a = biconvex_a.GetRealWidth();
        const float real_width_b = biconvex_b.GetRealWidth();

        const float delta_a = fabs( real_width - real_width_a );
        const float delta_b = fabs( real_width - real_width_b );

        if ( delta_a < delta_b )
        {
            if ( delta_a < threshold )
            {
                biconvex = biconvex_a;
                return;
            }

            width_b = ( width_a + width_b ) / 2;
        }
        else
        {
            if ( delta_b < threshold )
            {
                biconvex = biconvex_b;
                return;
            }

            width_a = ( width_a + width_b ) / 2;
        }

        iteration++;
    }
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

        virtualgo::Biconvex biconvex;
        FindBiconvexWithRealWidth( biconvex, width, height, bevel );

        const int subdivisions = 5;

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
