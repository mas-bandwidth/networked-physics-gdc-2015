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
static const int NumStateUpdates = 256;
static const int RightPort = 1001;

enum StatefulMode
{
    STATEFUL_MODE_INPUT_ONLY,
    STATEFUL_MODE_INPUT_DESYNC,
    STATEFUL_MODE_INPUT_AND_STATE,
    STATEFUL_NUM_MODES
};

const char * stateful_mode_descriptions[]
{
    "Input Only",
    "Input Desync",
    "Input and State"
};

struct StatefulModeData
{
    int bandwidth = 256 * 1000;                     // 256 kbps maximum bandwidth
    float playout_delay = 0.035f;                   // handle +/- two frames jitter @ 60 fps
    float latency = 0.0f;
    float packet_loss = 5.0f;
    float jitter = 2 * 1/60.0f;
};

static StatefulModeData stateful_mode_data[STATEFUL_NUM_MODES];

static void InitStatefulModes()
{
    stateful_mode_data[STATEFUL_MODE_INPUT_ONLY].packet_loss = 0.0f;
    stateful_mode_data[STATEFUL_MODE_INPUT_ONLY].jitter = 0.0f;
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
    game::Input input;
    uint16_t sequence = 0;
    int num_cubes = 0;
    int cube_index[MaxCubesPerPacket];
    QuantizedCubeStateWithVelocity cube_state[MaxCubesPerPacket];
};

struct StatePacket : public protocol::Packet
{
    StateUpdate state_update;

    StatePacket() : Packet( STATE_PACKET ) {}

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
            serialize_int( stream, state_update.cube_index[i], 0, NumCubes - 1 );

