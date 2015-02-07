#include "StatefulDemo.h"

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

static const int MaxCubesPerPacket = 31;
static const int RightPort = 1001;

enum StatefulMode
{
    STATEFUL_MODE_DEFAULT,
    STATEFUL_NUM_MODES
};

const char * stateful_mode_descriptions[]
{
    "Default",
};

struct StatefulModeData
{
    int bandwidth = 64 * 1000;                      // 64 kbps bandwidth by default
    float playout_delay = 0.035f;                   // handle +/- two frames jitter @ 60 fps
    float latency = 0.0f;
    float packet_loss = 5.0f;
    float jitter = 2 * 1 / 60.0f;
};

static StatefulModeData stateful_mode_data[STATEFUL_NUM_MODES];

static void InitStatefulModes()
{
    // ...
}

enum StatefulPackets
{
    STATE_PACKET,
    STATE_NUM_PACKETS
};

struct CubeData : public QuantizedCubeStateWithVelocity
{
    int index;
};

struct StateUpdate
{
    int num_cubes = 0;
    int cube_index[MaxCubesPerPacket];
    QuantizedCubeStateWithVelocity cube_state[MaxCubesPerPacket];
};

struct StatePacket : public protocol::Packet
{
    uint16_t sequence;

    StateUpdate state;

