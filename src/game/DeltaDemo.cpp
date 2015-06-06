#include "DeltaDemo.h"

#ifdef CLIENT

#include "stdlib.h"
#include "Cubes.h"
#include "Global.h"
#include "Snapshot.h"
#include "Font.h"
#include "FontManager.h"
#include "protocol/Stream.h"
#include "protocol/SlidingWindow.h"
#include "protocol/SequenceBuffer.h"
#include "protocol/PacketFactory.h"
#include "network/Simulator.h"

static const int LeftPort = 1000;
static const int RightPort = 1001;
static const int MaxSnapshots = 256;
static const int MaxPacketSize = 64 * 1024;         // this has to be really large for the worst case!

#if DELTA_DATA

struct DeltaData
{
    float orientation_x;
    float orientation_y;
    float orientation_z;
    float orientation_w;
    float position_x;
    float position_y;
    float position_z;
};

static FILE * delta_data = nullptr;

#endif // #if DELTA_DATA

#if DELTA_STATS

static const int MaxPositionDelta = 1024;
static const int MaxSmallestThreeDelta = 1024;
static const int MaxAxisDelta = 1024;
static const int MaxAngleDelta = 1024;
static const int MaxAxisAngleDelta = 1024;
static const int MaxQuaternionDelta = 1024;
static const int MaxRelativeQuaternionDelta = 1024;

static uint64_t delta_position_accum_x[MaxPositionDelta];
static uint64_t delta_position_accum_y[MaxPositionDelta];
static uint64_t delta_position_accum_z[MaxPositionDelta];

static uint64_t delta_smallest_three_accum_a[MaxSmallestThreeDelta];
static uint64_t delta_smallest_three_accum_b[MaxSmallestThreeDelta];
static uint64_t delta_smallest_three_accum_c[MaxSmallestThreeDelta];

static uint64_t delta_quaternion_accum_x[MaxQuaternionDelta];
static uint64_t delta_quaternion_accum_y[MaxQuaternionDelta];
static uint64_t delta_quaternion_accum_z[MaxQuaternionDelta];
static uint64_t delta_quaternion_accum_w[MaxQuaternionDelta];

static uint64_t delta_angle_accum[MaxAngleDelta];

static uint64_t delta_axis_accum_x[MaxAxisDelta];
static uint64_t delta_axis_accum_y[MaxAxisDelta];
static uint64_t delta_axis_accum_z[MaxAxisDelta];

static uint64_t delta_axis_angle_accum_x[MaxAxisAngleDelta];
static uint64_t delta_axis_angle_accum_y[MaxAxisAngleDelta];
static uint64_t delta_axis_angle_accum_z[MaxAxisAngleDelta];

static uint64_t delta_relative_quaternion_accum_x[MaxRelativeQuaternionDelta];
static uint64_t delta_relative_quaternion_accum_y[MaxRelativeQuaternionDelta];
static uint64_t delta_relative_quaternion_accum_z[MaxRelativeQuaternionDelta];
static uint64_t delta_relative_quaternion_accum_w[MaxRelativeQuaternionDelta];

static FILE * position_values = nullptr;
static FILE * quaternion_values = nullptr;
static FILE * quaternion_float_values = nullptr;
static FILE * relative_quaternion_values = nullptr;
static FILE * smallest_three_values = nullptr;
static FILE * axis_angle_values = nullptr;

#endif // #ifdef DELTA_STATS

enum Context
{
    CONTEXT_QUANTIZED_SNAPSHOT_SLIDING_WINDOW,      // quantized send snapshots (for serialize write)
    CONTEXT_QUANTIZED_SNAPSHOT_SEQUENCE_BUFFER,     // quantized recv snapshots (for serialize read)
    CONTEXT_QUANTIZED_INITIAL_SNAPSHOT              // quantized initial snapshot
};

enum DeltaMode
{
    DELTA_MODE_NOT_CHANGED,
    DELTA_MODE_CHANGED_INDEX,
    DELTA_MODE_RELATIVE_INDEX,
    DELTA_MODE_RELATIVE_POSITION,
    DELTA_MODE_RELATIVE_ORIENTATION,
    DELTA_NUM_MODES
};

const char * delta_mode_descriptions[]
{
    "Not changed",
    "Changed index",
    "Relative index",
    "Relative position",
    "Relative orientation",
};

struct DeltaModeData : public SnapshotModeData
{
    DeltaModeData()
    {
        playout_delay = 0.05f;
        send_rate = 60.0f;
        latency = 0.005f;      // 100ms round trip -- IMPORTANT! Otherwise delta compression is too easy!
#if DELTA_DATA
        packet_loss = 0.0f;
        jitter = 0.0f;
#else
        packet_loss = 5.0f;
        jitter = 1.0 / 60.0f;
#endif
        interpolation = SNAPSHOT_INTERPOLATION_LINEAR;
    }
};

static DeltaModeData delta_mode_data[DELTA_NUM_MODES];

static void InitDeltaModes()
{
    // ...
}

typedef protocol::SlidingWindow<QuantizedSnapshot> QuantizedSnapshotSlidingWindow;
typedef protocol::SequenceBuffer<QuantizedSnapshot> QuantizedSnapshotSequenceBuffer;

enum DeltaPackets
{
    DELTA_SNAPSHOT_PACKET,
    DELTA_ACK_PACKET,
    DELTA_NUM_PACKETS
};

