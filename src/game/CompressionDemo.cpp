#include "CompressionDemo.h"

#ifdef CLIENT

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

enum Context
{
    CONTEXT_SNAPSHOT_SLIDING_WINDOW,                // send snapshots (for serialize write)
    CONTEXT_SNAPSHOT_SEQUENCE_BUFFER,               // recv snapshots (for serialize read)
};

enum CompressionMode
{
    COMPRESSION_MODE_UNCOMPRESSED,
    COMPRESSION_MODE_ORIENTATION,
    COMPRESSION_MODE_LINEAR_VELOCITY,
    COMPRESSION_MODE_AT_REST_FLAG,
    COMPRESSION_MODE_POSITION,
    COMPRESSION_NUM_MODES
};

const char * compression_mode_descriptions[]
{
    "Uncompressed",
    "Orientation",
    "Linear velocity",
    "At rest flag",
    "Position",
};

struct CompressionModeData : public SnapshotModeData
{
    CompressionModeData()
    {
        playout_delay = 0.067f;
        send_rate = 60.0f;
        latency = 0.05f;      // 100ms round trip
        packet_loss = 5.0f;
        jitter = 1.0 / 60.0f;
        interpolation = SNAPSHOT_INTERPOLATION_LINEAR;
    }
};

static CompressionModeData compression_mode_data[COMPRESSION_NUM_MODES];

static void InitCompressionModes()
{
    compression_mode_data[COMPRESSION_MODE_UNCOMPRESSED].interpolation = SNAPSHOT_INTERPOLATION_HERMITE;
    compression_mode_data[COMPRESSION_MODE_ORIENTATION].interpolation = SNAPSHOT_INTERPOLATION_HERMITE;
    compression_mode_data[COMPRESSION_MODE_LINEAR_VELOCITY].interpolation = SNAPSHOT_INTERPOLATION_HERMITE;
    compression_mode_data[COMPRESSION_MODE_AT_REST_FLAG].interpolation = SNAPSHOT_INTERPOLATION_HERMITE;
}

typedef protocol::SlidingWindow<Snapshot> SnapshotSlidingWindow;
typedef protocol::SequenceBuffer<Snapshot> SnapshotSequenceBuffer;

typedef protocol::SlidingWindow<QuantizedSnapshot> QuantizedSnapshotSlidingWindow;
typedef protocol::SequenceBuffer<QuantizedSnapshot> QuantizedSnapshotSequenceBuffer;

enum CompressionPackets
{
    COMPRESSION_SNAPSHOT_PACKET,
    COMPRESSION_ACK_PACKET,
    COMPRESSION_NUM_PACKETS
};

struct CompressionSnapshotPacket : public protocol::Packet
{
    uint16_t sequence;
    int compression_mode;

