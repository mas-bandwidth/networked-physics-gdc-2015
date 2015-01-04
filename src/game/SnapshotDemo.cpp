#include "SnapshotDemo.h"

#ifdef CLIENT

#include "Cubes.h"
#include "Global.h"
#include "vectorial/vec3f.h"
#include "vectorial/quat4f.h"
#include "protocol/Stream.h"
#include "protocol/SequenceBuffer.h"
#include "protocol/PacketFactory.h"
#include "network/Simulator.h"

static const int MaxPacketSize = 64 * 1024;         // this has to be really large for the worst case!

static const int NumCubes = 900 + MaxPlayers;

static const int NumSnapshots = 64;

static const int RightPort = 1001;

enum SnapshotMode
{
    SNAPSHOT_MODE_NAIVE_60PPS,                  // 1. naive snapshots with uncompressed data no jitter @ 60pps
    SNAPSHOT_MODE_NAIVE_60PPS_JITTER,           // 2. naive snapshots with jitter +/2 frames @ 60pps
    SNAPSHOT_MODE_NAIVE_10PPS,                  // 3. naive snapshots at 10pps with interpolation
    SNAPSHOT_MODE_LINEAR_INTERPOLATION_10PPS,   // 5. linear interpolation at 10pps
    SNAPSHOT_MODE_HERMITE_INTERPOLATION_10PPS,  // 6. hermite interpolation at 10pps
    SNAPSHOT_MODE_HERMITE_EXTRAPOLATION_10PPS,  // 7. hermite extrapolation at 10pps
    SNAPSHOT_NUM_MODES
};

const char * snapshot_mode_descriptions[]
{
    "Snapshots at 60 packets per-second",
    "Snapshots at 60 packets per-second (with jitter)",
    "Snapshots at 10 packets per-second",
    "Linear interpolation at 10 packets per-second",
    "Hermite interpolation at 10 packets per-second",
    "Hermite extrapolation at 10 packets per-second",
};

enum SnapshotInterpolation
{
    SNAPSHOT_INTERPOLATION_NONE,
    SNAPSHOT_INTERPOLATION_LINEAR,
    SNAPSHOT_INTERPOLATION_HERMITE,
    SNAPSHOT_INTERPOLATION_HERMITE_WITH_EXTRAPOLATION
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

static SnapshotModeData snapshot_mode_data[SNAPSHOT_NUM_MODES];

static void InitSnapshotModes()
{
    snapshot_mode_data[SNAPSHOT_MODE_NAIVE_60PPS_JITTER].jitter = 2 * 1.0f / 60.0f;

    snapshot_mode_data[SNAPSHOT_MODE_NAIVE_10PPS].send_rate = 10.0f;
    snapshot_mode_data[SNAPSHOT_MODE_NAIVE_10PPS].jitter = 2 * 1.0f / 60.0f;

    snapshot_mode_data[SNAPSHOT_MODE_LINEAR_INTERPOLATION_10PPS].send_rate = 10.0f;
    snapshot_mode_data[SNAPSHOT_MODE_LINEAR_INTERPOLATION_10PPS].jitter = 2 * 1.0f / 60.0f;
    snapshot_mode_data[SNAPSHOT_MODE_LINEAR_INTERPOLATION_10PPS].packet_loss = 5;
    snapshot_mode_data[SNAPSHOT_MODE_LINEAR_INTERPOLATION_10PPS].interpolation = SNAPSHOT_INTERPOLATION_LINEAR;

    snapshot_mode_data[SNAPSHOT_MODE_HERMITE_INTERPOLATION_10PPS].send_rate = 10.0f;
    snapshot_mode_data[SNAPSHOT_MODE_HERMITE_INTERPOLATION_10PPS].jitter = 2 * 1.0f / 60.0f;
    snapshot_mode_data[SNAPSHOT_MODE_HERMITE_INTERPOLATION_10PPS].packet_loss = 5;
    snapshot_mode_data[SNAPSHOT_MODE_HERMITE_INTERPOLATION_10PPS].interpolation = SNAPSHOT_INTERPOLATION_HERMITE;

    snapshot_mode_data[SNAPSHOT_MODE_HERMITE_EXTRAPOLATION_10PPS].send_rate = 10.0f;
    snapshot_mode_data[SNAPSHOT_MODE_HERMITE_EXTRAPOLATION_10PPS].jitter = 2 * 1.0f / 60.0f;
    snapshot_mode_data[SNAPSHOT_MODE_HERMITE_EXTRAPOLATION_10PPS].packet_loss = 5;
    snapshot_mode_data[SNAPSHOT_MODE_HERMITE_EXTRAPOLATION_10PPS].interpolation = SNAPSHOT_INTERPOLATION_HERMITE_WITH_EXTRAPOLATION;
}

enum SnapshotPackets
{
    SNAPSHOT_NAIVE_PACKET,
    SNAPSHOT_ACK_PACKET,
    SNAPSHOT_NUM_PACKETS
};

#define SERIALIZE_ANGULAR_VELOCITY

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

template <typename Stream> void serialize_vector( Stream & stream, vectorial::vec3f & vector )
{
    float values[3];
    if ( Stream::IsWriting )
    {
        values[0] = vector.x();
        values[1] = vector.y();
        values[2] = vector.z();
    }
    serialize_float( stream, values[0] );
    serialize_float( stream, values[1] );
    serialize_float( stream, values[2] );
    if ( Stream::IsReading )
        vector.load( values );
}

template <typename Stream> void serialize_quaternion( Stream & stream, vectorial::quat4f & quaternion )
{
    float values[4];
    if ( Stream::IsWriting )
    {
        values[0] = quaternion.x();
        values[1] = quaternion.y();
        values[2] = quaternion.z();
        values[3] = quaternion.w();
    }
    serialize_float( stream, values[0] );
    serialize_float( stream, values[1] );
    serialize_float( stream, values[2] );
    serialize_float( stream, values[3] );
    if ( Stream::IsReading )
        quaternion.load( values );
}

struct SnapshotNaivePacket : public protocol::Packet
{
    uint16_t sequence;
    bool has_velocity;