template <typename Stream> void serialize_cube_changed( Stream & stream, QuantizedCubeState & cube, const QuantizedCubeState & base )
{
    serialize_bool( stream, cube.interacting );

    bool position_changed;
    bool orientation_changed;

    if ( Stream::IsWriting )
    {
        position_changed = cube.position_x != base.position_x || cube.position_y != base.position_y || cube.position_z != base.position_z;
        orientation_changed = cube.orientation != base.orientation;
    }

    serialize_bool( stream, position_changed );
    serialize_bool( stream, orientation_changed );

    if ( position_changed )
    {
        serialize_int( stream, cube.position_x, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
        serialize_int( stream, cube.position_y, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
        serialize_int( stream, cube.position_z, 0, +QuantizedPositionBoundZ - 1 );
    }
    else
    {
        cube.position_x = base.position_x;
        cube.position_y = base.position_y;
        cube.position_z = base.position_z;
    }

    if ( orientation_changed )
        serialize_object( stream, cube.orientation );
    else
        cube.orientation = base.orientation;
}

template <typename Stream> void serialize_offset( Stream & stream, int & offset, int small_bound, int large_bound )
{
    if ( Stream::IsWriting )
    {
        CORE_ASSERT( offset >= small_bound - 1 || offset <= - small_bound );

        if ( offset > 0 )
        {   
            offset -= small_bound - 1;
        }
        else
        {
            offset += small_bound - 1;
            CORE_ASSERT( offset < 0 );                        // note: otherwise two offset values end up sharing the zero value
        }

        CORE_ASSERT( offset >= -large_bound );
        CORE_ASSERT( offset <= +large_bound - 1 );
    }

    serialize_int( stream, 
                   offset, 
                  -large_bound,
                   large_bound - 1 );

    if ( Stream::IsReading )
    {
        if ( offset >= 0 )
            offset += small_bound - 1;
        else
            offset -= small_bound - 1;
    }
}

template <typename Stream> void serialize_relative_position( Stream & stream,
                                                             int & position_x,
                                                             int & position_y,
                                                             int & position_z,
                                                             int base_position_x,
                                                             int base_position_y,
                                                             int base_position_z )
{
    const int RelativePositionBound_Small = 16;
    const int RelativePositionBound_Large = 256;

    bool relative_position = false;
    bool relative_position_small_x = false;
    bool relative_position_small_y = false;
    bool relative_position_small_z = false;

    if ( Stream::IsWriting )
    {
        const int dx = position_x - base_position_x;
        const int dy = position_y - base_position_y;
        const int dz = position_z - base_position_z;

        const int relative_min = -RelativePositionBound_Large - ( RelativePositionBound_Small - 1 );        // -256 - 15 = -271
        const int relative_max =  RelativePositionBound_Large - 1 + ( RelativePositionBound_Small - 1 );    // +255 + 15 = 270

        relative_position = dx >= relative_min && dx <= relative_max &&
                            dy >= relative_min && dy <= relative_max &&
                            dz >= relative_min && dz <= relative_max;

        if ( relative_position )
        {
            relative_position_small_x = dx >= -RelativePositionBound_Small && dx < RelativePositionBound_Small;
            relative_position_small_y = dy >= -RelativePositionBound_Small && dy < RelativePositionBound_Small;
            relative_position_small_z = dz >= -RelativePositionBound_Small && dz < RelativePositionBound_Small;
        }
    }

    serialize_bool( stream, relative_position );

    if ( relative_position )
    {
        serialize_bool( stream, relative_position_small_x );
        serialize_bool( stream, relative_position_small_y );
        serialize_bool( stream, relative_position_small_z );

        int offset_x, offset_y, offset_z;

        if ( Stream::IsWriting )
        {
            offset_x = position_x - base_position_x;
            offset_y = position_y - base_position_y;
            offset_z = position_z - base_position_z;
        }

        if ( relative_position_small_x )
        {
            serialize_int( stream, offset_x, -RelativePositionBound_Small, RelativePositionBound_Small - 1 );
        }
        else
        {
            serialize_offset( stream, offset_x, RelativePositionBound_Small, RelativePositionBound_Large );
        }

        if ( relative_position_small_y )
        {
            serialize_int( stream, offset_y, -RelativePositionBound_Small, RelativePositionBound_Small - 1 );
        }
        else
        {
            serialize_offset( stream, offset_y, RelativePositionBound_Small, RelativePositionBound_Large );
        }

        if ( relative_position_small_z )
        {
            serialize_int( stream, offset_z, -RelativePositionBound_Small, RelativePositionBound_Small - 1 );
        }
        else
        {
            serialize_offset( stream, offset_z, RelativePositionBound_Small, RelativePositionBound_Large );
        }

        if ( Stream::IsReading )
        {
            position_x = base_position_x + offset_x;
            position_y = base_position_y + offset_y;
            position_z = base_position_z + offset_z;
        }
    }
    else
    {
        serialize_int( stream, position_x, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
        serialize_int( stream, position_y, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
        serialize_int( stream, position_z, 0, +QuantizedPositionBoundZ - 1 );
    }
}

template <typename Stream> void serialize_cube_relative_position( Stream & stream, QuantizedCubeState & cube, const QuantizedCubeState & base )
{
    serialize_bool( stream, cube.interacting );

    bool position_changed;
    bool orientation_changed;

    if ( Stream::IsWriting )
    {
        position_changed = cube.position_x != base.position_x || cube.position_y != base.position_y || cube.position_z != base.position_z;
        orientation_changed = cube.orientation != base.orientation;
    }

    serialize_bool( stream, position_changed );
    serialize_bool( stream, orientation_changed );

    if ( position_changed )
    {
        serialize_relative_position( stream, cube.position_x, cube.position_y, cube.position_z, base.position_x, base.position_y, base.position_z );
    }
    else if ( Stream::IsReading )
    {
        cube.position_x = base.position_x;
        cube.position_y = base.position_y;
        cube.position_z = base.position_z;
    }

    if ( orientation_changed )
    {
        serialize_object( stream, cube.orientation );
    }
    else
    {
        cube.orientation = base.orientation;
    }
}

template <typename Stream> void serialize_relative_orientation( Stream & stream, compressed_quaternion<9> & orientation, const compressed_quaternion<9> & base_orientation )
{
    const int RelativeOrientationBound_Small = 16;
    const int RelativeOrientationBound_Large = 128;

    bool relative_orientation = false;
    bool small_a = false;
    bool small_b = false;
    bool small_c = false;

    if ( Stream::IsWriting )
    {
        const int da = orientation.integer_a - base_orientation.integer_a;
        const int db = orientation.integer_b - base_orientation.integer_b;
        const int dc = orientation.integer_c - base_orientation.integer_c;

        const int relative_min = -RelativeOrientationBound_Large - ( RelativeOrientationBound_Small - 1 );        // -256 - 15 = -271
        const int relative_max =  RelativeOrientationBound_Large - 1 + ( RelativeOrientationBound_Small - 1 );    // +255 + 15 = 270

        if ( orientation.largest == base_orientation.largest &&
             da >= relative_min && da < relative_max &&
             db >= relative_min && db < relative_max &&
             dc >= relative_min && dc < relative_max )
        {
            relative_orientation = true;

            small_a = da >= -RelativeOrientationBound_Small && da < RelativeOrientationBound_Small;
            small_b = db >= -RelativeOrientationBound_Small && db < RelativeOrientationBound_Small;
            small_c = dc >= -RelativeOrientationBound_Small && dc < RelativeOrientationBound_Small;
        }
    }

    serialize_bool( stream, relative_orientation );

    if ( relative_orientation )
    {
        serialize_bool( stream, small_a );
        serialize_bool( stream, small_b );
        serialize_bool( stream, small_c );

        int offset_a, offset_b, offset_c;

        if ( Stream::IsWriting )
        {
            offset_a = orientation.integer_a - base_orientation.integer_a;
            offset_b = orientation.integer_b - base_orientation.integer_b;
            offset_c = orientation.integer_c - base_orientation.integer_c;
        }

        if ( small_a )
        {
            serialize_int( stream, offset_a, -RelativeOrientationBound_Small, RelativeOrientationBound_Small - 1 );
        }
        else
        {
            serialize_offset( stream, offset_a, RelativeOrientationBound_Small, RelativeOrientationBound_Large );
        }

        if ( small_b )
        {
            serialize_int( stream, offset_b, -RelativeOrientationBound_Small, RelativeOrientationBound_Small - 1 );
        }
        else
        {
            serialize_offset( stream, offset_b, RelativeOrientationBound_Small, RelativeOrientationBound_Large );
        }

        if ( small_c )
        {
            serialize_int( stream, offset_c, -RelativeOrientationBound_Small, RelativeOrientationBound_Small - 1 );
        }
        else
        {
            serialize_offset( stream, offset_c, RelativeOrientationBound_Small, RelativeOrientationBound_Large );
        }

        if ( Stream::IsReading )
        {
            orientation.largest = base_orientation.largest;
            orientation.integer_a = base_orientation.integer_a + offset_a;
            orientation.integer_b = base_orientation.integer_b + offset_b;
            orientation.integer_c = base_orientation.integer_c + offset_c;
        }
    }
    else 
    {
        serialize_object( stream, orientation );
    }
}

template <typename Stream> void serialize_cube_relative_orientation( Stream & stream, QuantizedCubeState & cube, const QuantizedCubeState & base )
{
    serialize_bool( stream, cube.interacting );

    bool position_changed;
    bool orientation_changed;

    if ( Stream::IsWriting )
    {
        position_changed = cube.position_x != base.position_x || cube.position_y != base.position_y || cube.position_z != base.position_z;
        orientation_changed = cube.orientation != base.orientation;
    }

    serialize_bool( stream, position_changed );
    serialize_bool( stream, orientation_changed );

    if ( position_changed )
    {
        serialize_relative_position( stream, cube.position_x, cube.position_y, cube.position_z, base.position_x, base.position_y, base.position_z );
    }
    else if ( Stream::IsReading )
    {
        cube.position_x = base.position_x;
        cube.position_y = base.position_y;
        cube.position_z = base.position_z;
    }

    if ( orientation_changed )
    {
        serialize_relative_orientation( stream, cube.orientation, base.orientation );
    }
    else
    {
        cube.orientation = base.orientation;
    }
}

#if DELTA_STATS

void UpdateDeltaStats( const QuantizedCubeState & cube, const QuantizedCubeState & base )
{
    // IMPORTANT: Don't count identical cubes in delta stats. These are already handled by changed flag.
    if ( cube == base )
        return;

    // IMPORTANT: Don't write position values if identical to base. We serialize one bit to handle this case (~5% of "changed" cubes)
    if ( cube.position_x != base.position_x ||
         cube.position_y != base.position_y ||
         cube.position_z != base.position_z )
    {
        fprintf( position_values, "%d,%d,%d,%d,%d,%d\n", 
                 cube.position_x, cube.position_y, cube.position_z,
                 base.position_x, base.position_y, base.position_z );
    }

    // IMPORTANT: Don't write orientation values if identical to base. We serialize one bit to handle this case (~5% of "changed" cubes)
    if ( cube.orientation.largest == base.orientation.largest && 
         cube.orientation.integer_a == base.orientation.integer_a && 
         cube.orientation.integer_b == base.orientation.integer_b && 
         cube.orientation.integer_c == base.orientation.integer_c )
    {
        return;
    }

    fprintf( smallest_three_values, "%d,%d,%d,%d,%d,%d,%d,%d\n", 
        cube.orientation.largest, cube.orientation.integer_a, cube.orientation.integer_b, cube.orientation.integer_c, 
        base.orientation.largest, base.orientation.integer_a, base.orientation.integer_b, base.orientation.integer_c );

    const int position_delta_x = core::clamp( abs( cube.position_x - base.position_x ), 0, MaxPositionDelta - 1 );
    const int position_delta_y = core::clamp( abs( cube.position_y - base.position_y ), 0, MaxPositionDelta - 1 );
    const int position_delta_z = core::clamp( abs( cube.position_z - base.position_z ), 0, MaxPositionDelta - 1 );

    CORE_ASSERT( position_delta_x >= 0 );
    CORE_ASSERT( position_delta_y >= 0 );
    CORE_ASSERT( position_delta_z >= 0 );
    CORE_ASSERT( position_delta_x < MaxPositionDelta );
    CORE_ASSERT( position_delta_y < MaxPositionDelta );
    CORE_ASSERT( position_delta_z < MaxPositionDelta );

    delta_position_accum_x[position_delta_x]++;
    delta_position_accum_y[position_delta_y]++;
    delta_position_accum_z[position_delta_z]++;

    const int smallest_three_delta_a = abs( cube.orientation.integer_a - base.orientation.integer_a );
    const int smallest_three_delta_b = abs( cube.orientation.integer_b - base.orientation.integer_b );
    const int smallest_three_delta_c = abs( cube.orientation.integer_c - base.orientation.integer_c );

    CORE_ASSERT( smallest_three_delta_a >= 0 );
    CORE_ASSERT( smallest_three_delta_b >= 0 );
    CORE_ASSERT( smallest_three_delta_c >= 0 );
    CORE_ASSERT( smallest_three_delta_a < MaxSmallestThreeDelta );
    CORE_ASSERT( smallest_three_delta_b < MaxSmallestThreeDelta );
    CORE_ASSERT( smallest_three_delta_c < MaxSmallestThreeDelta );

    delta_smallest_three_accum_a[smallest_three_delta_a]++;
    delta_smallest_three_accum_b[smallest_three_delta_b]++;
    delta_smallest_three_accum_c[smallest_three_delta_c]++;

    vectorial::quat4f orientation;
    vectorial::quat4f base_orientation;

    cube.orientation.Save( orientation );
    base.orientation.Save( base_orientation );

    if ( vectorial::dot( orientation, base_orientation ) < 0 )
        orientation = -orientation;

    const int quaternion_x = (int) core::clamp( ( orientation.x() + 1.0f ) / 2.0f * ( MaxQuaternionDelta - 1 ) + 0.5f, 0.0f, float( MaxQuaternionDelta - 1 ) );
    const int quaternion_y = (int) core::clamp( ( orientation.y() + 1.0f ) / 2.0f * ( MaxQuaternionDelta - 1 ) + 0.5f, 0.0f, float( MaxQuaternionDelta - 1 ) );
    const int quaternion_z = (int) core::clamp( ( orientation.z() + 1.0f ) / 2.0f * ( MaxQuaternionDelta - 1 ) + 0.5f, 0.0f, float( MaxQuaternionDelta - 1 ) );
    const int quaternion_w = (int) core::clamp( ( orientation.w() + 1.0f ) / 2.0f * ( MaxQuaternionDelta - 1 ) + 0.5f, 0.0f, float( MaxQuaternionDelta - 1 ) );

    const int base_quaternion_x = (int) core::clamp( ( base_orientation.x() + 1.0f ) / 2.0f * ( MaxQuaternionDelta - 1 ) + 0.5f, 0.0f, float( MaxQuaternionDelta - 1 ) );
    const int base_quaternion_y = (int) core::clamp( ( base_orientation.y() + 1.0f ) / 2.0f * ( MaxQuaternionDelta - 1 ) + 0.5f, 0.0f, float( MaxQuaternionDelta - 1 ) );
    const int base_quaternion_z = (int) core::clamp( ( base_orientation.z() + 1.0f ) / 2.0f * ( MaxQuaternionDelta - 1 ) + 0.5f, 0.0f, float( MaxQuaternionDelta - 1 ) );
    const int base_quaternion_w = (int) core::clamp( ( base_orientation.w() + 1.0f ) / 2.0f * ( MaxQuaternionDelta - 1 ) + 0.5f, 0.0f, float( MaxQuaternionDelta - 1 ) );

    fprintf( quaternion_values, "%d,%d,%d,%d,%d,%d,%d,%d\n", 
        quaternion_x, quaternion_y, quaternion_z, quaternion_w, 
        base_quaternion_x, base_quaternion_y, base_quaternion_z, base_quaternion_w );

    fprintf( quaternion_float_values, "%f,%f,%f,%f,%f,%f,%f,%f\n", 
        cube.original_orientation.x(), cube.original_orientation.y(), cube.original_orientation.z(), cube.original_orientation.w(), 
        base.original_orientation.x(), base.original_orientation.y(), base.original_orientation.z(), base.original_orientation.w() );

    const int quaternion_delta_x = abs( quaternion_x - base_quaternion_x );
    const int quaternion_delta_y = abs( quaternion_y - base_quaternion_y );
    const int quaternion_delta_z = abs( quaternion_z - base_quaternion_z );
    const int quaternion_delta_w = abs( quaternion_w - base_quaternion_w );

    CORE_ASSERT( quaternion_delta_x >= 0 );
    CORE_ASSERT( quaternion_delta_y >= 0 );
    CORE_ASSERT( quaternion_delta_z >= 0 );
    CORE_ASSERT( quaternion_delta_w >= 0 );
    CORE_ASSERT( quaternion_delta_x < MaxQuaternionDelta );
    CORE_ASSERT( quaternion_delta_y < MaxQuaternionDelta );
    CORE_ASSERT( quaternion_delta_z < MaxQuaternionDelta );
    CORE_ASSERT( quaternion_delta_w < MaxQuaternionDelta );

    delta_quaternion_accum_x[quaternion_delta_x]++;
    delta_quaternion_accum_y[quaternion_delta_y]++;
    delta_quaternion_accum_z[quaternion_delta_z]++;
    delta_quaternion_accum_w[quaternion_delta_w]++;

    float float_angle, float_base_angle;
    vectorial::vec3f axis, base_axis;
    orientation.to_axis_angle( axis, float_angle );
    base_orientation.to_axis_angle( base_axis, float_base_angle );

    if ( vectorial::dot( axis, base_axis ) < 0 )
    {
        axis = -axis;
        float_angle = -float_angle;
    }

    const float pi = 3.14157f;

    const int angle = (int) floor( float_angle / ( 2 * pi ) * ( MaxAngleDelta - 1 ) + 0.5f );
    const int base_angle = (int) floor( float_base_angle / ( 2 * pi ) * ( MaxAngleDelta - 1 ) + 0.5f );
    const int angle_delta = core::clamp( abs( angle - base_angle ), 0, MaxAngleDelta - 1 );

    CORE_ASSERT( angle_delta >= 0 );
    CORE_ASSERT( angle_delta < MaxAngleDelta );

    delta_angle_accum[angle_delta]++;

    const int axis_x = (int) floor( axis.x() * ( MaxAxisDelta - 1 ) + 0.5f );
    const int axis_y = (int) floor( axis.y() * ( MaxAxisDelta - 1 ) + 0.5f );
    const int axis_z = (int) floor( axis.z() * ( MaxAxisDelta - 1 ) + 0.5f );

    const int base_axis_x = (int) floor( base_axis.x() * ( MaxAxisDelta - 1 ) + 0.5f );
    const int base_axis_y = (int) floor( base_axis.y() * ( MaxAxisDelta - 1 ) + 0.5f );
    const int base_axis_z = (int) floor( base_axis.z() * ( MaxAxisDelta - 1 ) + 0.5f );

    fprintf( axis_angle_values, "%d,%d,%d,%d,%d,%d,%d,%d\n", 
        axis_x, axis_y, axis_z, angle, 
        base_axis_x, base_axis_y, base_axis_z, base_angle );

    const int axis_delta_x = core::clamp( abs( axis_x - base_axis_x ), 0, MaxAxisDelta - 1 );
    const int axis_delta_y = core::clamp( abs( axis_y - base_axis_y ), 0, MaxAxisDelta - 1 );
    const int axis_delta_z = core::clamp( abs( axis_z - base_axis_z ), 0, MaxAxisDelta - 1 );

    CORE_ASSERT( axis_delta_x >= 0 );
    CORE_ASSERT( axis_delta_y >= 0 );
    CORE_ASSERT( axis_delta_z >= 0 );
    CORE_ASSERT( axis_delta_x < MaxAxisDelta );
    CORE_ASSERT( axis_delta_y < MaxAxisDelta );
    CORE_ASSERT( axis_delta_z < MaxAxisDelta );

    delta_axis_accum_x[axis_delta_x]++;
    delta_axis_accum_y[axis_delta_y]++;
    delta_axis_accum_z[axis_delta_z]++;

    vectorial::vec3f axis_angle = axis * angle;
    vectorial::vec3f base_axis_angle = base_axis * base_angle;

    const int axis_angle_x = (int) floor( axis_angle.x() / ( 2 * pi ) * ( MaxAxisAngleDelta - 1 ) + 0.5f );
    const int axis_angle_y = (int) floor( axis_angle.y() / ( 2 * pi ) * ( MaxAxisAngleDelta - 1 ) + 0.5f );
    const int axis_angle_z = (int) floor( axis_angle.z() / ( 2 * pi ) * ( MaxAxisAngleDelta - 1 ) + 0.5f );

    const int base_axis_angle_x = (int) floor( base_axis_angle.x() / ( 2 * pi ) * ( MaxAxisAngleDelta - 1 ) + 0.5f );
    const int base_axis_angle_y = (int) floor( base_axis_angle.y() / ( 2 * pi ) * ( MaxAxisAngleDelta - 1 ) + 0.5f );
    const int base_axis_angle_z = (int) floor( base_axis_angle.z() / ( 2 * pi ) * ( MaxAxisAngleDelta - 1 ) + 0.5f );

    const int axis_angle_delta_x = core::clamp( abs( axis_angle_x - base_axis_angle_x ), 0, MaxAxisAngleDelta - 1 );
    const int axis_angle_delta_y = core::clamp( abs( axis_angle_y - base_axis_angle_y ), 0, MaxAxisAngleDelta - 1 );
    const int axis_angle_delta_z = core::clamp( abs( axis_angle_z - base_axis_angle_z ), 0, MaxAxisAngleDelta - 1 );

    CORE_ASSERT( axis_angle_delta_x >= 0 );
    CORE_ASSERT( axis_angle_delta_y >= 0 );
    CORE_ASSERT( axis_angle_delta_z >= 0 );
    CORE_ASSERT( axis_angle_delta_x < MaxAxisAngleDelta );
    CORE_ASSERT( axis_angle_delta_y < MaxAxisAngleDelta );
    CORE_ASSERT( axis_angle_delta_z < MaxAxisAngleDelta );

    delta_axis_angle_accum_x[axis_angle_delta_x]++;
    delta_axis_angle_accum_y[axis_angle_delta_y]++;
    delta_axis_angle_accum_z[axis_angle_delta_z]++;

    vectorial::quat4f relative_quaternion = orientation * vectorial::conjugate( base_orientation );

    const int relative_quaternion_x = (int) floor( ( relative_quaternion.x() + 1.0f ) * 0.5f * ( MaxRelativeQuaternionDelta - 1 ) + 0.5f );
    const int relative_quaternion_y = (int) floor( ( relative_quaternion.y() + 1.0f ) * 0.5f * ( MaxRelativeQuaternionDelta - 1 ) + 0.5f );
    const int relative_quaternion_z = (int) floor( ( relative_quaternion.z() + 1.0f ) * 0.5f * ( MaxRelativeQuaternionDelta - 1 ) + 0.5f );
    const int relative_quaternion_w = (int) floor( ( relative_quaternion.w() + 1.0f ) * 0.5f * ( MaxRelativeQuaternionDelta - 1 ) + 0.5f );

    fprintf( relative_quaternion_values, "%d,%d,%d,%d\n", 
        relative_quaternion_x,
        relative_quaternion_y,
        relative_quaternion_z,
        relative_quaternion_w );

    const int relative_quaternion_delta_x = abs( ( MaxRelativeQuaternionDelta / 2 - 1 ) - relative_quaternion_x );
    const int relative_quaternion_delta_y = abs( ( MaxRelativeQuaternionDelta / 2 - 1 ) - relative_quaternion_y );
    const int relative_quaternion_delta_z = abs( ( MaxRelativeQuaternionDelta / 2 - 1 ) - relative_quaternion_z );
    const int relative_quaternion_delta_w = abs( ( MaxRelativeQuaternionDelta - 1 ) - relative_quaternion_w );

    CORE_ASSERT( relative_quaternion_delta_x >= 0 );
    CORE_ASSERT( relative_quaternion_delta_y >= 0 );
    CORE_ASSERT( relative_quaternion_delta_z >= 0 );
    CORE_ASSERT( relative_quaternion_delta_w >= 0 );

    CORE_ASSERT( relative_quaternion_delta_x < MaxRelativeQuaternionDelta );
    CORE_ASSERT( relative_quaternion_delta_y < MaxRelativeQuaternionDelta );
    CORE_ASSERT( relative_quaternion_delta_z < MaxRelativeQuaternionDelta );
    CORE_ASSERT( relative_quaternion_delta_w < MaxRelativeQuaternionDelta );

    delta_relative_quaternion_accum_x[relative_quaternion_delta_x]++;
    delta_relative_quaternion_accum_y[relative_quaternion_delta_y]++;
    delta_relative_quaternion_accum_z[relative_quaternion_delta_z]++;
    delta_relative_quaternion_accum_w[relative_quaternion_delta_w]++;
}

#endif // #if DELTA_STATS

struct DeltaSnapshotPacket : public protocol::Packet
{
    uint16_t sequence;
    uint16_t base_sequence;
    bool initial;
    int delta_mode;

    DeltaSnapshotPacket() : Packet( DELTA_SNAPSHOT_PACKET )
    {
        sequence = 0;
        delta_mode = DELTA_MODE_NOT_CHANGED;
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        auto quantized_snapshot_sliding_window = (QuantizedSnapshotSlidingWindow*) stream.GetContext( CONTEXT_QUANTIZED_SNAPSHOT_SLIDING_WINDOW );
        auto quantized_snapshot_sequence_buffer = (QuantizedSnapshotSequenceBuffer*) stream.GetContext( CONTEXT_QUANTIZED_SNAPSHOT_SEQUENCE_BUFFER );
        auto quantized_initial_snapshot = (QuantizedSnapshot*) stream.GetContext( CONTEXT_QUANTIZED_INITIAL_SNAPSHOT );

        serialize_uint16( stream, sequence );

        serialize_int( stream, delta_mode, 0, DELTA_NUM_MODES - 1 );

        serialize_bool( stream, initial );

        if ( !initial )
            serialize_uint16( stream, base_sequence );

        QuantizedCubeState * quantized_cubes = nullptr;

        if ( Stream::IsWriting )
        {
            CORE_ASSERT( quantized_snapshot_sliding_window );
            auto & entry = quantized_snapshot_sliding_window->Get( sequence );
            quantized_cubes = (QuantizedCubeState*) &entry.cubes[0];
        }
        else
        {
            CORE_ASSERT( quantized_snapshot_sequence_buffer );
            auto entry = quantized_snapshot_sequence_buffer->Insert( sequence );
            CORE_ASSERT( entry );
            quantized_cubes = (QuantizedCubeState*) &entry->cubes[0];
        }
        CORE_ASSERT( quantized_cubes );

        switch ( delta_mode )
        {
            case DELTA_MODE_NOT_CHANGED:
            {
                CORE_ASSERT( quantized_initial_snapshot );

                QuantizedCubeState * quantized_base_cubes = nullptr;

//                if ( Stream::IsWriting )
//                    printf( "encoding snapshot %d relative to baseline %d\n", sequence, base_sequence );

                if ( initial )
                {
                    quantized_base_cubes = quantized_initial_snapshot->cubes;
                }
                else
                {
                    if ( Stream::IsWriting )
                    {
                        CORE_ASSERT( quantized_snapshot_sliding_window );
                        auto & entry = quantized_snapshot_sliding_window->Get( base_sequence );
                        quantized_base_cubes = (QuantizedCubeState*) &entry.cubes[0];
                    }
                    else
                    {
                        CORE_ASSERT( quantized_snapshot_sequence_buffer );
                        auto entry = quantized_snapshot_sequence_buffer->Find( base_sequence );
                        CORE_ASSERT( entry );
                        quantized_base_cubes = (QuantizedCubeState*) &entry->cubes[0];
                    }
                }

                for ( int i = 0; i < NumCubes; ++i )
                {
                    bool changed = false;

                    if ( Stream::IsWriting )
                    {
                        changed = quantized_cubes[i] != quantized_base_cubes[i];
#if DELTA_STATS
                        UpdateDeltaStats( quantized_cubes[i], quantized_base_cubes[i] );
#endif // #if DELTA_STATS
                    }

                    serialize_bool( stream, changed );

                    if ( changed )
                    {
                        serialize_cube_changed( stream, quantized_cubes[i], quantized_base_cubes[i] );
                    }
                    else if ( Stream::IsReading )
                    {
                        memcpy( &quantized_cubes[i], &quantized_base_cubes[i], sizeof( QuantizedCubeState ) );
                    }
                }
            }
            break;

            case DELTA_MODE_CHANGED_INDEX:
            {
                CORE_ASSERT( quantized_initial_snapshot );

                QuantizedCubeState * quantized_base_cubes = nullptr;

                if ( initial )
                {
                    quantized_base_cubes = quantized_initial_snapshot->cubes;
                }
                else
                {
                    if ( Stream::IsWriting )
                    {
                        CORE_ASSERT( quantized_snapshot_sliding_window );
                        auto & entry = quantized_snapshot_sliding_window->Get( base_sequence );
                        quantized_base_cubes = (QuantizedCubeState*) &entry.cubes[0];
                    }
                    else
                    {
                        CORE_ASSERT( quantized_snapshot_sequence_buffer );
                        auto entry = quantized_snapshot_sequence_buffer->Find( base_sequence );
                        CORE_ASSERT( entry );
                        quantized_base_cubes = (QuantizedCubeState*) &entry->cubes[0];
                    }
                }

                const int MaxIndex = 89;

                int num_changed = 0;
                bool use_indices = false;
                bool changed[NumCubes];
                if ( Stream::IsWriting )
                {
                    for ( int i = 0; i < NumCubes; ++i )
                    {
                        changed[i] = quantized_cubes[i] != quantized_base_cubes[i];
                        if ( changed[i] )
                            num_changed++;
                    }
                    if ( num_changed < MaxIndex )
                        use_indices = true;
                }

                serialize_bool( stream, use_indices );

                if ( use_indices )
                {
                    serialize_int( stream, num_changed, 0, MaxIndex + 1 );

                    if ( Stream::IsWriting )
                    {
                        int num_written = 0;

                        for ( int i = 0; i < NumCubes; ++i )
                        {
                            if ( changed[i] )
                            {
                                serialize_int( stream, i, 0, NumCubes - 1 );
                                serialize_cube_changed( stream, quantized_cubes[i], quantized_base_cubes[i] );
                                num_written++;
                            }
                        }

                        CORE_ASSERT( num_written == num_changed );
                    }
                    else
                    {
                        memset( changed, 0, sizeof( changed ) );

                        for ( int j = 0; j < num_changed; ++j )
                        {
                            int i;
                            serialize_int( stream, i, 0, NumCubes - 1 );
                            serialize_cube_changed( stream, quantized_cubes[i], quantized_base_cubes[i] );
                            changed[i] = true;
                        }

                        for ( int i = 0; i < NumCubes; ++i )
                        {
                            if ( !changed[i] )
                                memcpy( &quantized_cubes[i], &quantized_base_cubes[i], sizeof( QuantizedCubeState ) );
                        }
                    }
                }
                else
                {
                    for ( int i = 0; i < NumCubes; ++i )
                    {
                        serialize_bool( stream, changed[i] );

                        if ( changed[i] )
                        {
                            serialize_cube_changed( stream, quantized_cubes[i], quantized_base_cubes[i] );
                        }
                        else if ( Stream::IsReading )
                        {
                            memcpy( &quantized_cubes[i], &quantized_base_cubes[i], sizeof( QuantizedCubeState ) );
                        }
                    }
                }
            }
            break;

            case DELTA_MODE_RELATIVE_INDEX:
            {
                CORE_ASSERT( quantized_initial_snapshot );

                QuantizedCubeState * quantized_base_cubes = nullptr;

                if ( initial )
                {
                    quantized_base_cubes = quantized_initial_snapshot->cubes;
                }
                else
                {
                    if ( Stream::IsWriting )
                    {
                        CORE_ASSERT( quantized_snapshot_sliding_window );
                        auto & entry = quantized_snapshot_sliding_window->Get( base_sequence );
                        quantized_base_cubes = (QuantizedCubeState*) &entry.cubes[0];
                    }
                    else
                    {
                        CORE_ASSERT( quantized_snapshot_sequence_buffer );
                        auto entry = quantized_snapshot_sequence_buffer->Find( base_sequence );
                        CORE_ASSERT( entry );
                        quantized_base_cubes = (QuantizedCubeState*) &entry->cubes[0];
                    }
                }

                const int MaxChanged = 255;

                int num_changed = 0;
                bool use_indices = false;
                bool changed[NumCubes];
                if ( Stream::IsWriting )
                {
                    for ( int i = 0; i < NumCubes; ++i )
                    {
                        changed[i] = quantized_cubes[i] != quantized_base_cubes[i];
                        if ( changed[i] )
                            num_changed++;
                    }

                    const int relative_index_bits = count_relative_index_bits( changed );

                    if ( relative_index_bits < 900 && num_changed <= MaxChanged )
                    {
//                        if ( num_changed > 0 )
//                            printf( "num changed: %d, relative index bits: %d (%.1f avg)\n", num_changed, relative_index_bits, relative_index_bits / float( num_changed ) );

                        use_indices = true;
                    }
                }

                serialize_bool( stream, use_indices );

                if ( use_indices )
                {
                    serialize_int( stream, num_changed, 0, MaxChanged );

                    if ( Stream::IsWriting )
                    {
                        int num_written = 0;

                        bool first = true;
                        int previous_index = 0;

                        for ( int i = 0; i < NumCubes; ++i )
                        {
                            if ( changed[i] )
                            {
                                if ( first )
                                {
                                    serialize_int( stream, i, 0, NumCubes - 1 );
                                    first = false;
                                }
                                else
                                {   
                                    serialize_index_relative( stream, previous_index, i );
                                }

                                serialize_cube_changed( stream, quantized_cubes[i], quantized_base_cubes[i] );

                                num_written++;

                                previous_index = i;
                            }
                        }

                        CORE_ASSERT( num_written == num_changed );
                    }
                    else
                    {
                        memset( changed, 0, sizeof( changed ) );

                        int previous_index = 0;

                        for ( int j = 0; j < num_changed; ++j )
                        {
                            int i;
                            if ( j == 0 )
                                serialize_int( stream, i, 0, NumCubes - 1 );
                            else                                
                                serialize_index_relative( stream, previous_index, i );

                            serialize_cube_changed( stream, quantized_cubes[i], quantized_base_cubes[i] );

                            changed[i] = true;

                            previous_index = i;
                        }

                        for ( int i = 0; i < NumCubes; ++i )
                        {
                            if ( !changed[i] )
                                memcpy( &quantized_cubes[i], &quantized_base_cubes[i], sizeof( QuantizedCubeState ) );
                        }
                    }
                }
                else
                {
                    for ( int i = 0; i < NumCubes; ++i )
                    {
                        serialize_bool( stream, changed[i] );

                        if ( changed[i] )
                        {
                            serialize_cube_changed( stream, quantized_cubes[i], quantized_base_cubes[i] );
                        }
                        else if ( Stream::IsReading )
                        {
                            memcpy( &quantized_cubes[i], &quantized_base_cubes[i], sizeof( QuantizedCubeState ) );
                        }
                    }
                }
            }
            break;

            case DELTA_MODE_RELATIVE_POSITION:
            {
                CORE_ASSERT( quantized_initial_snapshot );

                QuantizedCubeState * quantized_base_cubes = nullptr;

                if ( initial )
                {
                    quantized_base_cubes = quantized_initial_snapshot->cubes;
                }
                else
                {
                    if ( Stream::IsWriting )
                    {
                        CORE_ASSERT( quantized_snapshot_sliding_window );
                        auto & entry = quantized_snapshot_sliding_window->Get( base_sequence );
                        quantized_base_cubes = (QuantizedCubeState*) &entry.cubes[0];
                    }
                    else
                    {
                        CORE_ASSERT( quantized_snapshot_sequence_buffer );
                        auto entry = quantized_snapshot_sequence_buffer->Find( base_sequence );
                        CORE_ASSERT( entry );
                        quantized_base_cubes = (QuantizedCubeState*) &entry->cubes[0];
                    }
                }

                const int MaxIndex = 126;

                int num_changed = 0;
                bool use_indices = false;
                bool changed[NumCubes];
                if ( Stream::IsWriting )
                {
                    for ( int i = 0; i < NumCubes; ++i )
                    {
                        changed[i] = quantized_cubes[i] != quantized_base_cubes[i];
                        if ( changed[i] )
                            num_changed++;
                    }
                    if ( num_changed < MaxIndex )
                        use_indices = true;
                }

                serialize_bool( stream, use_indices );

                if ( use_indices )
                {
                    serialize_int( stream, num_changed, 0, MaxIndex + 1 );

                    if ( Stream::IsWriting )
                    {
                        int num_written = 0;

                        bool first = true;
                        int previous_index = 0;

                        for ( int i = 0; i < NumCubes; ++i )
                        {
                            if ( changed[i] )
                            {
                                if ( first )
                                {
                                    serialize_int( stream, i, 0, NumCubes - 1 );
                                    first = false;
                                }
                                else
                                {   
                                    serialize_index_relative( stream, previous_index, i );
                                }

                                serialize_cube_relative_position( stream, quantized_cubes[i], quantized_base_cubes[i] );

                                num_written++;

                                previous_index = i;
                            }
                        }

                        CORE_ASSERT( num_written == num_changed );
                    }
                    else
                    {
                        memset( changed, 0, sizeof( changed ) );

                        int previous_index = 0;

                        for ( int j = 0; j < num_changed; ++j )
                        {
                            int i;
                            if ( j == 0 )
                                serialize_int( stream, i, 0, NumCubes - 1 );
                            else                                
                                serialize_index_relative( stream, previous_index, i );

                            serialize_cube_relative_position( stream, quantized_cubes[i], quantized_base_cubes[i] );

                            changed[i] = true;

                            previous_index = i;
                        }

                        for ( int i = 0; i < NumCubes; ++i )
                        {
                            if ( !changed[i] )
                                memcpy( &quantized_cubes[i], &quantized_base_cubes[i], sizeof( QuantizedCubeState ) );
                        }
                    }
                }
                else
                {
                    for ( int i = 0; i < NumCubes; ++i )
                    {
                        serialize_bool( stream, changed[i] );

                        if ( changed[i] )
                        {
                            serialize_cube_relative_position( stream, quantized_cubes[i], quantized_base_cubes[i] );
                        }
                        else if ( Stream::IsReading )
                        {
                            memcpy( &quantized_cubes[i], &quantized_base_cubes[i], sizeof( QuantizedCubeState ) );
                        }
                    }
                }
            }
            break;

            case DELTA_MODE_RELATIVE_ORIENTATION:
            {
                CORE_ASSERT( quantized_initial_snapshot );

                QuantizedCubeState * quantized_base_cubes = nullptr;

                if ( initial )
                {
                    quantized_base_cubes = quantized_initial_snapshot->cubes;
                }
                else
                {
                    if ( Stream::IsWriting )
                    {
                        CORE_ASSERT( quantized_snapshot_sliding_window );
                        auto & entry = quantized_snapshot_sliding_window->Get( base_sequence );
                        quantized_base_cubes = (QuantizedCubeState*) &entry.cubes[0];
                    }
                    else
                    {
                        CORE_ASSERT( quantized_snapshot_sequence_buffer );
                        auto entry = quantized_snapshot_sequence_buffer->Find( base_sequence );
                        CORE_ASSERT( entry );
                        quantized_base_cubes = (QuantizedCubeState*) &entry->cubes[0];
                    }
                }

                const int MaxIndex = 126;

                int num_changed = 0;
                bool use_indices = false;
                bool changed[NumCubes];
                if ( Stream::IsWriting )
                {
                    for ( int i = 0; i < NumCubes; ++i )
                    {
                        changed[i] = quantized_cubes[i] != quantized_base_cubes[i];
                        if ( changed[i] )
                            num_changed++;
                    }
                    if ( num_changed < MaxIndex )
                        use_indices = true;
                }

                serialize_bool( stream, use_indices );

                if ( use_indices )
                {
                    serialize_int( stream, num_changed, 0, MaxIndex + 1 );

                    if ( Stream::IsWriting )
                    {
                        int num_written = 0;

                        bool first = true;
                        int previous_index = 0;

                        for ( int i = 0; i < NumCubes; ++i )
                        {
                            if ( changed[i] )
                            {
                                if ( first )
                                {
                                    serialize_int( stream, i, 0, NumCubes - 1 );
                                    first = false;
                                }
                                else
                                {   
                                    serialize_index_relative( stream, previous_index, i );
                                }

                                serialize_cube_relative_orientation( stream, quantized_cubes[i], quantized_base_cubes[i] );

                                num_written++;

                                previous_index = i;
                            }
                        }

                        CORE_ASSERT( num_written == num_changed );
                    }
                    else
                    {
                        memset( changed, 0, sizeof( changed ) );

                        int previous_index = 0;

                        for ( int j = 0; j < num_changed; ++j )
                        {
                            int i;
                            if ( j == 0 )
                                serialize_int( stream, i, 0, NumCubes - 1 );
                            else                                
                                serialize_index_relative( stream, previous_index, i );

                            serialize_cube_relative_orientation( stream, quantized_cubes[i], quantized_base_cubes[i] );

                            changed[i] = true;

                            previous_index = i;
                        }

                        for ( int i = 0; i < NumCubes; ++i )
                        {
                            if ( !changed[i] )
                                memcpy( &quantized_cubes[i], &quantized_base_cubes[i], sizeof( QuantizedCubeState ) );
                        }
                    }
                }
                else
                {
                    for ( int i = 0; i < NumCubes; ++i )
                    {
                        serialize_bool( stream, changed[i] );

                        if ( changed[i] )
                        {
                            serialize_cube_relative_orientation( stream, quantized_cubes[i], quantized_base_cubes[i] );
                        }
                        else if ( Stream::IsReading )
                        {
                            memcpy( &quantized_cubes[i], &quantized_base_cubes[i], sizeof( QuantizedCubeState ) );
                        }
                    }
                }
            }
            break;

            default:
                break;
        }
    }
};

struct DeltaAckPacket : public protocol::Packet
{
    uint16_t ack;

    DeltaAckPacket() : Packet( DELTA_ACK_PACKET )
    {
        ack = 0;
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_uint16( stream, ack );
    }
};

class DeltaPacketFactory : public protocol::PacketFactory
{
    core::Allocator * m_allocator;

public:

    DeltaPacketFactory( core::Allocator & allocator )
        : PacketFactory( allocator, DELTA_NUM_PACKETS )
    {
        m_allocator = &allocator;
    }

protected:

    protocol::Packet * CreateInternal( int type )
    {
        switch ( type )
        {
            case DELTA_SNAPSHOT_PACKET:   return CORE_NEW( *m_allocator, DeltaSnapshotPacket );
            case DELTA_ACK_PACKET:        return CORE_NEW( *m_allocator, DeltaAckPacket );
            default:
                return nullptr;
        }
    }
};

struct DeltaInternal
{
    DeltaInternal( core::Allocator & allocator, const SnapshotModeData & mode_data ) 
        : packet_factory( allocator ), interpolation_buffer( allocator, mode_data )
    {
        this->allocator = &allocator;
        network::SimulatorConfig networkSimulatorConfig;
        quantized_snapshot_sliding_window = CORE_NEW( allocator, QuantizedSnapshotSlidingWindow, allocator, MaxSnapshots );
        quantized_snapshot_sequence_buffer = CORE_NEW( allocator, QuantizedSnapshotSequenceBuffer, allocator, MaxSnapshots );
        networkSimulatorConfig.packetFactory = &packet_factory;
        networkSimulatorConfig.maxPacketSize = MaxPacketSize;
        network_simulator = CORE_NEW( allocator, network::Simulator, networkSimulatorConfig );
        context[0] = quantized_snapshot_sliding_window;
        context[1] = quantized_snapshot_sequence_buffer;
        context[2] = &quantized_initial_snapshot;
        network_simulator->SetContext( context );
        Reset( mode_data );
    }

    ~DeltaInternal()
    {
        CORE_ASSERT( network_simulator );
        typedef network::Simulator NetworkSimulator;
        CORE_DELETE( *allocator, NetworkSimulator, network_simulator );
        CORE_DELETE( *allocator, QuantizedSnapshotSlidingWindow, quantized_snapshot_sliding_window );
        CORE_DELETE( *allocator, QuantizedSnapshotSequenceBuffer, quantized_snapshot_sequence_buffer );
        network_simulator = nullptr;
        quantized_snapshot_sliding_window = nullptr;
        quantized_snapshot_sequence_buffer = nullptr;
    }

    void Reset( const SnapshotModeData & mode_data )
    {
        interpolation_buffer.Reset();
        network_simulator->Reset();
        network_simulator->ClearStates();
        network_simulator->AddState( { mode_data.latency, mode_data.jitter, mode_data.packet_loss } );
        quantized_snapshot_sliding_window->Reset();
        quantized_snapshot_sequence_buffer->Reset();
        send_sequence = 0;
        recv_sequence = 0;
        send_accumulator = 1.0f;
        received_ack = false;
    }

    core::Allocator * allocator;
    uint16_t send_sequence;
    uint16_t recv_sequence;
    bool received_ack;
    float send_accumulator;
    const void * context[3];
    network::Simulator * network_simulator;
    QuantizedSnapshotSlidingWindow * quantized_snapshot_sliding_window;
    QuantizedSnapshotSequenceBuffer * quantized_snapshot_sequence_buffer;
    DeltaPacketFactory packet_factory;
    SnapshotInterpolationBuffer interpolation_buffer;
    QuantizedSnapshot quantized_initial_snapshot;
};

#if DELTA_STATS

void DumpDeltaAccumulators()
{
    {
        FILE * file = fopen( "output/delta_position.txt", "w" );
        if ( file )
        {
            for ( int i = 0; i < MaxPositionDelta; ++i )
                fprintf( file, "%lld,%lld,%lld\n", delta_position_accum_x[i], delta_position_accum_y[i], delta_position_accum_z[i] );
            fclose( file );
        }
    }

    {
        FILE * file = fopen( "output/delta_smallest_three.txt", "w" );
        if ( file )
        {
            for ( int i = 0; i < MaxSmallestThreeDelta; ++i )
                fprintf( file, "%lld,%lld,%lld\n", delta_smallest_three_accum_a[i], delta_smallest_three_accum_b[i], delta_smallest_three_accum_c[i] );
            fclose( file );
        }
    }

    {
        FILE * file = fopen( "output/delta_quaternion.txt", "w" );
        if ( file )
        {
            for ( int i = 0; i < MaxQuaternionDelta; ++i )
                fprintf( file, "%lld,%lld,%lld,%lld\n", delta_quaternion_accum_x[i], delta_quaternion_accum_y[i], delta_quaternion_accum_z[i], delta_quaternion_accum_w[i] );

            fclose( file );
        }
    }

    {
        FILE * file = fopen( "output/delta_angle.txt", "w" );
        if ( file )
        {
            for ( int i = 0; i < MaxAngleDelta; ++i )
                fprintf( file, "%lld\n", delta_angle_accum[i] );

            fclose( file );
        }
    }

    {
        FILE * file = fopen( "output/delta_axis.txt", "w" );
        if ( file )
        {
            for ( int i = 0; i < MaxAxisDelta; ++i )
                fprintf( file, "%lld,%lld,%lld\n", delta_axis_accum_x[i], delta_axis_accum_y[i], delta_axis_accum_z[i] );

            fclose( file );
        }
    }

    {
        FILE * file = fopen( "output/delta_axis_angle.txt", "w" );
        if ( file )
        {
            for ( int i = 0; i < MaxAxisAngleDelta; ++i )
                fprintf( file, "%lld,%lld,%lld\n", delta_axis_angle_accum_x[i], delta_axis_angle_accum_y[i], delta_axis_angle_accum_z[i] );

            fclose( file );
        }
    }

    {
        FILE * file = fopen( "output/delta_relative_quaternion.txt", "w" );
        if ( file )
        {
            for ( int i = 0; i < MaxRelativeQuaternionDelta; ++i )
                fprintf( file, "%lld,%lld,%lld,%lld\n", delta_relative_quaternion_accum_x[i], delta_relative_quaternion_accum_y[i], delta_relative_quaternion_accum_z[i], delta_relative_quaternion_accum_w[i] );

            fclose( file );
        }
    }
}

#endif // #if DELTA_STATS

DeltaDemo::DeltaDemo( core::Allocator & allocator )
{
    InitDeltaModes();

    SetMode( DELTA_MODE_NOT_CHANGED );

    m_allocator = &allocator;
    m_internal = nullptr;
    m_settings = CORE_NEW( *m_allocator, CubesSettings );
    m_delta = CORE_NEW( *m_allocator, DeltaInternal, *m_allocator, delta_mode_data[GetMode()] );
  
#if DELTA_STATS

    memset( delta_position_accum_x, 0, sizeof( delta_position_accum_x ) );
    memset( delta_position_accum_y, 0, sizeof( delta_position_accum_x ) );
    memset( delta_position_accum_z, 0, sizeof( delta_position_accum_x ) );

    memset( delta_smallest_three_accum_a, 0, sizeof( delta_smallest_three_accum_a ) );
    memset( delta_smallest_three_accum_b, 0, sizeof( delta_smallest_three_accum_b ) );
    memset( delta_smallest_three_accum_c, 0, sizeof( delta_smallest_three_accum_c ) );

    memset( delta_quaternion_accum_x, 0, sizeof( delta_quaternion_accum_x ) );
    memset( delta_quaternion_accum_y, 0, sizeof( delta_quaternion_accum_y ) );
    memset( delta_quaternion_accum_z, 0, sizeof( delta_quaternion_accum_z ) );
    memset( delta_quaternion_accum_w, 0, sizeof( delta_quaternion_accum_w ) );

    memset( delta_angle_accum, 0, sizeof( delta_angle_accum ) );

    memset( delta_axis_accum_x, 0, sizeof( delta_axis_accum_x ) );
    memset( delta_axis_accum_y, 0, sizeof( delta_axis_accum_y ) );
    memset( delta_axis_accum_z, 0, sizeof( delta_axis_accum_z ) );

    memset( delta_axis_angle_accum_x, 0, sizeof( delta_axis_angle_accum_x ) );
    memset( delta_axis_angle_accum_y, 0, sizeof( delta_axis_angle_accum_y ) );
    memset( delta_axis_angle_accum_z, 0, sizeof( delta_axis_angle_accum_z ) );

    memset( delta_relative_quaternion_accum_x, 0, sizeof( delta_relative_quaternion_accum_x ) );
    memset( delta_relative_quaternion_accum_y, 0, sizeof( delta_relative_quaternion_accum_y ) );
    memset( delta_relative_quaternion_accum_z, 0, sizeof( delta_relative_quaternion_accum_z ) );
    memset( delta_relative_quaternion_accum_w, 0, sizeof( delta_relative_quaternion_accum_w ) );

    position_values = fopen( "output/position_values.txt", "w" );
    quaternion_values = fopen( "output/quaternion_values.txt", "w" );
    quaternion_float_values = fopen( "output/quaternion_float_values.txt", "w" );
    relative_quaternion_values = fopen( "output/relative_quaternion_values.txt", "w" );
    smallest_three_values = fopen( "output/smallest_three_values.txt", "w" );
    axis_angle_values = fopen( "output/axis_angle_values.txt", "w" );

#endif // #if DELTA_STATS

#if DELTA_DATA

    delta_data = fopen( "output/delta_data.bin", "wb" );

#endif // #if DELTA_DATA
}

DeltaDemo::~DeltaDemo()
{
#if DELTA_DATA

    fclose( delta_data );

#endif // #if DELTA_DATA


#if DELTA_STATS

    DumpDeltaAccumulators();

    fclose( position_values );
    fclose( quaternion_values ) ;
    fclose( quaternion_float_values ) ;
    fclose( relative_quaternion_values );
    fclose( smallest_three_values );

#endif // #if DELTA_STATS

    Shutdown();
 
    CORE_DELETE( *m_allocator, DeltaInternal, m_delta );
    CORE_DELETE( *m_allocator, CubesSettings, m_settings );

    m_delta = nullptr;
    m_settings = nullptr;
    m_allocator = nullptr;
}

bool DeltaDemo::Initialize()
{
    if ( m_internal )
        Shutdown();

    m_internal = CORE_NEW( *m_allocator, CubesInternal );    

    CubesConfig config;
    
    config.num_simulations = 1;
    config.num_views = 2;

    m_internal->Initialize( *m_allocator, config, m_settings );

    auto game_instance = m_internal->GetGameInstance( 0 );

    // hack: we must pump one physics update to make sure initial state is valid
    CubesUpdateConfig update_config;
    update_config.sim[0].num_frames = 1;
    m_internal->Update( update_config );    

    GetQuantizedSnapshot( game_instance, m_delta->quantized_initial_snapshot );

    return true;
}

void DeltaDemo::Shutdown()
{
    CORE_ASSERT( m_allocator );

    CORE_ASSERT( m_delta );
    m_delta->Reset( delta_mode_data[GetMode()] );

    if ( m_internal )
    {
        m_internal->Free( *m_allocator );
        CORE_DELETE( *m_allocator, CubesInternal, m_internal );
        m_internal = nullptr;
    }
}

void DeltaDemo::Update()
{
    CubesUpdateConfig update_config;

    auto local_input = m_internal->GetLocalInput();

    // setup left simulation to update one frame with local input

    update_config.sim[0].num_frames = 1;
    update_config.sim[0].frame_input[0] = local_input;

    // send a snapshot packet to the right side

    m_delta->send_accumulator += global.timeBase.deltaTime;

    if ( m_delta->send_accumulator >= 1.0f / delta_mode_data[GetMode()].send_rate )
    {
        m_delta->send_accumulator = 0.0f;   

        auto game_instance = m_internal->GetGameInstance( 0 );

        auto snapshot_packet = (DeltaSnapshotPacket*) m_delta->packet_factory.Create( DELTA_SNAPSHOT_PACKET );

        snapshot_packet->sequence = m_delta->send_sequence++;
        snapshot_packet->base_sequence = m_delta->quantized_snapshot_sliding_window->GetAck() + 1;
        snapshot_packet->initial = !m_delta->received_ack;

        snapshot_packet->delta_mode = GetMode();

        uint16_t sequence;

        auto & snapshot = m_delta->quantized_snapshot_sliding_window->Insert( sequence );

        if ( GetQuantizedSnapshot( game_instance, snapshot ) )
        {
            m_delta->network_simulator->SendPacket( network::Address( "::1", RightPort ), snapshot_packet );

#if DELTA_DATA

            const int reps = ( sequence == 0 ) ? 6 : 1;

            for ( int j = 0; j < reps; ++j )
            {
                auto * cubes = (QuantizedCubeState*) &snapshot.cubes[0];

                for ( int i = 0; i < NumCubes; ++i )
                {
                    DeltaData current;

                    current.orientation_x = cubes[i].original_orientation.x();
                    current.orientation_y = cubes[i].original_orientation.y();
                    current.orientation_z = cubes[i].original_orientation.z();
                    current.orientation_w = cubes[i].original_orientation.w();
                    current.position_x = cubes[i].original_position.x();
                    current.position_y = cubes[i].original_position.y();
                    current.position_z = cubes[i].original_position.z();

                    fwrite( &current, sizeof( DeltaData ), 1, delta_data );
                }
            }

#endif // #if DELTA_DATA

        }
        else
        {
            m_delta->packet_factory.Destroy( snapshot_packet );
        }
    }

    // update the network simulator

    m_delta->network_simulator->Update( global.timeBase );

    // receive packets from the simulator (with latency, packet loss and jitter applied...)

    bool received_snapshot_this_frame = false;
    uint16_t ack_sequence = 0;

    while ( true )
    {
        auto packet = m_delta->network_simulator->ReceivePacket();
        if ( !packet )
            break;

        const auto port = packet->GetAddress().GetPort();
        const auto type = packet->GetType();

        if ( type == DELTA_SNAPSHOT_PACKET && port == RightPort )
        {
            auto snapshot_packet = (DeltaSnapshotPacket*) packet;

            QuantizedSnapshot * quantized_snapshot = m_delta->quantized_snapshot_sequence_buffer->Find( snapshot_packet->sequence );
    
            CORE_ASSERT( quantized_snapshot );
    
            Snapshot snapshot;
            for ( int i = 0; i < NumCubes; ++i )
                quantized_snapshot->cubes[i].Save( snapshot.cubes[i] );

            m_delta->interpolation_buffer.AddSnapshot( global.timeBase.time, snapshot_packet->sequence, snapshot.cubes );

            if ( !received_snapshot_this_frame || ( received_snapshot_this_frame && core::sequence_greater_than( snapshot_packet->sequence, ack_sequence ) ) )
            {
                received_snapshot_this_frame = true;
                ack_sequence = snapshot_packet->sequence;
            }
        }
        else if ( type == DELTA_ACK_PACKET && port == LeftPort )
        {
            auto ack_packet = (DeltaAckPacket*) packet;

            m_delta->quantized_snapshot_sliding_window->Ack( ack_packet->ack - 1 );
            m_delta->received_ack = true;
        }

        m_delta->packet_factory.Destroy( packet );
    }

    // if any snapshots packets were received this frame, send an ack packet back to the left simulation

    if ( received_snapshot_this_frame )
    {
        auto ack_packet = (DeltaAckPacket*) m_delta->packet_factory.Create( DELTA_ACK_PACKET );
        ack_packet->ack = ack_sequence;
        m_delta->network_simulator->SetBandwidthExclude( true );
        m_delta->network_simulator->SendPacket( network::Address( "::1", LeftPort ), ack_packet );
        m_delta->network_simulator->SetBandwidthExclude( false );
    }

    // if we are an an interpolation mode, we need to grab the view updates for the right side from the interpolation buffer

    int num_object_updates = 0;

    view::ObjectUpdate object_updates[NumCubes];

    m_delta->interpolation_buffer.GetViewUpdate( delta_mode_data[GetMode()], global.timeBase.time, object_updates, num_object_updates );

    if ( num_object_updates > 0 )
        m_internal->view[1].objects.UpdateObjects( object_updates, num_object_updates );
    else if ( m_delta->interpolation_buffer.interpolating )
        printf( "no snapshot to interpolate towards!\n" );

    // run the simulation

    m_internal->Update( update_config );
}

bool DeltaDemo::Clear()
{
    return m_internal->Clear();
}

void DeltaDemo::Render()
{
    CubesRenderConfig render_config;

    render_config.render_mode = CUBES_RENDER_SPLITSCREEN;

    m_internal->Render( render_config );

    const float bandwidth = m_delta->network_simulator->GetBandwidth();

    char bandwidth_string[256];
    if ( bandwidth < 1024 )
        snprintf( bandwidth_string, (int) sizeof( bandwidth_string ), "Bandwidth: %d kbps", (int) bandwidth );
    else
        snprintf( bandwidth_string, (int) sizeof( bandwidth_string ), "Bandwidth: %.2f mbps", bandwidth / 1000 );

    Font * font = global.fontManager->GetFont( "Bandwidth" );
    if ( font )
    {
        const float text_x = ( global.displayWidth - font->GetTextWidth( bandwidth_string ) ) / 2;
        const float text_y = 5;
        font->Begin();
        font->DrawText( text_x, text_y, bandwidth_string, Color( 0.27f,0.81f,1.0f ) );
        font->End();
    }
}

bool DeltaDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    return m_internal->KeyEvent( key, scancode, action, mods );
}

bool DeltaDemo::CharEvent( unsigned int code )
{
    // ...

    return false;
}

int DeltaDemo::GetNumModes() const
{
    return DELTA_NUM_MODES;
}

const char * DeltaDemo::GetModeDescription( int mode ) const
{
    return delta_mode_descriptions[mode];
}

#endif // #ifdef CLIENT
