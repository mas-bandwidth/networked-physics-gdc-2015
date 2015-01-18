#ifndef GAME_SNAPSHOT_H
#define GAME_SNAPSHOT_H

#include <float.h>
#include <stdlib.h>
#include "Cubes.h"
#include "vectorial/vec3f.h"
#include "vectorial/quat4f.h"
#include "protocol/Stream.h"
#include "protocol/SequenceBuffer.h"

//#define SERIALIZE_ANGULAR_VELOCITY

static const int MaxPacketSize = 64 * 1024;         // this has to be really large for the worst case!

static const int NumCubes = 900 + MaxPlayers;

static const int NumSnapshots = 64;

enum SnapshotInterpolation
{
    SNAPSHOT_INTERPOLATION_NONE,
    SNAPSHOT_INTERPOLATION_LINEAR,
    SNAPSHOT_INTERPOLATION_HERMITE,
    SNAPSHOT_INTERPOLATION_HERMITE_WITH_EXTRAPOLATION
};

struct CubeState
{
    bool interacting;
    vectorial::vec3f position;
    vectorial::quat4f orientation;
    vectorial::vec3f linear_velocity;
#ifdef SERIALIZE_ANGULAR_VELOCITY
    vectorial::vec3f angular_velocity;
#endif // #ifdef SERIALIZE_ANGULAR_VELOCITY
};

template <typename Stream> inline void serialize_vector( Stream & stream, vectorial::vec3f & vector )
{
    float values[3];
    if ( Stream::IsWriting )
        vector.store( values );
    serialize_float( stream, values[0] );
    serialize_float( stream, values[1] );
    serialize_float( stream, values[2] );
    if ( Stream::IsReading )
        vector.load( values );
}

template <typename Stream> inline void serialize_compressed_vector( Stream & stream, vectorial::vec3f & vector, float maximum, float resolution )
{
    float values[3];
    if ( Stream::IsWriting )
        vector.store( values );
    serialize_compressed_float( stream, values[0], -maximum, maximum, resolution );
    serialize_compressed_float( stream, values[1], -maximum, maximum, resolution );
    serialize_compressed_float( stream, values[2], -maximum, maximum, resolution );
    if ( Stream::IsReading )
        vector.load( values );
}

template <typename Stream> inline void serialize_compressed_vector( Stream & stream, vectorial::vec3f & vector, const vectorial::vec3f & minimum, const vectorial::vec3f & maximum, float resolution )
{
    float values[3];
    if ( Stream::IsWriting )
        vector.store( values );
    serialize_compressed_float( stream, values[0], minimum.x(), maximum.x(), resolution );
    serialize_compressed_float( stream, values[1], minimum.y(), maximum.y(), resolution );
    serialize_compressed_float( stream, values[2], minimum.z(), maximum.z(), resolution );
    if ( Stream::IsReading )
        vector.load( values );
}

template <typename Stream> inline void serialize_quaternion( Stream & stream, vectorial::quat4f & quaternion )
{
    float values[4];
    if ( Stream::IsWriting )
        quaternion.store( values );
    serialize_float( stream, values[0] );
    serialize_float( stream, values[1] );
    serialize_float( stream, values[2] );
    serialize_float( stream, values[3] );
    if ( Stream::IsReading )
        quaternion.load( values );
}

