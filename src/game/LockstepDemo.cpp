#include "LockstepDemo.h"
#include "Cubes.h"

#ifdef CLIENT

#include "Global.h"
#include "Render.h"
#include "Console.h"
#include "core/Queue.h"
#include "protocol/Stream.h"
#include "protocol/SlidingWindow.h"
#include "protocol/PacketFactory.h"
#include "network/Simulator.h"

static const int MaxInputs = 256;
static const int MaxPacketSize = 1024;
static const int PlayoutDelayBufferSize = 1024;

static const int LeftPort = 1000;
static const int RightPort = 1001;

enum LockstepModes
{
    LOCKSTEP_MODE_NON_DETERMINISTIC,
    LOCKSTEP_MODE_DETERMINISTIC,
    LOCKSTEP_MODE_TCP_100MS_1PC,
    LOCKSTEP_MODE_TCP_200MS_2PC,
    LOCKSTEP_MODE_TCP_250MS_5PC,
    LOCKSTEP_MODE_UDP_2000MS_25PC,
    LOCKSTEP_MODE_UDP_2000MS_50PC,
    LOCKSTEP_NUM_MODES
};

static const char * lockstep_mode_descriptions[] =
{
    "Deterministic Mode",
    "Non-Determistic Mode",
    "TCP at 100ms latency and 1%% packet loss (100ms playout delay buffer)",
    "TCP at 200ms latency and 2%% packet loss (100ms playout delay buffer)",
    "TCP at 250ms latency and 5%% packet loss (100ms playout delay buffer)",
    "UDP at 2 seconds latency and 25%% packet loss (100ms playout delay buffer)",
    "UDP at 2 seconds latency and 50%% packet loss (250ms playout delay buffer)",
};

struct LockstepModeData
{
    bool tcp = true;
    bool deterministic = true;
    float playout_delay = 0.1f;
    float latency = 0.0f;
    float packet_loss = 0.0f;
    float jitter = 0.0f;
};

static LockstepModeData lockstep_mode_data[LOCKSTEP_NUM_MODES];

static void InitLockstepModes()
{
    lockstep_mode_data[LOCKSTEP_MODE_NON_DETERMINISTIC].tcp = true;
    lockstep_mode_data[LOCKSTEP_MODE_NON_DETERMINISTIC].deterministic = false;
    lockstep_mode_data[LOCKSTEP_MODE_NON_DETERMINISTIC].latency = 1.0f;     // note: this is one way latency. eg. RTT/2

    lockstep_mode_data[LOCKSTEP_MODE_DETERMINISTIC].tcp = true;
    lockstep_mode_data[LOCKSTEP_MODE_DETERMINISTIC].playout_delay = 0.1f;
    lockstep_mode_data[LOCKSTEP_MODE_DETERMINISTIC].latency = 1.0f;

    lockstep_mode_data[LOCKSTEP_MODE_TCP_100MS_1PC].playout_delay = 0.1f;
    lockstep_mode_data[LOCKSTEP_MODE_TCP_100MS_1PC].latency = 0.05f;
    lockstep_mode_data[LOCKSTEP_MODE_TCP_100MS_1PC].packet_loss = 1.0f;
    lockstep_mode_data[LOCKSTEP_MODE_TCP_100MS_1PC].jitter = 1.0f / 60.0f;

    lockstep_mode_data[LOCKSTEP_MODE_TCP_200MS_2PC].playout_delay = 0.1f;
    lockstep_mode_data[LOCKSTEP_MODE_TCP_200MS_2PC].latency = 0.1f;
    lockstep_mode_data[LOCKSTEP_MODE_TCP_200MS_2PC].packet_loss = 2.0f;
    lockstep_mode_data[LOCKSTEP_MODE_TCP_200MS_2PC].jitter = 1.0f / 60.0f;

    lockstep_mode_data[LOCKSTEP_MODE_TCP_250MS_5PC].playout_delay = 0.1f;
    lockstep_mode_data[LOCKSTEP_MODE_TCP_250MS_5PC].latency = 0.125f;
    lockstep_mode_data[LOCKSTEP_MODE_TCP_250MS_5PC].packet_loss = 5.0f;
    lockstep_mode_data[LOCKSTEP_MODE_TCP_250MS_5PC].jitter = 1.0f / 60.0f;

    lockstep_mode_data[LOCKSTEP_MODE_UDP_2000MS_25PC].tcp = false;
    lockstep_mode_data[LOCKSTEP_MODE_UDP_2000MS_25PC].playout_delay = 0.1f;
    lockstep_mode_data[LOCKSTEP_MODE_UDP_2000MS_25PC].latency = 1.0f;
    lockstep_mode_data[LOCKSTEP_MODE_UDP_2000MS_25PC].packet_loss = 25.0f;
    lockstep_mode_data[LOCKSTEP_MODE_UDP_2000MS_25PC].jitter = 1.0f / 60.0f;

    lockstep_mode_data[LOCKSTEP_MODE_UDP_2000MS_50PC].tcp = false;
    lockstep_mode_data[LOCKSTEP_MODE_UDP_2000MS_50PC].playout_delay = 0.25f;
    lockstep_mode_data[LOCKSTEP_MODE_UDP_2000MS_50PC].latency = 1.0f;
    lockstep_mode_data[LOCKSTEP_MODE_UDP_2000MS_50PC].packet_loss = 50.0f;
    lockstep_mode_data[LOCKSTEP_MODE_UDP_2000MS_50PC].jitter = 1.0f / 60.0f;
}