    SnapshotNaivePacket() : Packet( SNAPSHOT_NAIVE_PACKET )
    {
        sequence = 0;
        has_velocity = false;
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_uint16( stream, sequence );
        serialize_bool( stream, has_velocity );
        for ( int i = 0; i < NumCubes; ++i )
        {
            serialize_bool( stream, cubes[i].interacting );
            serialize_vector( stream, cubes[i].position );
            serialize_quaternion( stream, cubes[i].orientation );
            if ( has_velocity )
            {
                serialize_vector( stream, cubes[i].linear_velocity );
#ifdef SERIALIZE_ANGULAR_VELOCITY
                serialize_vector( stream, cubes[i].angular_velocity );
#endif // #ifdef SERIALIZE_ANGULAR_VELOCITY
            }
            else if ( Stream::IsReading )
            {
                cubes[i].linear_velocity.zero();
#ifdef SERIALIZE_ANGULAR_VELOCITY
                cubes[i].angular_velocity.zero();
#endif // #ifdef SERIALIZE_ANGULAR_VELOCITY
            }
        }
    }

    CubeState cubes[NumCubes];
};

struct SnapshotAckPacket : public protocol::Packet
{
    uint16_t ack;

    SnapshotAckPacket() : Packet( SNAPSHOT_ACK_PACKET )
    {
        ack = 0;
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_uint16( stream, ack );
    }
};

class SnapshotPacketFactory : public protocol::PacketFactory
{
    core::Allocator * m_allocator;

public:

    SnapshotPacketFactory( core::Allocator & allocator )
        : PacketFactory( allocator, SNAPSHOT_NUM_PACKETS )
    {
        m_allocator = &allocator;
    }

protected:

    protocol::Packet * CreateInternal( int type )
    {
        switch ( type )
        {
            case SNAPSHOT_NAIVE_PACKET:     return CORE_NEW( *m_allocator, SnapshotNaivePacket );
            case SNAPSHOT_ACK_PACKET:       return CORE_NEW( *m_allocator, SnapshotAckPacket );
            default:
                return nullptr;
        }
    }
};

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

        const double frames_since_start = time * mode_data.send_rate;

        if ( time <= 0 )
            return;

        // if we are interpolating but the interpolation start time is too old,
        // go back to the not interpolating state, so we can find a new start point.

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

struct SnapshotInternal
{
    SnapshotInternal( core::Allocator & allocator, const SnapshotModeData & mode_data ) 
        : packet_factory( allocator ), interpolation_buffer( allocator, mode_data )
    {
        this->allocator = &allocator;
        network::SimulatorConfig networkSimulatorConfig;
        networkSimulatorConfig.packetFactory = &packet_factory;
        network_simulator = CORE_NEW( allocator, network::Simulator, networkSimulatorConfig );
        Reset( mode_data );
    }