template <typename Stream> inline void serialize_compressed_quaternion( Stream & stream, vectorial::quat4f & quaternion, int component_bits )
{
    CORE_ASSERT( component_bits > 1 );
    CORE_ASSERT( component_bits <= 31 );

    const float minimum = - 1.0f / 1.414214f;       // 1.0f / sqrt(2)
    const float maximum = + 1.0f / 1.414214f;

    const float scale = float( ( 1 << component_bits ) - 1 );

    if ( Stream::IsWriting )
    {
        float values[4];
        quaternion.store( values );

        const float x = values[0];
        const float y = values[1];
        const float z = values[2];
        const float w = values[3];

        const float abs_x = fabs( x );
        const float abs_y = fabs( y );
        const float abs_z = fabs( z );
        const float abs_w = fabs( w );

        float largest_value = abs_x;

        uint32_t largest = 0;

        float a,b,c;

        if ( abs_y > largest_value )
        {
            largest = 1;
            largest_value = abs_y;
        }

        if ( abs_z > largest_value )
        {
            largest = 2;
            largest_value = abs_z;
        }

        if ( abs_w > largest_value )
        {
            largest = 3;
            largest_value = abs_w;
        }

        switch ( largest )
        {
            case 0:
                if ( x >= 0 )
                {
                    a = y;
                    b = z;
                    c = w;
                }
                else
                {
                    a = -y;
                    b = -z;
                    c = -w;
                }
                break;

            case 1:
                if ( y >= 0 )
                {
                    a = x;
                    b = z;
                    c = w;
                }
                else
                {
                    a = -x;
                    b = -z;
                    c = -w;
                }
                break;

            case 2:
                if ( z >= 0 )
                {
                    a = x;
                    b = y;
                    c = w;
                }
                else
                {
                    a = -x;
                    b = -y;
                    c = -w;
                }
                break;

            case 3:
                if ( w >= 0 )
                {
                    a = x;
                    b = y;
                    c = z;
                }
                else
                {
                    a = -x;
                    b = -y;
                    c = -z;
                }
                break;

            default:
                assert( false );
        }

        const float normal_a = ( a - minimum ) / ( maximum - minimum ); 
        const float normal_b = ( b - minimum ) / ( maximum - minimum );
        const float normal_c = ( c - minimum ) / ( maximum - minimum );

        uint32_t integer_a = math::floor( normal_a * scale + 0.5f );
        uint32_t integer_b = math::floor( normal_b * scale + 0.5f );
        uint32_t integer_c = math::floor( normal_c * scale + 0.5f );

        serialize_bits( stream, largest, 2 );
        serialize_bits( stream, integer_a, component_bits );
        serialize_bits( stream, integer_b, component_bits );
        serialize_bits( stream, integer_c, component_bits );
    }
    else
    {
        uint32_t largest;
        uint32_t integer_a;
        uint32_t integer_b;
        uint32_t integer_c;

        serialize_bits( stream, largest, 2 );
        serialize_bits( stream, integer_a, component_bits );
        serialize_bits( stream, integer_b, component_bits );
        serialize_bits( stream, integer_c, component_bits );

        const float minimum = - 1.0f / 1.414214f;       // note: 1.0f / sqrt(2)
        const float maximum = + 1.0f / 1.414214f;

        const float inverse_scale = 1.0f / scale;

        const float a = integer_a * inverse_scale * ( maximum - minimum ) + minimum;
        const float b = integer_b * inverse_scale * ( maximum - minimum ) + minimum;
        const float c = integer_c * inverse_scale * ( maximum - minimum ) + minimum;

        switch ( largest )
        {
            case 0:
            {
                // (?) y z w

                quaternion = vectorial::normalize( vectorial::quat4f( sqrtf( 1 - a*a - b*b - c*c ), a, b, c ) );
            }
            break;

            case 1:
            {
                // x (?) z w

                quaternion = vectorial::normalize( vectorial::quat4f( a, sqrtf( 1 - a*a - b*b - c*c ), b, c ) );
            }
            break;

            case 2:
            {
                // x y (?) w

                quaternion = vectorial::normalize( vectorial::quat4f( a, b, sqrtf( 1 - a*a - b*b - c*c ), c ) );
            }
            break;

            case 3:
            {
                // x y z (?)

                quaternion = vectorial::normalize( vectorial::quat4f( a, b, c, sqrtf( 1 - a*a - b*b - c*c ) ) );
            }
            break;

            default:
            {
                assert( false );
                quaternion = vectorial::quat4f::identity();
            }
        }
    }
}