typedef protocol::SlidingWindow<game::Input> LockstepInputSlidingWindow;

enum LockstepPackets
{
    LOCKSTEP_PACKET_INPUT,
    LOCKSTEP_PACKET_ACK,
    LOCKSTEP_NUM_PACKETS
};

struct LockstepInputPacket : public protocol::Packet
{
    uint16_t sequence;
    int num_inputs;
    game::Input inputs[MaxInputs];

    LockstepInputPacket() : Packet( LOCKSTEP_PACKET_INPUT )
    {
        sequence = 0;
        num_inputs = 0;
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_uint16( stream, sequence );
        
        serialize_int( stream, num_inputs, 0, MaxInputs );

        if ( num_inputs >= 1 )
        {   
            serialize_bool( stream, inputs[0].left );
            serialize_bool( stream, inputs[0].right );
            serialize_bool( stream, inputs[0].up );
            serialize_bool( stream, inputs[0].down );
            serialize_bool( stream, inputs[0].push );
            serialize_bool( stream, inputs[0].pull );

            for ( int i = 1; i < num_inputs; ++i )
            {
                bool input_changed = Stream::IsWriting ? inputs[i] != inputs[i-1] : false;
                serialize_bool( stream, input_changed );
                if ( input_changed )
                {
                    serialize_bool( stream, inputs[i].left );
                    serialize_bool( stream, inputs[i].right );
                    serialize_bool( stream, inputs[i].up );
                    serialize_bool( stream, inputs[i].down );
                    serialize_bool( stream, inputs[i].push );
                    serialize_bool( stream, inputs[i].pull );
                }
                else if ( Stream::IsReading )
                {
                    inputs[i] = inputs[i-1];
                }
            }
        }
    }
};

struct LockstepAckPacket : public protocol::Packet
{
    uint16_t ack;

    LockstepAckPacket() : Packet( LOCKSTEP_PACKET_ACK )
    {
        ack = 0;
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_uint16( stream, ack );
    }
};

class LockstepPacketFactory : public protocol::PacketFactory
{
    core::Allocator * m_allocator;

public:

    LockstepPacketFactory( core::Allocator & allocator )
        : PacketFactory( allocator, LOCKSTEP_NUM_PACKETS )
    {
        m_allocator = &allocator;
    }

protected:

    protocol::Packet * CreateInternal( int type )
    {
        switch ( type )
        {
            case LOCKSTEP_PACKET_INPUT:     return CORE_NEW( *m_allocator, LockstepInputPacket );
            case LOCKSTEP_PACKET_ACK:       return CORE_NEW( *m_allocator, LockstepAckPacket );
            default:
                return nullptr;
        }
    }
};

struct LockstepPlayoutDelayBuffer
{
    LockstepPlayoutDelayBuffer( core::Allocator & allocator, const LockstepModeData & mode_data )
        : input_queue( allocator )
    {
        stopped = true;
        start_time = 0.0;
        most_recent_input = 0;
        frame = 0;
        playout_delay = mode_data.playout_delay;
        core::queue::reserve( input_queue, PlayoutDelayBufferSize );
    }

