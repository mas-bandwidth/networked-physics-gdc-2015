#include "SyncDemo.h"

#ifdef CLIENT

#include "stdlib.h"
#include "Cubes.h"
#include "Global.h"
#include "Font.h"
#include "Snapshot.h"
#include "FontManager.h"
#include "protocol/Stream.h"
#include "protocol/PacketFactory.h"
#include "network/Simulator.h"
#include <algorithm>

static const int MaxCubesPerPacket = 63;
static const int NumStateUpdates = 256;
static const int RightPort = 1001;

enum SyncMode
{
    SYNC_MODE_UNCOMPRESSED,
    SYNC_MODE_COMPRESSED,
    SYNC_MODE_QUANTIZE_ON_BOTH_SIDES,
    SYNC_MODE_PACKET_LOSS,
    SYNC_MODE_BASIC_SMOOTHING,
    SYNC_MODE_ADAPTIVE_SMOOTHING,
    SYNC_NUM_MODES
};

const char * sync_mode_descriptions[]
{
    "Uncompressed",
    "Compressed",
    "Quantize on both sides",
    "Packet Loss",
    "Basic Smoothing",
    "Adaptive Smoothing"
};

struct SyncModeData
{
    float playout_delay = 5 * ( 1.0f / 60.0f );
    float latency = 0.0f;
    float packet_loss = 0.0f;
    float jitter = 0.0f;
};

static SyncModeData sync_mode_data[SYNC_NUM_MODES];

static void InitSyncModes()
{
    sync_mode_data[SYNC_MODE_PACKET_LOSS].packet_loss = 10.0f;
    sync_mode_data[SYNC_MODE_BASIC_SMOOTHING].packet_loss = 10.0f;
    sync_mode_data[SYNC_MODE_ADAPTIVE_SMOOTHING].packet_loss = 10.0f;
}

enum SyncPackets
{
    SYNC_STATE_PACKET_UNCOMPRESSED,
    SYNC_STATE_PACKET_COMPRESSED,
    SYNC_NUM_PACKETS
};

struct StateUpdateUncompressed
{
    game::Input input;
    uint16_t sequence = 0;
    int num_cubes = 0;
    int cube_index[MaxCubesPerPacket];
    CubeState cube_state[MaxCubesPerPacket];
};

struct StateUpdateCompressed
{
    game::Input input;
    uint16_t sequence = 0;
    int num_cubes = 0;
    int cube_index[MaxCubesPerPacket];
    QuantizedCubeState_HighPrecision cube_state[MaxCubesPerPacket];
};

template <typename Stream> void serialize_cube_state_uncompressed( Stream & stream, int & index, CubeState & cube )
{
    serialize_int( stream, index, 0, NumCubes - 1 );
    serialize_vector( stream, cube.position );
    serialize_quaternion( stream, cube.orientation );

    bool at_rest = Stream::IsWriting ? cube.AtRest() : false;
    
    serialize_bool( stream, at_rest );

    if ( !at_rest )
    {
        serialize_vector( stream, cube.linear_velocity );
        serialize_vector( stream, cube.angular_velocity );
    }
    else if ( Stream::IsReading )
    {
        cube.linear_velocity = vectorial::vec3f(0,0,0);
        cube.angular_velocity = vectorial::vec3f(0,0,0);
    }
}

template <typename Stream> void serialize_cube_state_compressed( Stream & stream, int & index, QuantizedCubeState_HighPrecision & cube )
{
    serialize_int( stream, index, 0, NumCubes - 1 );

    serialize_int( stream, cube.position_x, -QuantizedPositionBoundXY_HighPrecision, +QuantizedPositionBoundXY_HighPrecision - 1 );
    serialize_int( stream, cube.position_y, -QuantizedPositionBoundXY_HighPrecision, +QuantizedPositionBoundXY_HighPrecision - 1 );
    serialize_int( stream, cube.position_z, 0, QuantizedPositionBoundZ_HighPrecision - 1 );

    serialize_object( stream, cube.orientation );

    bool at_rest = Stream::IsWriting ? cube.AtRest() : false;
    
    serialize_bool( stream, at_rest );

    if ( !at_rest )
    {
        serialize_int( stream, cube.linear_velocity_x, -QuantizedLinearVelocityBound_HighPrecision, +QuantizedLinearVelocityBound_HighPrecision - 1 );
        serialize_int( stream, cube.linear_velocity_y, -QuantizedLinearVelocityBound_HighPrecision, +QuantizedLinearVelocityBound_HighPrecision - 1 );
        serialize_int( stream, cube.linear_velocity_z, -QuantizedLinearVelocityBound_HighPrecision, +QuantizedLinearVelocityBound_HighPrecision - 1 );

        serialize_int( stream, cube.angular_velocity_x, -QuantizedAngularVelocityBound_HighPrecision, +QuantizedAngularVelocityBound_HighPrecision - 1 );
        serialize_int( stream, cube.angular_velocity_y, -QuantizedAngularVelocityBound_HighPrecision, +QuantizedAngularVelocityBound_HighPrecision - 1 );
        serialize_int( stream, cube.angular_velocity_z, -QuantizedAngularVelocityBound_HighPrecision, +QuantizedAngularVelocityBound_HighPrecision - 1 );
    }
    else if ( Stream::IsReading )
    {
        cube.linear_velocity_x = 0;
        cube.linear_velocity_y = 0;
        cube.linear_velocity_z = 0;

        cube.angular_velocity_x = 0;
        cube.angular_velocity_y = 0;
        cube.angular_velocity_z = 0;
    }
}