    CompressionSnapshotPacket() : Packet( COMPRESSION_SNAPSHOT_PACKET )
    {
        sequence = 0;
        compression_mode = COMPRESSION_MODE_UNCOMPRESSED;
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        auto snapshot_sliding_window = (SnapshotSlidingWindow*) stream.GetContext( CONTEXT_SNAPSHOT_SLIDING_WINDOW );
        auto snapshot_sequence_buffer = (SnapshotSequenceBuffer*) stream.GetContext( CONTEXT_SNAPSHOT_SEQUENCE_BUFFER );

        serialize_uint16( stream, sequence );

        serialize_int( stream, compression_mode, 0, COMPRESSION_NUM_MODES - 1 );

        CubeState * cubes = nullptr;

        if ( Stream::IsWriting )
        {
            CORE_ASSERT( snapshot_sliding_window );
            auto & entry = snapshot_sliding_window->Get( sequence );
            cubes = (CubeState*) &entry.cubes[0];
        }
        else
        {
            CORE_ASSERT( snapshot_sequence_buffer );
            auto entry = snapshot_sequence_buffer->Insert( sequence );
            CORE_ASSERT( entry );
            cubes = (CubeState*) &entry->cubes[0];
        }
        CORE_ASSERT( cubes );

        switch ( compression_mode )
        {
            case COMPRESSION_MODE_UNCOMPRESSED:
            {
                for ( int i = 0; i < NumCubes; ++i )
                {
                    serialize_bool( stream, cubes[i].interacting );
                    serialize_vector( stream, cubes[i].position );
                    serialize_quaternion( stream, cubes[i].orientation );
                    serialize_vector( stream, cubes[i].linear_velocity );
                }
            }
            break;

            case COMPRESSION_MODE_ORIENTATION:
            {
                for ( int i = 0; i < NumCubes; ++i )
                {
                    serialize_bool( stream, cubes[i].interacting );
                    serialize_vector( stream, cubes[i].position );
                    serialize_compressed_quaternion( stream, cubes[i].orientation, 9 );
                    serialize_vector( stream, cubes[i].linear_velocity );
                }
            }
            break;

            case COMPRESSION_MODE_LINEAR_VELOCITY:
            {
                for ( int i = 0; i < NumCubes; ++i )
                {
                    serialize_bool( stream, cubes[i].interacting );
                    serialize_vector( stream, cubes[i].position );
                    serialize_compressed_quaternion( stream, cubes[i].orientation, 9 );
                    serialize_compressed_vector( stream, cubes[i].linear_velocity, MaxLinearSpeed, 0.01f );
                }
            }
            break;

            case COMPRESSION_MODE_AT_REST_FLAG:
            {
                for ( int i = 0; i < NumCubes; ++i )
                {
                    serialize_bool( stream, cubes[i].interacting );
                    serialize_vector( stream, cubes[i].position );
                    serialize_compressed_quaternion( stream, cubes[i].orientation, 9 );

                    bool at_rest;
                    if ( Stream::IsWriting )
                        at_rest = length_squared( cubes[i].linear_velocity ) <= 0.000001f;
                    serialize_bool( stream, at_rest );
                    if ( !at_rest )
                        serialize_compressed_vector( stream, cubes[i].linear_velocity, MaxLinearSpeed, 0.01f );
                    else if ( Stream::IsReading )
                        cubes[i].linear_velocity = vectorial::vec3f::zero();
                }
            }
            break;

            case COMPRESSION_MODE_POSITION:
            {
                for ( int i = 0; i < NumCubes; ++i )
                {
                    QuantizedCubeState quantized_cube;

                    if ( Stream::IsWriting )
                        quantized_cube.Load( cubes[i] );

                    serialize_bool( stream, quantized_cube.interacting );
                    serialize_int( stream, quantized_cube.position_x, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
                    serialize_int( stream, quantized_cube.position_y, -QuantizedPositionBoundXY, +QuantizedPositionBoundXY - 1 );
                    serialize_int( stream, quantized_cube.position_z, 0, +QuantizedPositionBoundZ - 1 );
                    serialize_object( stream, quantized_cube.orientation );

                    if ( Stream::IsReading )
                        quantized_cube.Save( cubes[i] );
                }
            }
            break;

            default:
                break;
        }
    }
};

struct CompressionAckPacket : public protocol::Packet
{
    uint16_t ack;

    CompressionAckPacket() : Packet( COMPRESSION_ACK_PACKET )
    {
        ack = 0;
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_uint16( stream, ack );
    }
};

class CompressionPacketFactory : public protocol::PacketFactory
{
    core::Allocator * m_allocator;

public:

    CompressionPacketFactory( core::Allocator & allocator )
        : PacketFactory( allocator, COMPRESSION_NUM_PACKETS )
    {
        m_allocator = &allocator;
    }

protected:

    protocol::Packet * CreateInternal( int type )
    {
        switch ( type )
        {
            case COMPRESSION_SNAPSHOT_PACKET:   return CORE_NEW( *m_allocator, CompressionSnapshotPacket );
            case COMPRESSION_ACK_PACKET:        return CORE_NEW( *m_allocator, CompressionAckPacket );
            default:
                return nullptr;
        }
    }
};

struct CompressionInternal
{
    CompressionInternal( core::Allocator & allocator, const SnapshotModeData & mode_data ) 
        : packet_factory( allocator ), interpolation_buffer( allocator, mode_data )
    {
        this->allocator = &allocator;
        network::SimulatorConfig networkSimulatorConfig;
        snapshot_sliding_window = CORE_NEW( allocator, SnapshotSlidingWindow, allocator, MaxSnapshots );
        snapshot_sequence_buffer = CORE_NEW( allocator, SnapshotSequenceBuffer, allocator, MaxSnapshots );
        networkSimulatorConfig.packetFactory = &packet_factory;
        networkSimulatorConfig.maxPacketSize = MaxPacketSize;
        network_simulator = CORE_NEW( allocator, network::Simulator, networkSimulatorConfig );
        context[0] = snapshot_sliding_window;
        context[1] = snapshot_sequence_buffer;
        network_simulator->SetContext( context );
        Reset( mode_data );
    }

    ~CompressionInternal()
    {
        CORE_ASSERT( network_simulator );
        typedef network::Simulator NetworkSimulator;
        CORE_DELETE( *allocator, NetworkSimulator, network_simulator );
        CORE_DELETE( *allocator, SnapshotSlidingWindow, snapshot_sliding_window );
        CORE_DELETE( *allocator, SnapshotSequenceBuffer, snapshot_sequence_buffer );
        network_simulator = nullptr;
        snapshot_sliding_window = nullptr;
        snapshot_sequence_buffer = nullptr;
    }