    void AddInputs( double time, uint16_t sequence, int num_inputs, game::Input * inputs )
    {
        CORE_ASSERT( num_inputs > 0 );

        if ( stopped )
        {
            start_time = time;
            stopped = false;
        }

        const uint16_t first_input_sequence = sequence - num_inputs;

        for ( int i = 0; i < num_inputs; ++i )
        {
            const uint16_t sequence = first_input_sequence + i;

            if ( sequence == most_recent_input )
            {
                most_recent_input = sequence + 1;
                core::queue::push_back( input_queue, inputs[i] );
            }
        }
    }

    void GetFrames( double time, int & num_frames, game::Input * frame_input )
    {
        num_frames = 0;

        if ( stopped )
            return;

        for ( int i = 0; i < MaxSimFrames; ++i )
        {
            if ( time < ( start_time + ( frame + 0.5 ) * ( 1.0f / 60.0f ) + playout_delay ) )
                break;

            if ( !core::queue::size( input_queue ) )
                break;

            frame_input[i] = input_queue[0];

            core::queue::pop_front( input_queue );

            num_frames++;

            frame++;
        }
    }

    void Reset()
    {
        stopped = true;
        start_time = 0.0;
        most_recent_input = 0;
        frame = 0;
        core::queue::clear( input_queue );
    }

    bool stopped;
    double start_time;
    uint16_t most_recent_input;
    uint64_t frame;
    float playout_delay;
    core::Queue<game::Input> input_queue;
};

struct LockstepInternal
{
    LockstepInternal( core::Allocator & allocator, const LockstepModeData & mode_data ) 
        : packet_factory( allocator ), input_sliding_window( allocator, MaxInputs ), playout_delay_buffer( allocator, mode_data )
    {
        this->allocator = &allocator;
        network::SimulatorConfig networkSimulatorConfig;
        networkSimulatorConfig.packetFactory = &packet_factory;
        network_simulator = CORE_NEW( allocator, network::Simulator, networkSimulatorConfig );
        Reset( mode_data );
    }

    ~LockstepInternal()
    {
        CORE_ASSERT( network_simulator );
        typedef network::Simulator NetworkSimulator;
        CORE_DELETE( *allocator, NetworkSimulator, network_simulator );
        network_simulator = nullptr;
    }

    void Reset( const LockstepModeData & mode_data )
    {
        input_sliding_window.Reset();
        playout_delay_buffer.Reset();
        network_simulator->Reset();
        network_simulator->ClearStates();
        network_simulator->AddState( { mode_data.latency, mode_data.jitter, mode_data.packet_loss } );
        network_simulator->SetTCPMode( mode_data.tcp );
    }

    core::Allocator * allocator;
    LockstepPacketFactory packet_factory;
    LockstepInputSlidingWindow input_sliding_window;
    LockstepPlayoutDelayBuffer playout_delay_buffer;
    network::Simulator * network_simulator;
};

LockstepDemo::LockstepDemo( core::Allocator & allocator )
{
    InitLockstepModes();
    SetMode( LOCKSTEP_MODE_DETERMINISTIC );
    m_allocator = &allocator;
    m_internal = nullptr;
    m_settings = CORE_NEW( *m_allocator, CubesSettings );
    m_lockstep = CORE_NEW( *m_allocator, LockstepInternal, *m_allocator, lockstep_mode_data[GetMode()] );
}

LockstepDemo::~LockstepDemo()
{
    Shutdown();
    CORE_DELETE( *m_allocator, LockstepInternal, m_lockstep );
    CORE_DELETE( *m_allocator, CubesSettings, m_settings );
    m_settings = nullptr;
    m_lockstep = nullptr;
    m_allocator = nullptr;
}

bool LockstepDemo::Initialize()
{
    if ( m_internal )
        Shutdown();

    m_internal = CORE_NEW( *m_allocator, CubesInternal );    

    CubesConfig config;
    
    config.num_simulations = 2;
    config.num_views = 2;

    m_internal->Initialize( *m_allocator, config, m_settings );

    m_settings->deterministic = lockstep_mode_data[GetMode()].deterministic;

    return true;
}

void LockstepDemo::Shutdown()
{
    CORE_ASSERT( m_allocator );

    CORE_ASSERT( m_lockstep );
    m_lockstep->Reset( lockstep_mode_data[GetMode()] );

    if ( m_internal )
    {
        m_internal->Free( *m_allocator );
        CORE_DELETE( *m_allocator, CubesInternal, m_internal );
        m_internal = nullptr;
    }
}