struct StatePacketUncompressed : public protocol::Packet
{
    StateUpdateUncompressed state_update;

    StatePacketUncompressed() : Packet( SYNC_STATE_PACKET_UNCOMPRESSED ) {}

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_bool( stream, state_update.input.left );
        serialize_bool( stream, state_update.input.right );
        serialize_bool( stream, state_update.input.up );
        serialize_bool( stream, state_update.input.down );
        serialize_bool( stream, state_update.input.push );
        serialize_bool( stream, state_update.input.pull );

        serialize_uint16( stream, state_update.sequence );

        serialize_int( stream, state_update.num_cubes, 0, MaxCubesPerPacket );

        for ( int i = 0; i < state_update.num_cubes; ++i )
        {
            serialize_cube_state_uncompressed( stream, state_update.cube_index[i], state_update.cube_state[i] );
        }
    }
};

struct StatePacketCompressed : public protocol::Packet
{
    StateUpdateCompressed state_update;

    StatePacketCompressed() : Packet( SYNC_STATE_PACKET_COMPRESSED ) {}

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_bool( stream, state_update.input.left );
        serialize_bool( stream, state_update.input.right );
        serialize_bool( stream, state_update.input.up );
        serialize_bool( stream, state_update.input.down );
        serialize_bool( stream, state_update.input.push );
        serialize_bool( stream, state_update.input.pull );

        serialize_uint16( stream, state_update.sequence );

        serialize_int( stream, state_update.num_cubes, 0, MaxCubesPerPacket );

        for ( int i = 0; i < state_update.num_cubes; ++i )
        {
            serialize_cube_state_compressed( stream, state_update.cube_index[i], state_update.cube_state[i] );
        }
    }
};

class StatePacketFactory : public protocol::PacketFactory
{
    core::Allocator * m_allocator;

public:

    StatePacketFactory( core::Allocator & allocator )
        : PacketFactory( allocator, SYNC_NUM_PACKETS )
    {
        m_allocator = &allocator;
    }

protected:

    protocol::Packet * CreateInternal( int type )
    {
        switch ( type )
        {
            case SYNC_STATE_PACKET_COMPRESSED:      return CORE_NEW( *m_allocator, StatePacketCompressed );
            case SYNC_STATE_PACKET_UNCOMPRESSED:    return CORE_NEW( *m_allocator, StatePacketUncompressed );
            default:
                return nullptr;
        }
    }
};

struct CubePriorityInfo
{
    int index;
    double accum;
};

template <typename T> struct StateJitterBuffer
{
    StateJitterBuffer( core::Allocator & allocator, const SyncModeData & mode_data )
        : state_updates( allocator, NumStateUpdates )
    {
        stopped = true;
        start_time = 0.0;
        playout_delay = mode_data.playout_delay;
    }

    void AddStateUpdate( double time, uint16_t sequence, const T & state_update )
    {
        if ( stopped )
        {
            start_time = time;
            stopped = false;
        }

        auto entry = state_updates.Insert( sequence );

        if ( entry )
            memcpy( entry, &state_update, sizeof( state_update ) );
    }

    bool GetStateUpdate( double time, T & state_update )
    {
        // we have not received a packet yet. no state update

        if ( stopped )
            return false;

        // if time minus playout delay is negative, it's too early for state updates

        time -= ( start_time + playout_delay );

        if ( time < 0 )
            return false;

        // if we are interpolating but the interpolation start time is too old,
        // go back to the not interpolating state, so we can find a new start point.

        const double frames_since_start = time * 60;        // note: locked to 60fps update rate

        uint16_t sequence = (uint16_t) uint64_t( floor( frames_since_start ) );

        auto entry = state_updates.Find( sequence );

        if ( !entry )
            return false;

        memcpy( &state_update, entry, sizeof( state_update ) );

        state_updates.Remove( sequence );

        return true;
    }

    void Reset()
    {
        stopped = true;
        start_time = 0.0;
        state_updates.Reset();
    }

    bool IsRunning( double time ) const
    {
        if ( stopped )
            return false;

        time -= ( start_time + playout_delay );

        return time >= 0;
    }

private:

    bool stopped;
    double start_time;
    float playout_delay;
    protocol::SequenceBuffer<T> state_updates;
};