    ~SnapshotInternal()
    {
        CORE_ASSERT( network_simulator );
        typedef network::Simulator NetworkSimulator;
        CORE_DELETE( *allocator, NetworkSimulator, network_simulator );
        network_simulator = nullptr;
    }

    void Reset( const SnapshotModeData & mode_data )
    {
        interpolation_buffer.Reset();
        network_simulator->Reset();
        network_simulator->ClearStates();
        network_simulator->AddState( { mode_data.latency, mode_data.jitter, mode_data.packet_loss } );
        send_sequence = 0;
        recv_sequence = 0;
        send_accumulator = 1.0f;
    }

    core::Allocator * allocator;
    SnapshotPacketFactory packet_factory;
    network::Simulator * network_simulator;
    SnapshotInterpolationBuffer interpolation_buffer;
    uint16_t send_sequence;
    uint16_t recv_sequence;
    float send_accumulator;
};

SnapshotDemo::SnapshotDemo( core::Allocator & allocator )
{
    InitSnapshotModes();
    m_allocator = &allocator;
    m_internal = nullptr;
    m_settings = CORE_NEW( *m_allocator, CubesSettings );
    m_snapshot = CORE_NEW( *m_allocator, SnapshotInternal, *m_allocator, snapshot_mode_data[GetMode()] );
}

SnapshotDemo::~SnapshotDemo()
{
    Shutdown();
    CORE_DELETE( *m_allocator, SnapshotInternal, m_snapshot );
    CORE_DELETE( *m_allocator, CubesSettings, m_settings );
    m_snapshot = nullptr;
    m_settings = nullptr;
    m_allocator = nullptr;
}

bool SnapshotDemo::Initialize()
{
    if ( m_internal )
        Shutdown();

    m_internal = CORE_NEW( *m_allocator, CubesInternal );    

    CubesConfig config;
    
    config.num_simulations = 1;
    config.num_views = 2;

    m_internal->Initialize( *m_allocator, config, m_settings );

    return true;
}

void SnapshotDemo::Shutdown()
{
    CORE_ASSERT( m_allocator );

    CORE_ASSERT( m_snapshot );
    m_snapshot->Reset( snapshot_mode_data[GetMode()] );

    if ( m_internal )
    {
        m_internal->Free( *m_allocator );
        CORE_DELETE( *m_allocator, CubesInternal, m_internal );
        m_internal = nullptr;
    }
}

void SnapshotDemo::Update()
{
    CubesUpdateConfig update_config;

    auto local_input = m_internal->GetLocalInput();

    // setup left simulation to update one frame with local input

    update_config.sim[0].num_frames = 1;
    update_config.sim[0].frame_input[0] = local_input;

    // send a snapshot packet to the right simulation

    m_snapshot->send_accumulator += global.timeBase.deltaTime;

    if ( m_snapshot->send_accumulator >= 1.0f / snapshot_mode_data[GetMode()].send_rate )
    {
        m_snapshot->send_accumulator = 0.0f;   

        auto game_instance = m_internal->GetGameInstance(0);

        const int num_active_objects = game_instance->GetNumActiveObjects();

        if ( num_active_objects > 0 )
        {
            auto snapshot_packet = (SnapshotNaivePacket*) m_snapshot->packet_factory.Create( SNAPSHOT_NAIVE_PACKET );

            snapshot_packet->sequence = m_snapshot->send_sequence++;

            snapshot_packet->has_velocity = snapshot_mode_data[GetMode()].interpolation >= SNAPSHOT_INTERPOLATION_HERMITE;

            const hypercube::ActiveObject * active_objects = game_instance->GetActiveObjects();

            CORE_ASSERT( active_objects );

            for ( int i = 0; i < num_active_objects; ++i )
            {
                auto & object = active_objects[i];

                const int index = object.id - 1;

                CORE_ASSERT( index >= 0 );
                CORE_ASSERT( index < NumCubes );

                snapshot_packet->cubes[index].position = vectorial::vec3f( object.position.x, object.position.y, object.position.z );

                snapshot_packet->cubes[index].orientation = vectorial::quat4f( object.orientation.x, 
                                                                               object.orientation.y, 
                                                                               object.orientation.z,
                                                                               object.orientation.w );

                snapshot_packet->cubes[index].linear_velocity = vectorial::vec3f( object.linearVelocity.x, 
                                                                                  object.linearVelocity.y,
                                                                                  object.linearVelocity.z );

#ifdef SERIALIZE_ANGULAR_VELOCITY
                snapshot_packet->cubes[index].angular_velocity = vectorial::vec3f( object.angularVelocity.x, 
                                                                                   object.angularVelocity.y,
                                                                                   object.angularVelocity.z );
#endif // #ifdef SERIALIZE_ANGULAR_VELOCITY

                snapshot_packet->cubes[index].interacting = object.authority == 0;
            }

            m_snapshot->network_simulator->SendPacket( network::Address( "::1", RightPort ), snapshot_packet );
        }
    }

    // update the network simulator

    m_snapshot->network_simulator->Update( global.timeBase );

    // receive packets from the simulator (with latency, packet loss and jitter applied...)

    while ( true )
    {
        auto packet = m_snapshot->network_simulator->ReceivePacket();
        if ( !packet )
            break;

        auto port = packet->GetAddress().GetPort();
        auto type = packet->GetType();

        // IMPORTANT: Make sure we actually serialize read/write the packet.
        // Otherwise we're just passing around pointers to structs. LAME! :D

        uint8_t buffer[MaxPacketSize];

        protocol::WriteStream write_stream( buffer, MaxPacketSize );
        packet->SerializeWrite( write_stream );
        write_stream.Flush();
        CORE_CHECK( !write_stream.IsOverflow() );

        m_snapshot->packet_factory.Destroy( packet );
        packet = nullptr;

        protocol::ReadStream read_stream( buffer, MaxPacketSize );
        auto read_packet = m_snapshot->packet_factory.Create( type );
        read_packet->SerializeRead( read_stream );
        CORE_CHECK( !read_stream.IsOverflow() );

        if ( type == SNAPSHOT_NAIVE_PACKET && port == RightPort )
        {
            // naive snapshot packet for right simulation

            auto snapshot_packet = (SnapshotNaivePacket*) read_packet;

            if ( GetMode() <= SNAPSHOT_MODE_NAIVE_10PPS )
            {
                // no interpolation buffer. just render directly to make a point.

                if ( core::sequence_greater_than( snapshot_packet->sequence, m_snapshot->recv_sequence ) )
                {
                    view::ObjectUpdate updates[NumCubes];

                    for ( int i = 0; i < NumCubes; ++i )
                    {
                        updates[i].id = i + 1;
                        updates[i].authority = snapshot_packet->cubes[i].interacting ? 0 : MaxPlayers;
                        updates[i].position = snapshot_packet->cubes[i].position;
                        updates[i].orientation = snapshot_packet->cubes[i].orientation;
                        updates[i].scale = ( i == 0 ) ? hypercube::PlayerCubeSize : hypercube::NonPlayerCubeSize;
                        updates[i].visible = true;
                    }

                    m_internal->view[1].objects.UpdateObjects( updates, NumCubes );

                    m_snapshot->recv_sequence = snapshot_packet->sequence;
                }
            }
            else
            {
                // send to interpolation buffer

                m_snapshot->interpolation_buffer.AddSnapshot( global.timeBase.time, snapshot_packet->sequence, snapshot_packet->cubes );
            }
        }

        m_snapshot->packet_factory.Destroy( read_packet );
        read_packet = nullptr;
    }

    // if we are an an interpolation mode, we need to grab the view updates for the right side from the interpolation buffer

    if ( GetMode() >= SNAPSHOT_MODE_LINEAR_INTERPOLATION_10PPS )
    {
        int num_object_updates = 0;

        view::ObjectUpdate object_updates[NumCubes];

        m_snapshot->interpolation_buffer.GetViewUpdate( snapshot_mode_data[GetMode()], global.timeBase.time, object_updates, num_object_updates );

        if ( num_object_updates > 0 )
            m_internal->view[1].objects.UpdateObjects( object_updates, num_object_updates );
        else
        {
            if ( m_snapshot->interpolation_buffer.interpolating )
                printf( "no snapshot to interpolate towards!\n" );
        }
    }

    // run the simulation

    m_internal->Update( update_config );
}

bool SnapshotDemo::Clear()
{
    return m_internal->Clear();
}

void SnapshotDemo::Render()
{
    CubesRenderConfig render_config;

    render_config.render_mode = CUBES_RENDER_SPLITSCREEN;

    m_internal->Render( render_config );
}

bool SnapshotDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    return m_internal->KeyEvent( key, scancode, action, mods );
}

bool SnapshotDemo::CharEvent( unsigned int code )
{
    // ...

    return false;
}

int SnapshotDemo::GetNumModes() const
{
    return SNAPSHOT_NUM_MODES;
}

const char * SnapshotDemo::GetModeDescription( int mode ) const
{
    return snapshot_mode_descriptions[mode];
}

#endif // #ifdef CLIENT
