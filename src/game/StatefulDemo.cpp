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

//static const int LeftPort = 1000;
//static const int RightPort = 1001;

enum Context
{
    CONTEXT_QUANTIZED_SNAPSHOT_SLIDING_WINDOW,      // quantized send snapshots (for serialize write)
    CONTEXT_QUANTIZED_SNAPSHOT_SEQUENCE_BUFFER,     // quantized recv snapshots (for serialize read)
    CONTEXT_QUANTIZED_INITIAL_SNAPSHOT              // quantized initial snapshot
};

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
    int bandwidth = 64 * 1000;                      // 64kbps bandwidth by default
    float playout_delay = 0.035f;                   // handle +/- two frames jitter @ 60 fps
    float send_rate = 60.0f;
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

struct StatePacket : public protocol::Packet
{
    uint16_t sequence;
    int num_cubes;

    // todo: need full quantized cube state with linear and angular velocity

    StatePacket() : Packet( STATE_PACKET )
    {
        sequence = 0;
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_uint16( stream, sequence );

        // ...
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
        recv_sequence = 0;
    }

    core::Allocator * allocator;
    uint16_t send_sequence;
    uint16_t recv_sequence;
    game::Input remote_input;
    network::Simulator * network_simulator;
    StatePacketFactory packet_factory;
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

    // todo: state packet

    /*
    // send a snapshot packet to the right simulation

    m_stateful->send_accumulator += global.timeBase.deltaTime;

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
            m_delta->network_simulator->SendPacket( network::Address( "::1", RightPort ), snapshot_packet );
        else
            m_delta->packet_factory.Destroy( snapshot_packet );
    }
    */

    // update the network simulator

    m_stateful->network_simulator->Update( global.timeBase );

    // receive packets from the simulator (with latency, packet loss and jitter applied...)

    while ( true )
    {
        auto packet = m_stateful->network_simulator->ReceivePacket();
        if ( !packet )
            break;

        // todo: process packet

        /*
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
        */

        m_stateful->packet_factory.Destroy( packet );
    }

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