void LockstepDemo::Update()
{
    CubesUpdateConfig update_config;

    auto local_input = m_internal->GetLocalInput();

    // setup left simulation to update one frame with local input

    update_config.sim[0].num_frames = 1;
    update_config.sim[0].frame_input[0] = local_input;

    // insert local input for this frame in the input sliding window

    auto & inputs = m_lockstep->input_sliding_window;

    CORE_ASSERT( !inputs.IsFull() );

    inputs.Insert( local_input );

    // send an input packet to the right simulation (all inputs since last ack)

    auto input_packet = (LockstepInputPacket*) m_lockstep->packet_factory.Create( LOCKSTEP_PACKET_INPUT );

    input_packet->sequence = inputs.GetSequence();

    inputs.GetArray( input_packet->inputs, input_packet->num_inputs );

    m_lockstep->network_simulator->SendPacket( network::Address( "::1", RightPort ), input_packet );

    // if we are in TCP mode, ack right away because the simulator ensures reliable-ordered delivery

    if ( m_lockstep->network_simulator->GetTCPMode() )
        inputs.Ack( inputs.GetSequence() );

    // update the network simulator

    m_lockstep->network_simulator->Update( global.timeBase );

    // receive packets from the simulator (with latency, packet loss and jitter applied...)

    bool received_input_this_frame = false;
    uint16_t ack_sequence = 0;

    while ( true )
    {
        auto packet = m_lockstep->network_simulator->ReceivePacket();
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

        m_lockstep->packet_factory.Destroy( packet );
        packet = nullptr;

        protocol::ReadStream read_stream( buffer, MaxPacketSize );
        auto read_packet = m_lockstep->packet_factory.Create( type );
        read_packet->SerializeRead( read_stream );
        CORE_CHECK( !read_stream.IsOverflow() );

        if ( type == LOCKSTEP_PACKET_INPUT && port == RightPort )
        {
            // input packet for right simulation

            auto input_packet = (LockstepInputPacket*) read_packet;

            if ( !m_lockstep->network_simulator->GetTCPMode() )
            {
                if ( !received_input_this_frame )
                {
                    received_input_this_frame = true;
                    ack_sequence = input_packet->sequence - 1;
                }
                else
                {
                    if ( core::sequence_greater_than( input_packet->sequence - 1, ack_sequence ) )
                        ack_sequence = input_packet->sequence - 1;
                }
            }

            m_lockstep->playout_delay_buffer.AddInputs( global.timeBase.time, input_packet->sequence, input_packet->num_inputs, input_packet->inputs );
        }
        else if ( type == LOCKSTEP_PACKET_ACK && port == LeftPort && !m_lockstep->network_simulator->GetTCPMode() )
        {
            // ack packet for left simulation

            auto ack_packet = (LockstepAckPacket*) read_packet;

            inputs.Ack( ack_packet->ack );
        }

        m_lockstep->packet_factory.Destroy( read_packet );
        read_packet = nullptr;
    }

    // if any input packets were received this frame, send an ack packet back to the left simulation

    if ( received_input_this_frame )
    {
        auto ack_packet = (LockstepAckPacket*) m_lockstep->packet_factory.Create( LOCKSTEP_PACKET_ACK );

        ack_packet->ack = ack_sequence;
    
        m_lockstep->network_simulator->SendPacket( network::Address( "::1", LeftPort ), ack_packet );
    }

    // if we have frames available from playout delay buffer set them up to simulate

    m_lockstep->playout_delay_buffer.GetFrames( global.timeBase.time, update_config.sim[1].num_frames, update_config.sim[1].frame_input );

    // run the simulation(s)

    m_internal->Update( update_config );
}

bool LockstepDemo::Clear()
{
    return m_internal->Clear();
}

void LockstepDemo::Render()
{
    CubesRenderConfig render_config;

    render_config.render_mode = CUBES_RENDER_SPLITSCREEN;

    m_internal->Render( render_config );
}

bool LockstepDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    return m_internal->KeyEvent( key, scancode, action, mods );
}

bool LockstepDemo::CharEvent( unsigned int code )
{
    return false;
}

int LockstepDemo::GetNumModes() const
{
    return LOCKSTEP_NUM_MODES;
}

const char * LockstepDemo::GetModeDescription( int mode ) const
{
    return lockstep_mode_descriptions[mode];
}

#endif // #ifdef CLIENT