inline void compress_orientation( const vectorial::quat4f & orientation, uint32_t & compressed_orientation )
{
    uint32_t largest = 0;
    float a,b,c;
    a = 0;
    b = 0;
    c = 0;

    const float w = orientation.w();
    const float x = orientation.x();
    const float y = orientation.y();
    const float z = orientation.z();

    #ifdef DEBUG
    const float epsilon = 0.0001f;
    const float length_squared = vectorial::length_squared( orientation );
    assert( length_squared >= 1.0f - epsilon && length_squared <= 1.0f + epsilon );
    #endif

    const float abs_w = fabs( w );
    const float abs_x = fabs( x );
    const float abs_y = fabs( y );
    const float abs_z = fabs( z );

    float largest_value = abs_x;

    if ( abs_y > largest_value )
    {
        largest = 1;
        largest_value = abs_y;
    }

    if ( abs_z > largest_value )
    {
        largest = 2;
        largest_value = abs_z;
    }

    if ( abs_w > largest_value )
    {
        largest = 3;
        largest_value = abs_w;
    }

    switch ( largest )
    {
        case 0:
            if ( x >= 0 )
            {
                a = y;
                b = z;
                c = w;
            }
            else
            {
                a = -y;
                b = -z;
                c = -w;
            }
            break;

        case 1:
            if ( y >= 0 )
            {
                a = x;
                b = z;
                c = w;
            }
            else
            {
                a = -x;
                b = -z;
                c = -w;
            }
            break;

        case 2:
            if ( z >= 0 )
            {
                a = x;
                b = y;
                c = w;
            }
            else
            {
                a = -x;
                b = -y;
                c = -w;
            }
            break;

        case 3:
            if ( w >= 0 )
            {
                a = x;
                b = y;
                c = z;
            }
            else
            {
                a = -x;
                b = -y;
                c = -z;
            }
            break;

        default:
            assert( false );
    }

    const float minimum = - 1.0f / 1.414214f;       // 1.0f / sqrt(2)
    const float maximum = + 1.0f / 1.414214f;

    const float normal_a = ( a - minimum ) / ( maximum - minimum ); 
    const float normal_b = ( b - minimum ) / ( maximum - minimum );
    const float normal_c = ( c - minimum ) / ( maximum - minimum );

    // todo: convert to 8 bits per-component. saves 6 bits per-orientation. probably enough res!

    uint32_t integer_a = math::floor( normal_a * 1024.0f + 0.5f );
    uint32_t integer_b = math::floor( normal_b * 1024.0f + 0.5f );
    uint32_t integer_c = math::floor( normal_c * 1024.0f + 0.5f );

    compressed_orientation = ( largest << 30 ) | ( integer_a << 20 ) | ( integer_b << 10 ) | integer_c;
}

inline void decompress_orientation( uint32_t compressed_orientation, vectorial::quat4f & orientation )
{
    uint32_t largest = compressed_orientation >> 30;
    uint32_t integer_a = ( compressed_orientation >> 20 ) & ( (1<<10) - 1 );
    uint32_t integer_b = ( compressed_orientation >> 10 ) & ( (1<<10) - 1 );
    uint32_t integer_c = ( compressed_orientation ) & ( (1<<10) - 1 );

    const float minimum = - 1.0f / 1.414214f;       // note: 1.0f / sqrt(2)
    const float maximum = + 1.0f / 1.414214f;

    const float a = integer_a / 1024.0f * ( maximum - minimum ) + minimum;
    const float b = integer_b / 1024.0f * ( maximum - minimum ) + minimum;
    const float c = integer_c / 1024.0f * ( maximum - minimum ) + minimum;

    switch ( largest )
    {
        case 0:
        {
            // (?) y z w

            orientation = vectorial::quat4f( sqrtf( 1 - a*a - b*b - c*c ), a, b, c );
        }
        break;

        case 1:
        {
            // x (?) z w

            orientation = vectorial::quat4f( a, sqrtf( 1 - a*a - b*b - c*c ), b, c );
        }
        break;

        case 2:
        {
            // x y (?) w

            orientation = vectorial::quat4f( a, b, sqrtf( 1 - a*a - b*b - c*c ), c );
        }
        break;

        case 3:
        {
            // x y z (?)

            orientation = vectorial::quat4f( a, b, c, sqrtf( 1 - a*a - b*b - c*c ) );
        }
        break;

        default:
        {
            assert( false );
            orientation = vectorial::quat4f::identity();
        }
    }

    orientation = vectorial::normalize( orientation );
}


static void InterpolateSnapshot_Linear( float t, 
                                        const __restrict CubeState * a, 
                                        const __restrict CubeState * b, 
                                        __restrict view::ObjectUpdate * output )
{
    for ( int i = 0; i < NumCubes; ++i )
    {
        output[i].id = i + 1;
        output[i].position = a[i].position + ( b[i].position - a[i].position ) * t;
        output[i].orientation = vectorial::slerp( t, a[i].orientation, b[i].orientation );
        output[i].scale = ( i == 0 ) ? hypercube::PlayerCubeSize : hypercube::NonPlayerCubeSize;
        output[i].authority = a[i].interacting ? 0 : MaxPlayers;
        output[i].visible = true;
    }
}