            serialize_int( stream, state_update.cube_state[i].position_x, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
            serialize_int( stream, state_update.cube_state[i].position_y, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
            serialize_int( stream, state_update.cube_state[i].position_z, 0, QuantizedPositionBoundZ - 1 );

            serialize_object( stream, state_update.cube_state[i].orientation );

            bool at_rest = Stream::IsWriting ? state_update.cube_state[i].AtRest() : false;
            
            serialize_bool( stream, at_rest );

            if ( !at_rest )
            {
                serialize_int( stream, state_update.cube_state[i].linear_velocity_x, -QuantizedLinearVelocityBound, +QuantizedLinearVelocityBound - 1 );
                serialize_int( stream, state_update.cube_state[i].linear_velocity_y, -QuantizedLinearVelocityBound, +QuantizedLinearVelocityBound - 1 );
                serialize_int( stream, state_update.cube_state[i].linear_velocity_z, -QuantizedLinearVelocityBound, +QuantizedLinearVelocityBound - 1 );

                serialize_int( stream, state_update.cube_state[i].angular_velocity_x, -QuantizedAngularVelocityBound, +QuantizedAngularVelocityBound - 1 );
                serialize_int( stream, state_update.cube_state[i].angular_velocity_y, -QuantizedAngularVelocityBound, +QuantizedAngularVelocityBound - 1 );
                serialize_int( stream, state_update.cube_state[i].angular_velocity_z, -QuantizedAngularVelocityBound, +QuantizedAngularVelocityBound - 1 );
            }
            else if ( Stream::IsReading )
            {
                state_update.cube_state[i].linear_velocity_x = 0;
                state_update.cube_state[i].linear_velocity_y = 0;
                state_update.cube_state[i].linear_velocity_z = 0;

                state_update.cube_state[i].angular_velocity_x = 0;
                state_update.cube_state[i].angular_velocity_y = 0;
                state_update.cube_state[i].angular_velocity_z = 0;
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
        : state_updates( allocator, NumStateUpdates )
    {
        stopped = true;
        start_time = 0.0;
        playout_delay = mode_data.playout_delay;
    }

    void AddStateUpdate( double time, uint16_t sequence, const StateUpdate & state_update )
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

    bool GetStateUpdate( double time, StateUpdate & state_update )
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
    protocol::SequenceBuffer<StateUpdate> state_updates;
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
        jitter_buffer = CORE_NEW( allocator, StateJitterBuffer, allocator, mode_data );
        Reset( mode_data );
    }

    ~StatefulInternal()
    {
        CORE_ASSERT( network_simulator );
        typedef network::Simulator NetworkSimulator;
        CORE_DELETE( *allocator, NetworkSimulator, network_simulator );
        CORE_DELETE( *allocator, StateJitterBuffer, jitter_buffer );
        network_simulator = nullptr;
        jitter_buffer = nullptr;
    }

    void Reset( const StatefulModeData & mode_data )
    {
        network_simulator->Reset();
        network_simulator->ClearStates();
        network_simulator->AddState( { mode_data.latency, mode_data.jitter, mode_data.packet_loss } );
        jitter_buffer->Reset();
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
    StateJitterBuffer * jitter_buffer;
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

void ApplySnapshot( GameInstance & game_instance, QuantizedSnapshotWithVelocity & snapshot )
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

void ApplyStateUpdate( GameInstance & game_instance, const StateUpdate & state_update )
{
    for ( int i = 0; i < state_update.num_cubes; ++i )
    {
        const int id = state_update.cube_index[i] + 1;

        hypercube::ActiveObject * active_object = game_instance.FindActiveObject( id );

        if ( active_object )
        {
            CubeState cube;

            state_update.cube_state[i].Save( cube );

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
    // quantize and clamp left simulation state

    QuantizedSnapshotWithVelocity left_snapshot;

    GetQuantizedSnapshotWithVelocity( m_internal->GetGameInstance( 0 ), left_snapshot );

    ClampSnapshot( left_snapshot );

    ApplySnapshot( *m_internal->simulation[0].game_instance, left_snapshot );

    // quantize and clamp right simulation state

    QuantizedSnapshotWithVelocity right_snapshot;

    GetQuantizedSnapshotWithVelocity( m_internal->GetGameInstance( 1 ), right_snapshot );

    ClampSnapshot( right_snapshot );

    ApplySnapshot( *m_internal->simulation[1].game_instance, right_snapshot );

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
        send_cubes[i].send = false;
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

    auto local_input = m_internal->GetLocalInput();

    state_packet->state_update.input = local_input;
    state_packet->state_update.sequence = m_stateful->send_sequence;

    if ( GetMode() >= STATEFUL_MODE_INPUT_AND_STATE )
    {
        state_packet->state_update.num_cubes = num_cubes_to_send;
        int j = 0;    
        for ( int i = 0; i < MaxCubesPerPacket; ++i )
        {
            if ( send_cubes[i].send )
            {
                state_packet->state_update.cube_index[j] = send_cubes[i].index;
                state_packet->state_update.cube_state[j] = left_snapshot.cubes[ send_cubes[i].index ];
                j++;
            }
        }
        CORE_ASSERT( j == num_cubes_to_send );
    }

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

//            printf( "add state update: %d\n", state_packet->sequence );

            m_stateful->jitter_buffer->AddStateUpdate( global.timeBase.time, state_packet->state_update.sequence, state_packet->state_update );
        }

        m_stateful->packet_factory.Destroy( packet );
    }

    // push state update to right simulation if available

    StateUpdate state_update;

    if ( m_stateful->jitter_buffer->GetStateUpdate( global.timeBase.time, state_update ) )
    {
        m_stateful->remote_input = state_update.input;

        ApplyStateUpdate( *m_internal->simulation[1].game_instance, state_update );
    }

    // run the simulation

    CubesUpdateConfig update_config;

    update_config.sim[0].num_frames = 1;
    update_config.sim[0].frame_input[0] = local_input;

    update_config.sim[1].num_frames = m_stateful->jitter_buffer->IsRunning( global.timeBase.time ) ? 1 : 0;
    update_config.sim[1].frame_input[0] = m_stateful->remote_input;

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
