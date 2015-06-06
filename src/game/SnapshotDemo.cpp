#include "SnapshotDemo.h"

#ifdef CLIENT

#include "Cubes.h"
#include "Global.h"
#include "Snapshot.h"
#include "protocol/PacketFactory.h"
#include "network/Simulator.h"

static const int RightPort = 1001;
static const int MaxPacketSize = 64 * 1024;         // this has to be really large for the worst case!

enum SnapshotMode
{
    SNAPSHOT_MODE_NAIVE_60PPS,                      // 1. naive snapshots with uncompressed data no jitter @ 60pps
    SNAPSHOT_MODE_NAIVE_60PPS_JITTER,               // 2. naive snapshots with jitter +/2 frames @ 60pps
    SNAPSHOT_MODE_NAIVE_10PPS,                      // 3. naive snapshots at 10pps with interpolation
    SNAPSHOT_MODE_LINEAR_INTERPOLATION_10PPS,       // 5. linear interpolation at 10pps
    SNAPSHOT_MODE_HERMITE_INTERPOLATION_10PPS,      // 6. hermite interpolation at 10pps
    SNAPSHOT_NUM_MODES
};

const char * snapshot_mode_descriptions[]
{
    "Snapshots at 60 packets per-second",
    "Snapshots at 60 packets per-second (with jitter)",
    "Snapshots at 10 packets per-second",
    "Linear interpolation at 10 packets per-second",
    "Hermite interpolation at 10 packets per-second",
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
}

enum SnapshotPackets
{
    SNAPSHOT_NAIVE_PACKET,
    SNAPSHOT_ACK_PACKET,
    SNAPSHOT_NUM_PACKETS
};

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

struct SnapshotInternal
{
    SnapshotInternal( core::Allocator & allocator, const SnapshotModeData & mode_data ) 
        : packet_factory( allocator ), interpolation_buffer( allocator, mode_data )
    {
        this->allocator = &allocator;
        network::SimulatorConfig networkSimulatorConfig;
        networkSimulatorConfig.packetFactory = &packet_factory;
        networkSimulatorConfig.maxPacketSize = MaxPacketSize;
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

    // send a snapshot packet to the right side

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

        if ( type == SNAPSHOT_NAIVE_PACKET && port == RightPort )
        {
            // naive snapshot packet for right simulation

            auto snapshot_packet = (SnapshotNaivePacket*) packet;

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

        m_snapshot->packet_factory.Destroy( packet );
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