inline void hermite_spline( float t,
                            const vectorial::vec3f & p0, 
                            const vectorial::vec3f & p1,
                            const vectorial::vec3f & t0,
                            const vectorial::vec3f & t1,
                            vectorial::vec3f & output )
{
    const float t2 = t*t;
    const float t3 = t2*t;
    const float h1 =  2*t3 - 3*t2 + 1;
    const float h2 = -2*t3 + 3*t2;
    const float h3 =    t3 - 2*t2 + t;
    const float h4 =    t3 - t2;
    output = h1*p0 + h2*p1 + h3*t0 + h4*t1;
}

static void InterpolateSnapshot_Hermite( float t, 
                                         float step_size,
                                         const __restrict CubeState * a, 
                                         const __restrict CubeState * b, 
                                         __restrict view::ObjectUpdate * output )
{
    for ( int i = 0; i < NumCubes; ++i )
    {
        hermite_spline( t, a[i].position, b[i].position, a[i].linear_velocity * step_size, b[i].linear_velocity * step_size, output[i].position );

        output[i].orientation = vectorial::slerp( t, a[i].orientation, b[i].orientation );

        output[i].id = i + 1;
        output[i].scale = ( i == 0 ) ? hypercube::PlayerCubeSize : hypercube::NonPlayerCubeSize;
        output[i].authority = a[i].interacting ? 0 : MaxPlayers;
        output[i].visible = true;
    }
}

static void InterpolateSnapshot_Hermite_WithExtrapolation( float t, 
                                                           float step_size,
                                                           float extrapolation,
                                                           const __restrict CubeState * a, 
                                                           const __restrict CubeState * b, 
                                                           __restrict view::ObjectUpdate * output )
{
    for ( int i = 0; i < NumCubes; ++i )
    {
        vectorial::vec3f p0 = a[i].position + a[i].linear_velocity * extrapolation;
        vectorial::vec3f p1 = b[i].position + b[i].linear_velocity * extrapolation;
        vectorial::vec3f t0 = a[i].linear_velocity * step_size;
        vectorial::vec3f t1 = b[i].linear_velocity * step_size;
        hermite_spline( t, p0, p1, t0, t1, output[i].position );

        output[i].orientation = vectorial::slerp( t, a[i].orientation, b[i].orientation );

        output[i].id = i + 1;
        output[i].scale = ( i == 0 ) ? hypercube::PlayerCubeSize : hypercube::NonPlayerCubeSize;
        output[i].authority = a[i].interacting ? 0 : MaxPlayers;
        output[i].visible = true;
    }
}

struct Snapshot
{
    CubeState cubes[NumCubes];
};

struct SnapshotModeData
{
    float playout_delay = 0.35f;        // one lost packet = no problem. two lost packets in a row = hitch
    float send_rate = 60.0f;
    float latency = 0.0f;
    float packet_loss = 0.0f;
    float jitter = 0.0f;
    float extrapolation = 0.2f;
    SnapshotInterpolation interpolation = SNAPSHOT_INTERPOLATION_NONE;
};

struct SnapshotInterpolationBuffer
{
    SnapshotInterpolationBuffer( core::Allocator & allocator, const SnapshotModeData & mode_data )
        : snapshots( allocator, NumSnapshots )
    {
        stopped = true;
        interpolating = false;
        start_time = 0.0;
        playout_delay = mode_data.playout_delay;
    }

    void AddSnapshot( double time, uint16_t sequence, const CubeState * cube_state )
    {
        CORE_ASSERT( cube_state > 0 );

        if ( stopped )
        {
            start_time = time;
            stopped = false;
        }

        auto entry = snapshots.Insert( sequence );

        if ( entry )
            memcpy( entry->cubes, cube_state, sizeof( entry->cubes ) );
    }