typedef StateJitterBuffer<StateUpdateCompressed> StateJitterBufferCompressed;
typedef StateJitterBuffer<StateUpdateUncompressed> StateJitterBufferUncompressed;

struct SyncInternal
{
    SyncInternal( core::Allocator & allocator, const SyncModeData & mode_data ) 
        : packet_factory( allocator )
    {
        this->allocator = &allocator;
        network::SimulatorConfig networkSimulatorConfig;
        networkSimulatorConfig.packetFactory = &packet_factory;
        networkSimulatorConfig.maxPacketSize = 4096;
        network_simulator = CORE_NEW( allocator, network::Simulator, networkSimulatorConfig );
        jitter_buffer_compressed = CORE_NEW( allocator, StateJitterBufferCompressed, allocator, mode_data );
        jitter_buffer_uncompressed = CORE_NEW( allocator, StateJitterBufferUncompressed, allocator, mode_data );
        Reset( mode_data );
    }

    ~SyncInternal()
    {
        CORE_ASSERT( network_simulator );
        typedef network::Simulator NetworkSimulator;
        CORE_DELETE( *allocator, NetworkSimulator, network_simulator );
        CORE_DELETE( *allocator, StateJitterBufferCompressed, jitter_buffer_compressed );
        CORE_DELETE( *allocator, StateJitterBufferUncompressed, jitter_buffer_uncompressed );
        network_simulator = nullptr;
        jitter_buffer_compressed = nullptr;
        jitter_buffer_uncompressed = nullptr;
    }

    void Reset( const SyncModeData & mode_data )
    {
        network_simulator->Reset();
        network_simulator->ClearStates();
        network_simulator->AddState( { mode_data.latency, mode_data.jitter, mode_data.packet_loss } );
        jitter_buffer_compressed->Reset();
        jitter_buffer_uncompressed->Reset();
        send_sequence = 0;
        for ( int i = 0; i < NumCubes; ++i )
        {
            priority_info[i].index = i;
            priority_info[i].accum = 0.0;
            position_error[i] = vectorial::vec3f(0,0,0);
            orientation_error[i] = vectorial::quat4f(0,0,0,1);
        }
    }

    core::Allocator * allocator;
    uint16_t send_sequence;
    game::Input remote_input;
    bool disable_packets = false;
    network::Simulator * network_simulator;
    StatePacketFactory packet_factory;
    CubePriorityInfo priority_info[NumCubes];
    StateJitterBufferCompressed * jitter_buffer_compressed;
    StateJitterBufferUncompressed * jitter_buffer_uncompressed;
    vectorial::vec3f position_error[NumCubes];
    vectorial::quat4f orientation_error[NumCubes];
};

SyncDemo::SyncDemo( core::Allocator & allocator )
{
    InitSyncModes();

    //SetMode( SYNC_MODE_UNCOMPRESSED );
    SetMode( SYNC_MODE_ADAPTIVE_SMOOTHING );
    m_allocator = &allocator;
    m_internal = nullptr;
    m_settings = CORE_NEW( *m_allocator, CubesSettings );
    m_sync = CORE_NEW( *m_allocator, SyncInternal, *m_allocator, sync_mode_data[GetMode()] );
}

SyncDemo::~SyncDemo()
{
    Shutdown();

    CORE_DELETE( *m_allocator, SyncInternal, m_sync );
    CORE_DELETE( *m_allocator, CubesSettings, m_settings );

    m_sync = nullptr;
    m_settings = nullptr;
    m_allocator = nullptr;
}

bool SyncDemo::Initialize()
{
    if ( m_internal )
        Shutdown();

    m_internal = CORE_NEW( *m_allocator, CubesInternal );    

    CubesConfig config;
    
    config.num_simulations = 2;
    config.num_views = 2;
    config.soften_simulation = true;

    m_internal->Initialize( *m_allocator, config, m_settings );

    return true;
}