    void Reset( const SnapshotModeData & mode_data )
    {
        interpolation_buffer.Reset();
        network_simulator->Reset();
        network_simulator->ClearStates();
        network_simulator->AddState( { mode_data.latency, mode_data.jitter, mode_data.packet_loss } );
        snapshot_sliding_window->Reset();
        snapshot_sequence_buffer->Reset();
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
    const void * context[2];
    network::Simulator * network_simulator;
    SnapshotSlidingWindow * snapshot_sliding_window;
    SnapshotSequenceBuffer * snapshot_sequence_buffer;
    CompressionPacketFactory packet_factory;
    SnapshotInterpolationBuffer interpolation_buffer;
};

CompressionDemo::CompressionDemo( core::Allocator & allocator )
{
    InitCompressionModes();
    m_allocator = &allocator;
    m_internal = nullptr;
    m_settings = CORE_NEW( *m_allocator, CubesSettings );
    m_compression = CORE_NEW( *m_allocator, CompressionInternal, *m_allocator, compression_mode_data[GetMode()] );
}

CompressionDemo::~CompressionDemo()
{
    Shutdown();
    CORE_DELETE( *m_allocator, CompressionInternal, m_compression );
    CORE_DELETE( *m_allocator, CubesSettings, m_settings );
    m_compression = nullptr;
    m_settings = nullptr;
    m_allocator = nullptr;
}

bool CompressionDemo::Initialize()
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

void CompressionDemo::Shutdown()
{
    CORE_ASSERT( m_allocator );

    CORE_ASSERT( m_compression );
    m_compression->Reset( compression_mode_data[GetMode()] );

    if ( m_internal )
    {
        m_internal->Free( *m_allocator );
        CORE_DELETE( *m_allocator, CubesInternal, m_internal );
        m_internal = nullptr;
    }
}

void CompressionDemo::Update()
{
    CubesUpdateConfig update_config;

    auto local_input = m_internal->GetLocalInput();

    // setup left simulation to update one frame with local input

    update_config.sim[0].num_frames = 1;
    update_config.sim[0].frame_input[0] = local_input;

    // send a snapshot packet to the right simulation

    m_compression->send_accumulator += global.timeBase.deltaTime;

    if ( m_compression->send_accumulator >= 1.0f / compression_mode_data[GetMode()].send_rate )
    {
        m_compression->send_accumulator = 0.0f;   

        auto game_instance = m_internal->GetGameInstance( 0 );

        auto snapshot_packet = (CompressionSnapshotPacket*) m_compression->packet_factory.Create( COMPRESSION_SNAPSHOT_PACKET );

        snapshot_packet->sequence = m_compression->send_sequence++;
        snapshot_packet->compression_mode = GetMode();

        uint16_t sequence;

        auto & snapshot = m_compression->snapshot_sliding_window->Insert( sequence );

        if ( GetSnapshot( game_instance, snapshot ) )
            m_compression->network_simulator->SendPacket( network::Address( "::1", RightPort ), snapshot_packet );
        else
            m_compression->packet_factory.Destroy( snapshot_packet );
    }

    // update the network simulator

    m_compression->network_simulator->Update( global.timeBase );

    // receive packets from the simulator (with latency, packet loss and jitter applied...)

    bool received_snapshot_this_frame = false;
    uint16_t ack_sequence = 0;

    while ( true )
    {
        auto packet = m_compression->network_simulator->ReceivePacket();
        if ( !packet )
            break;

        const auto port = packet->GetAddress().GetPort();
        const auto type = packet->GetType();

        if ( type == COMPRESSION_SNAPSHOT_PACKET && port == RightPort )
        {
            auto snapshot_packet = (CompressionSnapshotPacket*) packet;

            Snapshot * snapshot = m_compression->snapshot_sequence_buffer->Find( snapshot_packet->sequence );
            CORE_ASSERT( snapshot );
            m_compression->interpolation_buffer.AddSnapshot( global.timeBase.time, snapshot_packet->sequence, snapshot->cubes );

            if ( !received_snapshot_this_frame || ( received_snapshot_this_frame && core::sequence_greater_than( snapshot_packet->sequence, ack_sequence ) ) )
            {
                received_snapshot_this_frame = true;
                ack_sequence = snapshot_packet->sequence;
            }
        }
        else if ( type == COMPRESSION_ACK_PACKET && port == LeftPort )
        {
            auto ack_packet = (CompressionAckPacket*) packet;

            m_compression->snapshot_sliding_window->Ack( ack_packet->ack - 1 );
            m_compression->received_ack = true;
        }

        m_compression->packet_factory.Destroy( packet );
    }

    // if any snapshots packets were received this frame, send an ack packet back to the left simulation

    if ( received_snapshot_this_frame )
    {
        auto ack_packet = (CompressionAckPacket*) m_compression->packet_factory.Create( COMPRESSION_ACK_PACKET );
        ack_packet->ack = ack_sequence;
        m_compression->network_simulator->SetBandwidthExclude( true );
        m_compression->network_simulator->SendPacket( network::Address( "::1", LeftPort ), ack_packet );
        m_compression->network_simulator->SetBandwidthExclude( false );
    }

    // if we are an an interpolation mode, we need to grab the view updates for the right side from the interpolation buffer

    int num_object_updates = 0;

    view::ObjectUpdate object_updates[NumCubes];

    m_compression->interpolation_buffer.GetViewUpdate( compression_mode_data[GetMode()], global.timeBase.time, object_updates, num_object_updates );

    if ( num_object_updates > 0 )
        m_internal->view[1].objects.UpdateObjects( object_updates, num_object_updates );
    else if ( m_compression->interpolation_buffer.interpolating )
        printf( "no snapshot to interpolate towards!\n" );

    // run the simulation

    m_internal->Update( update_config );
}

bool CompressionDemo::Clear()
{
    return m_internal->Clear();
}

void CompressionDemo::Render()
{
    CubesRenderConfig render_config;

    render_config.render_mode = CUBES_RENDER_SPLITSCREEN;

    m_internal->Render( render_config );

    const float bandwidth = m_compression->network_simulator->GetBandwidth();

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

bool CompressionDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    return m_internal->KeyEvent( key, scancode, action, mods );
}

bool CompressionDemo::CharEvent( unsigned int code )
{
    // ...

    return false;
}

int CompressionDemo::GetNumModes() const
{
    return COMPRESSION_NUM_MODES;
}

const char * CompressionDemo::GetModeDescription( int mode ) const
{
    return compression_mode_descriptions[mode];
}

#endif // #ifdef CLIENT