    void GetViewUpdate( const SnapshotModeData & mode_data, double time, view::ObjectUpdate * object_update, int & num_object_updates )
    {
        num_object_updates = 0;

        // we have not received a packet yet. nothing to display!

        if ( stopped )
            return;

        // if time minus playout delay is negative, it's too early to display anything

        time -= ( start_time + playout_delay );

        if ( time <= 0 )
            return;

        // if we are interpolating but the interpolation start time is too old,
        // go back to the not interpolating state, so we can find a new start point.

        const double frames_since_start = time * mode_data.send_rate;

        if ( interpolating )
        {
            const int n = (int) floor( mode_data.playout_delay / ( 1.0f / mode_data.send_rate ) );

            uint16_t interpolation_sequence = (uint16_t) uint64_t( floor( frames_since_start ) );

            if ( core::sequence_difference( interpolation_sequence, interpolation_start_sequence ) > n )
                interpolating = false;
        }

        // if not interpolating, attempt to find an interpolation start point. 
        // if start point exists, go into interpolating mode and set end point to start point
        // so we can reuse code below to find a suitable end point on first time through.
        // if no interpolation start point is found, return.

        if ( !interpolating )
        {
            uint16_t interpolation_sequence = (uint16_t) uint64_t( floor( frames_since_start ) );

            auto snapshot = snapshots.Find( interpolation_sequence );

            if ( snapshot )
            {
                interpolation_start_sequence = interpolation_sequence;
                interpolation_end_sequence = interpolation_sequence;

                interpolation_start_time = frames_since_start * ( 1.0 / mode_data.send_rate );
                interpolation_end_time = interpolation_start_time;

                interpolating = true;
            }
        }

        if ( !interpolating )
            return;

        if ( time < interpolation_start_time )
            time = interpolation_start_time;

        CORE_ASSERT( time >= interpolation_start_time );

        // if current time is >= end time, we need to start a new interpolation
        // from the previous end time to the next sample that exists up to n samples
        // ahead, where n is the # of frames in the playout delay buffer, rounded up.

        if ( time >= interpolation_end_time )
        {
            const int n = (int) floor( mode_data.playout_delay / ( 1.0f / mode_data.send_rate ) );

            interpolation_start_sequence = interpolation_end_sequence;
            interpolation_start_time = interpolation_end_time;

            for ( int i = 0; i < n; ++i )
            {
                auto snapshot = snapshots.Find( interpolation_start_sequence + 1 + i );

                if ( snapshot )
                {
                    interpolation_end_sequence = interpolation_start_sequence + 1 + i;
                    interpolation_end_time = interpolation_start_time + ( 1.0 / mode_data.send_rate ) * ( 1 + i );
                    interpolation_step_size = ( 1.0 / mode_data.send_rate ) * ( 1 + i );
                    break;
                }
            }
        }

        // if current time is still > end time, we couldn't start a new interpolation so return.

        if ( time >= interpolation_end_time )
            return;

        // we are in a valid interpolation, calculate t by looking at current time 
        // relative to interpolation start/end times and perform the interpolation.

        CORE_ASSERT( time >= interpolation_start_time );
        CORE_ASSERT( time <= interpolation_end_time );
        CORE_ASSERT( interpolation_start_time < interpolation_end_time );

        const float t = ( time - interpolation_start_time ) / ( interpolation_end_time - interpolation_start_time );

        CORE_ASSERT( t >= 0 );
        CORE_ASSERT( t <= 1 );

        auto snapshot_a = snapshots.Find( interpolation_start_sequence );
        auto snapshot_b = snapshots.Find( interpolation_end_sequence );

        CORE_ASSERT( snapshot_a );
        CORE_ASSERT( snapshot_b );

        if ( mode_data.interpolation == SNAPSHOT_INTERPOLATION_LINEAR )
        {
            InterpolateSnapshot_Linear( t, snapshot_a->cubes, snapshot_b->cubes, object_update );            
            num_object_updates = NumCubes;
        }
        else if ( mode_data.interpolation == SNAPSHOT_INTERPOLATION_HERMITE )
        {
            InterpolateSnapshot_Hermite( t, interpolation_step_size, snapshot_a->cubes, snapshot_b->cubes, object_update );                
            num_object_updates = NumCubes;
        }
        else if ( mode_data.interpolation == SNAPSHOT_INTERPOLATION_HERMITE_WITH_EXTRAPOLATION )
        {
            InterpolateSnapshot_Hermite_WithExtrapolation( t, interpolation_step_size, mode_data.extrapolation, snapshot_a->cubes, snapshot_b->cubes, object_update );
            num_object_updates = NumCubes;
        }
        else
        {
            CORE_ASSERT( false );
        }
    }

    void Reset()
    {
        stopped = true;
        interpolating = false;
        interpolation_start_sequence = 0;
        interpolation_start_time = 0.0;
        interpolation_end_sequence = 0;
        interpolation_end_time = 0.0;
        interpolation_step_size = 0.0;
        start_time = 0.0;
        snapshots.Reset();
    }

    bool stopped;
    bool interpolating;
    uint16_t interpolation_start_sequence;
    uint16_t interpolation_end_sequence;
    double interpolation_start_time;
    double interpolation_end_time;
    double interpolation_step_size;
    double start_time;
    float playout_delay;
    protocol::SequenceBuffer<Snapshot> snapshots;
};

#endif // #ifndef GAME_SNAPSHOT_H