void SyncDemo::Shutdown()
{
    CORE_ASSERT( m_allocator );

    CORE_ASSERT( m_sync );
    m_sync->Reset( sync_mode_data[GetMode()] );

    if ( m_internal )
    {
        m_internal->Free( *m_allocator );
        CORE_DELETE( *m_allocator, CubesInternal, m_internal );
        m_internal = nullptr;
    }
}

void ClampSnapshot( QuantizedSnapshot_HighPrecision & snapshot )
{
    for ( int i = 0; i < NumCubes; ++i )
    {
        QuantizedCubeState_HighPrecision & cube = snapshot.cubes[i];

        cube.position_x = core::clamp( cube.position_x, -QuantizedPositionBoundXY_HighPrecision, +QuantizedPositionBoundXY_HighPrecision - 1 );
        cube.position_y = core::clamp( cube.position_y, -QuantizedPositionBoundXY_HighPrecision, +QuantizedPositionBoundXY_HighPrecision - 1 );
        cube.position_z = core::clamp( cube.position_z, 0, +QuantizedPositionBoundZ_HighPrecision - 1 );

        cube.linear_velocity_x = core::clamp( cube.linear_velocity_x, -QuantizedLinearVelocityBound_HighPrecision, +QuantizedLinearVelocityBound_HighPrecision - 1 );
        cube.linear_velocity_y = core::clamp( cube.linear_velocity_y, -QuantizedLinearVelocityBound_HighPrecision, +QuantizedLinearVelocityBound_HighPrecision - 1 );
        cube.linear_velocity_z = core::clamp( cube.linear_velocity_z, -QuantizedLinearVelocityBound_HighPrecision, +QuantizedLinearVelocityBound_HighPrecision - 1 );

        cube.angular_velocity_x = core::clamp( cube.angular_velocity_x, -QuantizedAngularVelocityBound_HighPrecision, +QuantizedAngularVelocityBound_HighPrecision - 1 );
        cube.angular_velocity_y = core::clamp( cube.angular_velocity_y, -QuantizedAngularVelocityBound_HighPrecision, +QuantizedAngularVelocityBound_HighPrecision - 1 );
        cube.angular_velocity_z = core::clamp( cube.angular_velocity_z, -QuantizedAngularVelocityBound_HighPrecision, +QuantizedAngularVelocityBound_HighPrecision - 1 );
    }
}

void ApplySnapshot( GameInstance & game_instance, QuantizedSnapshot_HighPrecision & snapshot )
{
    for ( int i = 0; i < NumCubes; ++i )
    {
        const int id = i + 1;

        hypercube::ActiveObject * active_object = game_instance.FindActiveObject( id );

        if ( active_object )
        {
            CubeState cube;
            snapshot.cubes[i].Save( cube );

            active_object->position = math::Vector( cube.position.x(), cube.position.y(), cube.position.z() );
            active_object->orientation = math::Quaternion( cube.orientation.w(), cube.orientation.x(), cube.orientation.y(), cube.orientation.z() );
            active_object->linearVelocity = math::Vector( cube.linear_velocity.x(), cube.linear_velocity.y(), cube.linear_velocity.z() );
            active_object->angularVelocity = math::Vector( cube.angular_velocity.x(), cube.angular_velocity.y(), cube.angular_velocity.z() );

            game_instance.MoveActiveObject( active_object );
        }
    }
}

void ApplyStateUpdateUncompressed( GameInstance & game_instance, const StateUpdateUncompressed & state_update, vectorial::vec3f * position_error, vectorial::quat4f * orientation_error )
{
    for ( int i = 0; i < state_update.num_cubes; ++i )
    {
        const int id = state_update.cube_index[i] + 1;

        hypercube::ActiveObject * active_object = game_instance.FindActiveObject( id );

        if ( active_object )
        {
            const CubeState & cube = state_update.cube_state[i];

            vectorial::vec3f old_position( active_object->position.x, active_object->position.y, active_object->position.z );
            vectorial::vec3f new_position = cube.position;

            position_error[id] = ( old_position + position_error[id] ) - new_position;  

            vectorial::quat4f old_orientation( active_object->orientation.x, active_object->orientation.y, active_object->orientation.z, active_object->orientation.w );
            vectorial::quat4f new_orientation = cube.orientation;

            orientation_error[id] = vectorial::conjugate( new_orientation ) * ( old_orientation * orientation_error[id] );

            active_object->position = math::Vector( cube.position.x(), cube.position.y(), cube.position.z() );
            active_object->orientation = math::Quaternion( cube.orientation.w(), cube.orientation.x(), cube.orientation.y(), cube.orientation.z() );
            active_object->linearVelocity = math::Vector( cube.linear_velocity.x(), cube.linear_velocity.y(), cube.linear_velocity.z() );
            active_object->angularVelocity = math::Vector( cube.angular_velocity.x(), cube.angular_velocity.y(), cube.angular_velocity.z() );
            active_object->authority = cube.interacting ? 0 : MaxPlayers;
            active_object->enabled = !cube.AtRest();

            game_instance.MoveActiveObject( active_object );
        }
    }
}