    StatePacket() : Packet( STATE_PACKET )
    {
        sequence = 0;
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_uint16( stream, sequence );

        serialize_int( stream, state.num_cubes, 0, MaxCubesPerPacket );

        for ( int i = 0; i < state.num_cubes; ++i )
        {
            serialize_int( stream, state.cube_index[i], 0, NumCubes - 1 );

            serialize_int( stream, state.cube_state[i].position_x, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
            serialize_int( stream, state.cube_state[i].position_y, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
            serialize_int( stream, state.cube_state[i].position_z, 0, QuantizedPositionBoundZ - 1 );

            serialize_object( stream, state.cube_state[i].orientation );

            bool at_rest = false;

            if ( Stream::IsWriting )
            {
                if ( state.cube_state[i].linear_velocity_x == 0 && state.cube_state[i].linear_velocity_y == 0 && state.cube_state[i].linear_velocity_z == 0 &&
                     state.cube_state[i].angular_velocity_x == 0 && state.cube_state[i].angular_velocity_y == 0 && state.cube_state[i].angular_velocity_z == 0 )
                {
                    at_rest = true;
                }
            }
            
            serialize_bool( stream, at_rest );

            if ( at_rest )
            {
                serialize_int( stream, state.cube_state[i].linear_velocity_x, -QuantizedLinearVelocityBound, +QuantizedLinearVelocityBound - 1 );
                serialize_int( stream, state.cube_state[i].linear_velocity_y, -QuantizedLinearVelocityBound, +QuantizedLinearVelocityBound - 1 );
                serialize_int( stream, state.cube_state[i].linear_velocity_z, -QuantizedLinearVelocityBound, +QuantizedLinearVelocityBound - 1 );

                serialize_int( stream, state.cube_state[i].angular_velocity_x, -QuantizedAngularVelocityBound, +QuantizedAngularVelocityBound - 1 );
                serialize_int( stream, state.cube_state[i].angular_velocity_y, -QuantizedAngularVelocityBound, +QuantizedAngularVelocityBound - 1 );
                serialize_int( stream, state.cube_state[i].angular_velocity_z, -QuantizedAngularVelocityBound, +QuantizedAngularVelocityBound - 1 );
            }
            else if ( Stream::IsReading )
            {
                state.cube_state[i].linear_velocity_x = 0;
                state.cube_state[i].linear_velocity_y = 0;
                state.cube_state[i].linear_velocity_z = 0;

                state.cube_state[i].angular_velocity_x = 0;
                state.cube_state[i].angular_velocity_y = 0;
                state.cube_state[i].angular_velocity_z = 0;
            }
        }
    }
};

class StatePacketFactory : public protocol::PacketFactory
{
    core::Allocator * m_allocator;

public:

    StatePacketFactory( core::Allocator & allocator )
        : PacketFactory( allocator, STATE_NUM_PACKETS )
    {
        m_allocator = &allocator;
    }

protected:

    protocol::Packet * CreateInternal( int type )
    {
        switch ( type )
        {
            case STATE_PACKET:   return CORE_NEW( *m_allocator, StatePacket );
            default:
                return nullptr;
        }
    }
};

struct CubePriorityInfo
{
    int index;
    float accum;
};

struct StateJitterBuffer
{
    StateJitterBuffer( core::Allocator & allocator, const StatefulModeData & mode_data )
    //    : snapshots( allocator, NumJitterBufferEntries )
    {
        stopped = true;
        start_time = 0.0;
        playout_delay = mode_data.playout_delay;
    }

    void AddState( double time, uint16_t sequence, const StateUpdate & state )
    {
        /*
        CORE_ASSERT( cube_state > 0 );

        if ( stopped )
        {
            start_time = time;
            stopped = false;
        }

        auto entry = snapshots.Insert( sequence );

        if ( entry )
            memcpy( entry->cubes, cube_state, sizeof( entry->cubes ) );
            */
    }

    void GetViewUpdate( const SnapshotModeData & mode_data, double time, view::ObjectUpdate * object_update, int & num_object_updates )
    {
#if 0

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

        if ( time >= interpolation_end_time + 0.0001f )
            return;

        // we are in a valid interpolation, calculate t by looking at current time 
        // relative to interpolation start/end times and perform the interpolation.

        const float t = core::clamp( ( time - interpolation_start_time ) / ( interpolation_end_time - interpolation_start_time ), 0.0, 1.0 );

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

#endif
    }

    void Reset()
    {
        stopped = true;
        start_time = 0.0;
//        snapshots.Reset();
    }

    bool stopped;
    double start_time;
    float playout_delay;
//    protocol::SequenceBuffer<Snapshot> snapshots;
};

struct StatefulInternal
{
    StatefulInternal( core::Allocator & allocator, const StatefulModeData & mode_data ) 
        : packet_factory( allocator )
    {
        this->allocator = &allocator;
        network::SimulatorConfig networkSimulatorConfig;
        networkSimulatorConfig.packetFactory = &packet_factory;
        networkSimulatorConfig.maxPacketSize = 1024;
        network_simulator = CORE_NEW( allocator, network::Simulator, networkSimulatorConfig );
        Reset( mode_data );
    }

    ~StatefulInternal()
    {
        CORE_ASSERT( network_simulator );
        typedef network::Simulator NetworkSimulator;
        CORE_DELETE( *allocator, NetworkSimulator, network_simulator );
        network_simulator = nullptr;
    }

    void Reset( const StatefulModeData & mode_data )
    {
        network_simulator->Reset();
        network_simulator->ClearStates();
        network_simulator->AddState( { mode_data.latency, mode_data.jitter, mode_data.packet_loss } );
        send_sequence = 0;
        for ( int i = 0; i < NumCubes; ++i )
        {
            priority_info[i].index = i;
            priority_info[i].accum = 0.0f;
        }
    }

    core::Allocator * allocator;
    uint16_t send_sequence;
    game::Input remote_input;
    network::Simulator * network_simulator;
    StatePacketFactory packet_factory;
    CubePriorityInfo priority_info[NumCubes];
};

StatefulDemo::StatefulDemo( core::Allocator & allocator )
{
    InitStatefulModes();

    m_allocator = &allocator;
    m_internal = nullptr;
    m_settings = CORE_NEW( *m_allocator, CubesSettings );
    m_stateful = CORE_NEW( *m_allocator, StatefulInternal, *m_allocator, stateful_mode_data[GetMode()] );
}

StatefulDemo::~StatefulDemo()
{
    Shutdown();

    CORE_DELETE( *m_allocator, StatefulInternal, m_stateful );
    CORE_DELETE( *m_allocator, CubesSettings, m_settings );

    m_stateful = nullptr;
    m_settings = nullptr;
    m_allocator = nullptr;
}

bool StatefulDemo::Initialize()
{
    if ( m_internal )
        Shutdown();

    m_internal = CORE_NEW( *m_allocator, CubesInternal );    

    CubesConfig config;
    
    config.num_simulations = 2;
    config.num_views = 2;

    m_internal->Initialize( *m_allocator, config, m_settings );

    return true;
}

void StatefulDemo::Shutdown()
{
    CORE_ASSERT( m_allocator );

    CORE_ASSERT( m_stateful );
    m_stateful->Reset( stateful_mode_data[GetMode()] );

    if ( m_internal )
    {
        m_internal->Free( *m_allocator );
        CORE_DELETE( *m_allocator, CubesInternal, m_internal );
        m_internal = nullptr;
    }
}

void ClampSnapshot( QuantizedSnapshotWithVelocity & snapshot )
{
    for ( int i = 0; i < NumCubes; ++i )
    {
        QuantizedCubeStateWithVelocity & cube = snapshot.cubes[i];

        cube.position_x = core::clamp( cube.position_x, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
        cube.position_y = core::clamp( cube.position_y, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
        cube.position_z = core::clamp( cube.position_z, 0, +QuantizedPositionBoundZ - 1 );

        cube.linear_velocity_x = core::clamp( cube.linear_velocity_x, -QuantizedLinearVelocityBound, +QuantizedLinearVelocityBound - 1 );
        cube.linear_velocity_y = core::clamp( cube.linear_velocity_y, -QuantizedLinearVelocityBound, +QuantizedLinearVelocityBound - 1 );
        cube.linear_velocity_z = core::clamp( cube.linear_velocity_z, -QuantizedLinearVelocityBound, +QuantizedLinearVelocityBound - 1 );

        cube.angular_velocity_x = core::clamp( cube.angular_velocity_x, -QuantizedAngularVelocityBound, +QuantizedAngularVelocityBound - 1 );
        cube.angular_velocity_y = core::clamp( cube.angular_velocity_y, -QuantizedAngularVelocityBound, +QuantizedAngularVelocityBound - 1 );
        cube.angular_velocity_z = core::clamp( cube.angular_velocity_z, -QuantizedAngularVelocityBound, +QuantizedAngularVelocityBound - 1 );
    }
}

void ApplySnapshot( CubesView & view, QuantizedSnapshotWithVelocity & snapshot )
{
    view::ObjectUpdate object_updates[NumCubes];

    int num_object_updates = NumCubes;

    for ( int i = 0; i < NumCubes; ++i )
    {
        CubeState cube;
        snapshot.cubes[i].Save( cube );
        object_updates[i].id = i + 1;
        object_updates[i].position = cube.position;
        object_updates[i].orientation = cube.orientation;
        object_updates[i].scale = ( i == 0 ) ? hypercube::PlayerCubeSize : hypercube::NonPlayerCubeSize;
        object_updates[i].authority = cube.interacting ? 0 : MaxPlayers;
        object_updates[i].visible = true;
    }

    view.objects.UpdateObjects( object_updates, num_object_updates );
}

void CalculateCubePriorities( float * priority, QuantizedSnapshotWithVelocity & snapshot )
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

void MeasureCubesToSend( QuantizedSnapshotWithVelocity & snapshot, SendCubeInfo * send_cubes, int max_bytes )
{
    // todo: actually serialize measure cubes to send that fit in given number of bytes
    for ( int i = 0; i < MaxCubesPerPacket; ++i )
    {
        send_cubes[i].send = true;
    }
}

void StatefulDemo::Update()
{
    CubesUpdateConfig update_config;

    auto local_input = m_internal->GetLocalInput();

    // setup left simulation to update one frame with local input

    update_config.sim[0].num_frames = 1;
    update_config.sim[0].frame_input[0] = local_input;

    // setup right simulation to update one frame with remote input

    update_config.sim[1].num_frames = 1;
    update_config.sim[1].frame_input[0] = m_stateful->remote_input;

    // quantize and clamp left simulation state

    QuantizedSnapshotWithVelocity left_snapshot;

    GetQuantizedSnapshotWithVelocity( m_internal->GetGameInstance( 0 ), left_snapshot );

    ClampSnapshot( left_snapshot );

    ApplySnapshot( m_internal->view[0], left_snapshot );

    // quantize and clamp right simulation state

    QuantizedSnapshotWithVelocity right_snapshot;

    GetQuantizedSnapshotWithVelocity( m_internal->GetGameInstance( 1 ), right_snapshot );

    ClampSnapshot( right_snapshot );

    ApplySnapshot( m_internal->view[1], right_snapshot );

    // calculate cube priorities and determine which cubes to send in packet

    float priority[NumCubes];

    CalculateCubePriorities( priority, left_snapshot );

    for ( int i = 0; i < NumCubes; ++i )
        m_stateful->priority_info[i].accum += global.timeBase.deltaTime * priority[i];

    CubePriorityInfo priority_info[NumCubes];

    memcpy( priority_info, m_stateful->priority_info, sizeof( CubePriorityInfo ) * NumCubes );

    std::sort( priority_info, priority_info + NumCubes, priority_sort_function );

    SendCubeInfo send_cubes[MaxCubesPerPacket];

    for ( int i = 0; i < MaxCubesPerPacket; ++i )
    {
        send_cubes[i].index = priority_info[i].index;
        send_cubes[i].send = false;     // not yet
    }

    const int MaxCubeBytes = 256;           // todo: this should be a function of packet header size and desired amount bandwidth in mode

    MeasureCubesToSend( left_snapshot, send_cubes, MaxCubeBytes );

    int num_cubes_to_send = 0;
    for ( int i = 0; i < MaxCubesPerPacket; i++ )
    {
        if ( send_cubes[i].send )
        {
            m_stateful->priority_info[send_cubes[i].index].accum = 0.0f;
            num_cubes_to_send++;
        }
    }

    // construct state packet containing cubes to be sent

    auto state_packet = (StatePacket*) m_stateful->packet_factory.Create( STATE_PACKET );

    state_packet->sequence = m_stateful->send_sequence;
    state_packet->state.num_cubes = num_cubes_to_send;
    int j = 0;    
    for ( int i = 0; i < MaxCubesPerPacket; ++i )
    {
        if ( send_cubes[i].send )
        {
            state_packet->state.cube_index[j] = send_cubes[i].index;
            state_packet->state.cube_state[j] = left_snapshot.cubes[ send_cubes[i].index ];
            j++;
        }
    }
    CORE_ASSERT( j == num_cubes_to_send );

    m_stateful->network_simulator->SendPacket( network::Address( "::1", RightPort ), state_packet );

    m_stateful->send_sequence++;

    // update the network simulator

    m_stateful->network_simulator->Update( global.timeBase );

    // receive packets from the simulator (with latency, packet loss and jitter applied...)

    while ( true )
    {
        auto packet = m_stateful->network_simulator->ReceivePacket();
        if ( !packet )
            break;

        const auto port = packet->GetAddress().GetPort();
        const auto type = packet->GetType();

        if ( type == STATE_PACKET && port == RightPort )
        {
            auto state_packet = (StatePacket*) packet;

//            printf( "received state packet %d\n", state_packet->sequence );

            // todo: process state packet and copy to jitter buffer
        }

        m_stateful->packet_factory.Destroy( packet );
    }

    // todo: query jitter buffer if a state update is ready this frame

    // todo: if it is, push to right simulation

    // 1. set input
    // 2. set time
    // 3. apply state

    // run the simulation

    m_internal->Update( update_config );
}

bool StatefulDemo::Clear()
{
    return m_internal->Clear();
}

void StatefulDemo::Render()
{
    CubesRenderConfig render_config;

    render_config.render_mode = CUBES_RENDER_SPLITSCREEN;

    m_internal->Render( render_config );

    const float bandwidth = m_stateful->network_simulator->GetBandwidth();

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

bool StatefulDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    return m_internal->KeyEvent( key, scancode, action, mods );
}

bool StatefulDemo::CharEvent( unsigned int code )
{
    // ...

    return false;
}

int StatefulDemo::GetNumModes() const
{
    return STATEFUL_NUM_MODES;
}

const char * StatefulDemo::GetModeDescription( int mode ) const
{
    return stateful_mode_descriptions[mode];
}

#endif // #ifdef CLIENT
