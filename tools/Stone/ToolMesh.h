#ifndef TOOL_MESH_H
#define TOOL_MESH_H

#include "core/Core.h"
#include "core/File.h"
#include <list>
#include <vector>
#include "vectorial/vec2f.h"
#include "vectorial/vec3f.h"

using namespace vectorial;

struct Vertex
{
    vec3f position;
    vec3f normal;
};

struct TexturedVertex
{
    vec3f position;
    vec3f normal;
    vec2f texCoords;
};

template <typename vertex_t, typename index_t = uint16_t> class Mesh
{
public:

    Mesh( int numBuckets = 2048 )
    {
        this->numBuckets = numBuckets;
        buckets = new std::list<index_t>[numBuckets];
    }

    ~Mesh()
    {
        delete [] buckets;
        buckets = NULL;
    }

    void Clear()
    {
        for ( int i = 0; i < numBuckets; ++i )
            buckets[i].clear();
        indexBuffer.clear();
        vertexBuffer.clear();
    }

    void AddTriangle( const vertex_t & a, const vertex_t & b, const vertex_t & c )
    {
        indexBuffer.push_back( AddVertex( a ) );
        indexBuffer.push_back( AddVertex( b ) );
        indexBuffer.push_back( AddVertex( c ) );
    }

    int GetNumIndices() const { return indexBuffer.size(); }
    int GetNumVertices() const { return vertexBuffer.size(); }
    int GetNumTriangles() const { return indexBuffer.size() / 3; }

    vertex_t * GetVertexBuffer() { CORE_ASSERT( vertexBuffer.size() ); return &vertexBuffer[0]; }

    index_t * GetIndexBuffer() { CORE_ASSERT( indexBuffer.size() ); return &indexBuffer[0]; }

    int GetLargestBucketSize() const
    {
        int largest = 0;
        for ( int i = 0; i < numBuckets; ++i )
        {
            if ( buckets[i].size() > largest )
                largest = buckets[i].size();
        }
        return largest;
    }

    float GetAverageBucketSize() const
    {
        float average = 0.0f;
        for ( int i = 0; i < numBuckets; ++i )
            average += buckets[i].size();
        return average / numBuckets;
    }

    int GetNumZeroBuckets() const
    {
        int numZero = 0;
        for ( int i = 0; i < numBuckets; ++i )
            if ( buckets[i].size() == 0 )
                numZero++;
        return numZero;
    }

protected:

    int GetGridCellBucket( uint32_t x, uint32_t y, uint32_t z )
    {
        uint32_t data[3] = { x, y, z };
        return core::hash_data( (const uint8_t*) &data[0], 12 ) % numBuckets;
    }

    int AddVertex( const vertex_t & vertex, float grid = 0.1f, float epsilon = 0.01f )
    {
        const float epsilonSquared = epsilon * epsilon;
        
        const float inverseGrid = 1.0f / grid;

        int x = (int) floor( vertex.position.x() * inverseGrid );
        int y = (int) floor( vertex.position.y() * inverseGrid );
        int z = (int) floor( vertex.position.z() * inverseGrid );

        int bucketIndex = GetGridCellBucket( x, y, z );

        std::list<index_t> & bucket = buckets[bucketIndex];

        int index = -1;

        for ( typename std::list<index_t>::iterator itor = bucket.begin(); itor != bucket.end(); ++itor )
        {
            const index_t i = *itor;
            const vertex_t & v = vertexBuffer[i];
            vec3f dp = v.position - vertex.position;
            vec3f dn = v.normal - vertex.normal;
            if ( length_squared( dp ) < epsilonSquared &&
                 length_squared( dn ) < epsilonSquared )
            {
                index = i;
                break;
            }
        }

        if ( index != -1 )
            return index;

        vertexBuffer.push_back( vertex );

        index = vertexBuffer.size() - 1;

        const float vx = vertex.position.x();
        const float vy = vertex.position.y();
        const float vz = vertex.position.z();

        // todo: this code is dumb. christer says it is faster to test
        // against adjacent cells vs. adding to multiple buckets here
        
        for ( int ix = -1; ix <= 1; ++ix )
        {
            for ( int iy = -1; iy <= 1; ++iy )
            {
                for ( int iz = -1; iz <= 1; ++iz )
                {
                    const float x1 = grid * ( x + ix ) - epsilon;
                    const float y1 = grid * ( y + iy ) - epsilon;
                    const float z1 = grid * ( z + iz ) - epsilon;

                    const float x2 = grid * ( x + ix + 1 ) + epsilon;
                    const float y2 = grid * ( y + iy + 1 ) + epsilon;
                    const float z2 = grid * ( z + iz + 1 ) + epsilon;

                    if ( vx >= x1 && vx <= x2 &&
                         vy >= y1 && vy <= y2 &&
                         vz >= z1 && vz <= z2 )
                    {
                        buckets[ GetGridCellBucket( x + ix, y + iy, z + iz ) ].push_back( index );
                    }
                }
            }
        }

        return index;
    }

private:

    std::vector<vertex_t> vertexBuffer;
    
    std::vector<index_t> indexBuffer;

    int numBuckets;
    std::list<index_t> * buckets;
};

extern bool WriteObjFile( Mesh<Vertex> & mesh, const char filename[] );

extern bool WriteMeshFile( Mesh<Vertex> & mesh, const char filename[] );

#endif