void ApplyStateUpdateCompressed( GameInstance & game_instance, const StateUpdateCompressed & state_update, vectorial::vec3f * position_error, vectorial::quat4f * orientation_error )
{
    for ( int i = 0; i < state_update.num_cubes; ++i )
    {
        const int id = state_update.cube_index[i] + 1;

        hypercube::ActiveObject * active_object = game_instance.FindActiveObject( id );

        if ( active_object )
        {
            CubeState cube;

            state_update.cube_state[i].Save( cube );

            vectorial::vec3f old_position( active_object->position.x, active_object->position.y, active_object->position.z );
            vectorial::vec3f new_position = cube.position;

            position_error[id] = ( old_position + position_error[id] ) - new_position;  

            vectorial::quat4f old_orientation( active_object->orientation.x, active_object->orientation.y, active_object->orientation.z, active_object->orientation.w );
            vectorial::quat4f new_orientation = cube.orientation;

            orientation_error[id] = vectorial::conjugate( new_orientation ) * ( old_orientation * orientation_error[id] );

            active_object->position = math::Vector( cube.position.x(), cube.position.y(), cube.position.z() );
            active_object->orientation = math::Quaternion( cube.orientation.w(), cube.orientation.x(), cube.orientation.y(), cube.orientation.z() );
            active_object->linearVelocity = math::Vector( cube.linear_velocity.x(), cube.linear_velocity.y(), cube.linear_velocity.z() );
            active_object->angularVelocity = math::Vector( cube.angular_velocity.x(), cube.angular_velocity.y(), cube.angular_velocity.z() );
            active_object->authority = cube.interacting ? 0 : MaxPlayers;
            active_object->enabled = !state_update.cube_state[i].AtRest();

            game_instance.MoveActiveObject( active_object );
        }
    }
}

void CalculateCubePriorities_Uncompressed( float * priority, Snapshot & snapshot )
{
    const float BasePriority = 1.0f;
    const float PlayerPriority = 1000000.0f;
    const float InteractingPriority = 100.0f;

    for ( int i = 0; i < NumCubes; ++i )
    {
        priority[i] = BasePriority;

        if ( i == 0 )
            priority[i] += PlayerPriority;

        if ( snapshot.cubes[i].interacting )
            priority[i] += InteractingPriority;
    }
}

void CalculateCubePriorities_Compressed( float * priority, QuantizedSnapshot_HighPrecision & snapshot )
{
    const float BasePriority = 1.0f;
    const float PlayerPriority = 1000000.0f;
    const float InteractingPriority = 100.0f;

    for ( int i = 0; i < NumCubes; ++i )
    {
        priority[i] = BasePriority;

        if ( i == 0 )
            priority[i] += PlayerPriority;

        if ( snapshot.cubes[i].interacting )
            priority[i] += InteractingPriority;
    }
}

bool priority_sort_function( const CubePriorityInfo & a, const CubePriorityInfo & b ) { return a.accum > b.accum; }

struct SendCubeInfo
{
    int index;
    bool send;
};

void MeasureCubesToSend_Uncompressed( Snapshot & snapshot, SendCubeInfo * send_cubes, int max_bytes )
{
    const int max_bits = max_bytes * 8;

    int bits = 0;

    for ( int i = 0; i < MaxCubesPerPacket; ++i )
    {
        protocol::MeasureStream stream( max_bytes * 2 );

        int id = send_cubes[i].index;

        serialize_cube_state_uncompressed( stream, id, snapshot.cubes[id] );

        const int bits_processed = stream.GetBitsProcessed();

        if ( bits + bits_processed < max_bits )
        {
            send_cubes[i].send = true;
            bits += bits_processed;
        }
    }
}

void MeasureCubesToSend_Compressed( QuantizedSnapshot_HighPrecision & snapshot, SendCubeInfo * send_cubes, int max_bytes )
{
    const int max_bits = max_bytes * 8;

    int bits = 0;

    for ( int i = 0; i < MaxCubesPerPacket; ++i )
    {
        protocol::MeasureStream stream( max_bytes * 2 );

        int id = send_cubes[i].index;

        serialize_cube_state_compressed( stream, id, snapshot.cubes[id] );

        const int bits_processed = stream.GetBitsProcessed();

        if ( bits + bits_processed < max_bits )
        {
            send_cubes[i].send = true;
            bits += bits_processed;
        }
    }
}

void SyncDemo::Update()
{
    // quantize and clamp simulation state if necessary

    Snapshot left_snapshot_uncompressed;

    QuantizedSnapshot_HighPrecision left_snapshot_compressed;

    if ( GetMode() == SYNC_MODE_UNCOMPRESSED )
    {
        GetSnapshot( m_internal->GetGameInstance( 0 ), left_snapshot_uncompressed );
    }
    else
    {
        GetQuantizedSnapshot_HighPrecision( m_internal->GetGameInstance( 0 ), left_snapshot_compressed );

        ClampSnapshot( left_snapshot_compressed );

        if ( GetMode() >= SYNC_MODE_QUANTIZE_ON_BOTH_SIDES )
            ApplySnapshot( *m_internal->simulation[0].game_instance, left_snapshot_compressed );
    }

    // quantize and clamp right simulation state if necessary

    if ( GetMode() >= SYNC_MODE_QUANTIZE_ON_BOTH_SIDES )
    {
        QuantizedSnapshot_HighPrecision right_snapshot;

        GetQuantizedSnapshot_HighPrecision( m_internal->GetGameInstance( 1 ), right_snapshot );

        ClampSnapshot( right_snapshot );

        ApplySnapshot( *m_internal->simulation[1].game_instance, right_snapshot );
    }

    // calculate cube priorities and determine which cubes to send in packet

    float priority[NumCubes];

    if ( GetMode() == SYNC_MODE_UNCOMPRESSED )
        CalculateCubePriorities_Uncompressed( priority, left_snapshot_uncompressed );
    else
        CalculateCubePriorities_Compressed( priority, left_snapshot_compressed );

    for ( int i = 0; i < NumCubes; ++i )
        m_sync->priority_info[i].accum += global.timeBase.deltaTime * priority[i];

    CubePriorityInfo priority_info[NumCubes];

    memcpy( priority_info, m_sync->priority_info, sizeof( CubePriorityInfo ) * NumCubes );

    std::sort( priority_info, priority_info + NumCubes, priority_sort_function );

    SendCubeInfo send_cubes[MaxCubesPerPacket];

    for ( int i = 0; i < MaxCubesPerPacket; ++i )
    {
        send_cubes[i].index = priority_info[i].index;
        send_cubes[i].send = false;
    }

    const int MaxCubeBytes = 500;

    if ( GetMode() == SYNC_MODE_UNCOMPRESSED )
        MeasureCubesToSend_Uncompressed( left_snapshot_uncompressed, send_cubes, MaxCubeBytes );
    else
        MeasureCubesToSend_Compressed( left_snapshot_compressed, send_cubes, MaxCubeBytes );

    int num_cubes_to_send = 0;

    for ( int i = 0; i < MaxCubesPerPacket; i++ )
    {
        if ( send_cubes[i].send )
        {
            m_sync->priority_info[send_cubes[i].index].accum = 0.0;
            num_cubes_to_send++;
        }
    }

    // construct state packet containing cubes to be sent

    auto local_input = m_internal->GetLocalInput();

    if ( GetMode() == SYNC_MODE_UNCOMPRESSED )
    {
        auto state_packet = (StatePacketUncompressed*) m_sync->packet_factory.Create( SYNC_STATE_PACKET_UNCOMPRESSED );

        state_packet->state_update.input = local_input;
        state_packet->state_update.sequence = m_sync->send_sequence;

        state_packet->state_update.num_cubes = num_cubes_to_send;
        int j = 0;    
        for ( int i = 0; i < MaxCubesPerPacket; ++i )
        {
            if ( send_cubes[i].send )
            {
                state_packet->state_update.cube_index[j] = send_cubes[i].index;
                state_packet->state_update.cube_state[j] = left_snapshot_uncompressed.cubes[ send_cubes[i].index ];
                j++;
            }
        }
        CORE_ASSERT( j == num_cubes_to_send );

        m_sync->network_simulator->SendPacket( network::Address( "::1", RightPort ), state_packet );

        m_sync->send_sequence++;
    }
    else
    {
        auto state_packet = (StatePacketCompressed*) m_sync->packet_factory.Create( SYNC_STATE_PACKET_COMPRESSED );

        state_packet->state_update.input = local_input;
        state_packet->state_update.sequence = m_sync->send_sequence;

        state_packet->state_update.num_cubes = num_cubes_to_send;
        int j = 0;    
        for ( int i = 0; i < MaxCubesPerPacket; ++i )
        {
            if ( send_cubes[i].send )
            {
                state_packet->state_update.cube_index[j] = send_cubes[i].index;
                state_packet->state_update.cube_state[j] = left_snapshot_compressed.cubes[ send_cubes[i].index ];
                j++;
            }
        }
        CORE_ASSERT( j == num_cubes_to_send );

        m_sync->network_simulator->SendPacket( network::Address( "::1", RightPort ), state_packet );

        m_sync->send_sequence++;
    }

    // update the network simulator

    m_sync->network_simulator->Update( global.timeBase );

    // receive packets from the simulator (with latency, packet loss and jitter applied...)

    while ( true )
    {
        auto packet = m_sync->network_simulator->ReceivePacket();
        if ( !packet )
            break;

        if ( !m_sync->disable_packets )
        {
            const auto port = packet->GetAddress().GetPort();
            const auto type = packet->GetType();

            if ( type == SYNC_STATE_PACKET_UNCOMPRESSED && port == RightPort )
            {
                auto state_packet = (StatePacketUncompressed*) packet;
                m_sync->jitter_buffer_uncompressed->AddStateUpdate( global.timeBase.time, state_packet->state_update.sequence, state_packet->state_update );
            }

            if ( type == SYNC_STATE_PACKET_COMPRESSED && port == RightPort )
            {
                auto state_packet = (StatePacketCompressed*) packet;
                m_sync->jitter_buffer_compressed->AddStateUpdate( global.timeBase.time, state_packet->state_update.sequence, state_packet->state_update );
            }
        }

        m_sync->packet_factory.Destroy( packet );
    }

    // push state update to right simulation if available

    if ( GetMode() == SYNC_MODE_UNCOMPRESSED )
    {
        StateUpdateUncompressed state_update_uncompressed;

        if ( m_sync->jitter_buffer_uncompressed->GetStateUpdate( global.timeBase.time, state_update_uncompressed ) )
        {
            m_sync->remote_input = state_update_uncompressed.input;

            ApplyStateUpdateUncompressed( *m_internal->simulation[1].game_instance, state_update_uncompressed, m_sync->position_error, m_sync->orientation_error );
        }
    }
    else
    {
        StateUpdateCompressed state_update_compressed;

        if ( m_sync->jitter_buffer_compressed->GetStateUpdate( global.timeBase.time, state_update_compressed ) )
        {
            m_sync->remote_input = state_update_compressed.input;

            ApplyStateUpdateCompressed( *m_internal->simulation[1].game_instance, state_update_compressed, m_sync->position_error, m_sync->orientation_error );
        }
    }

    // run the simulation

    CubesUpdateConfig update_config;

    update_config.sim[0].num_frames = 1;
    update_config.sim[0].frame_input[0] = local_input;

    const bool right_side_running = ( GetMode() == SYNC_MODE_UNCOMPRESSED ) 
        ? 
          m_sync->jitter_buffer_uncompressed->IsRunning( global.timeBase.time ) 
        : 
          m_sync->jitter_buffer_compressed->IsRunning( global.timeBase.time );

    update_config.sim[1].num_frames = right_side_running ? 1 : 0;
    update_config.sim[1].frame_input[0] = m_sync->remote_input;

    m_internal->Update( update_config );

    // reduce position and orientation error

    if ( GetMode() == SYNC_MODE_BASIC_SMOOTHING )
    {
        static const float Tightness = 0.975f;

        const vectorial::quat4f identity = vectorial::quat4f::identity();

        for ( int i = 0; i < NumCubes; ++i )
        {
            const float position_error_dist_squared = vectorial::length_squared( m_sync->position_error[i] );

            if ( position_error_dist_squared >= 0.000001f )
            {
                m_sync->position_error[i] *= Tightness;
            }
            else
            {
                m_sync->position_error[i] = vectorial::vec3f(0,0,0);
            }

            if ( vectorial::dot( m_sync->orientation_error[i], identity ) < 0 )
                m_sync->orientation_error[i] = -m_sync->orientation_error[i];

            const float orientation_error_dot = fabs( vectorial::dot( m_sync->orientation_error[i], vectorial::quat4f::identity() ) );

            if ( orientation_error_dot > 0.000001f )
            {
                m_sync->orientation_error[i] = vectorial::slerp( 1.0f - Tightness, m_sync->orientation_error[i], identity );
            }
            else
            {
                m_sync->orientation_error[i] = identity;
            }
        }
    }
    else if ( GetMode() >= SYNC_MODE_ADAPTIVE_SMOOTHING )
    {
        static const float TightnessA = 0.95f;
        static const float TightnessB = 0.85f;

        static const float PositionA = 0.25f;
        static const float PositionB = 1.0f;

        static const float OrientationA = 0.2f;
        static const float OrientationB = 0.5f;

        const vectorial::quat4f identity = vectorial::quat4f::identity();

        for ( int i = 0; i < NumCubes; ++i )
        {
            const float position_error_dist_squared = vectorial::length_squared( m_sync->position_error[i] );

            if ( position_error_dist_squared >= 0.000001f )
            {
                const float position_error_dist = sqrtf( position_error_dist_squared );

                float tightness = TightnessA;

                if ( position_error_dist > PositionA && position_error_dist < PositionB )
                {
                    const float alpha = ( position_error_dist - PositionA ) / ( PositionB - PositionA );
                    
                    tightness = TightnessA * ( 1.0f - alpha ) + TightnessB * ( alpha );
                }
                else if ( position_error_dist_squared >= PositionB )
                {
                    tightness = TightnessB;
                }

                m_sync->position_error[i] *= tightness;
            }
            else
            {
                 m_sync->position_error[i] = vectorial::vec3f(0,0,0);
            }

            if ( vectorial::dot( m_sync->orientation_error[i], identity ) < 0 )
                 m_sync->orientation_error[i] = -m_sync->orientation_error[i];

            const float orientation_error_dot = fabs( vectorial::dot( m_sync->orientation_error[i], vectorial::quat4f::identity() ) );

            if ( orientation_error_dot > 0.000001f )
            {
                float tightness = TightnessA;

                if ( orientation_error_dot > PositionA && orientation_error_dot < OrientationB )
                {
                    const float alpha = ( orientation_error_dot - OrientationA ) / ( OrientationB - OrientationA );
                    
                    tightness = TightnessA * ( 1.0f - alpha ) + TightnessB * ( alpha );
                }
                else if ( orientation_error_dot >= OrientationB )
                {
                    tightness = TightnessB;
                }

                m_sync->orientation_error[i] = vectorial::slerp( 1.0f - tightness, m_sync->orientation_error[i], identity );
            }
            else
            {
                m_sync->orientation_error[i] = identity;
            }
        }
    }
}

bool SyncDemo::Clear()
{
    return m_internal->Clear();
}

void SyncDemo::Render()
{
    // render cube simulations

    CubesRenderConfig render_config;

    render_config.render_mode = CUBES_RENDER_SPLITSCREEN;

    if ( GetMode() >= SYNC_MODE_BASIC_SMOOTHING )
    {
        render_config.view[1].position_error = m_sync->position_error;
        render_config.view[1].orientation_error = m_sync->orientation_error;
    }

    m_internal->Render( render_config );

    // render text overlays

    Font * font = global.fontManager->GetFont( "Bandwidth" );
    if ( font )
    {
        if ( m_sync->disable_packets )
        {
            // render sync disabled

            const char *sync_string = "*** Sync disabled! ***";
            const float text_x = ( global.displayWidth - font->GetTextWidth( sync_string ) ) / 2;
            const float text_y = 5;
            font->Begin();
            font->DrawText( text_x, text_y, sync_string, Color( 1.0f, 0.0f, 0.0f ) );
            font->End();
        }
        else
        {
            // render bandwidth overlay

            const float bandwidth = m_sync->network_simulator->GetBandwidth();

            char bandwidth_string[256];
            if ( bandwidth < 1024 )
                snprintf( bandwidth_string, (int) sizeof( bandwidth_string ), "Bandwidth: %d kbps", (int) bandwidth );
            else
                snprintf( bandwidth_string, (int) sizeof( bandwidth_string ), "Bandwidth: %.2f mbps", bandwidth / 1000 );

            {
                const float text_x = ( global.displayWidth - font->GetTextWidth( bandwidth_string ) ) / 2;
                const float text_y = 5;
                font->Begin();
                font->DrawText( text_x, text_y, bandwidth_string, Color( 0.27f,0.81f,1.0f ) );
                font->End();
            }
        }
    }
}

bool SyncDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    if ( key == GLFW_KEY_ENTER && action == GLFW_RELEASE )
    {
        m_sync->disable_packets = !m_sync->disable_packets;
    }

    return m_internal->KeyEvent( key, scancode, action, mods );
}

bool SyncDemo::CharEvent( unsigned int code )
{
    return false;
}

int SyncDemo::GetNumModes() const
{
    return SYNC_NUM_MODES;
}

const char * SyncDemo::GetModeDescription( int mode ) const
{
    return sync_mode_descriptions[mode];
}

#endif // #ifdef CLIENT
