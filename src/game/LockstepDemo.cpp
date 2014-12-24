#include "LockstepDemo.h"
#include "Cubes.h"

#ifdef CLIENT

#include "Global.h"
#include "Render.h"
#include "Console.h"
#include "protocol/Stream.h"
#include "protocol/SlidingWindow.h"
#include "protocol/PacketFactory.h"
#include "network/Simulator.h"

static const int MaxInputs = 256;
static const int LeftPort = 1000;
static const int RightPort = 1001;
static const int MaxPacketSize = 1024;

typedef protocol::RealSlidingWindow<game::Input> LockstepInputSlidingWindow;

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

            for ( int i = 0; i < num_inputs; ++i )
            {
                bool input_changed = Stream::IsWriting ? inputs[i] == inputs[i-1] : false;
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

struct LockstepInternal
{
    LockstepInternal( core::Allocator & allocator ) 
        : packet_factory( allocator ), input_sliding_window( allocator, MaxInputs )
    {
        this->allocator = &allocator;
        network::SimulatorConfig networkSimulatorConfig;
        networkSimulatorConfig.packetFactory = &packet_factory;
        network_simulator = CORE_NEW( allocator, network::Simulator, networkSimulatorConfig );
        const float latency = 0.1f;
        const float packet_loss = 5.0f;
        const float jitter = 2 * 1.0f / 60.0f;
        network_simulator->AddState( { latency, packet_loss, jitter } );
    }

    ~LockstepInternal()
    {
        CORE_ASSERT( network_simulator );
        typedef network::Simulator NetworkSimulator;
        CORE_DELETE( *allocator, NetworkSimulator, network_simulator );
        network_simulator = nullptr;
    }

    core::Allocator * allocator;
    LockstepPacketFactory packet_factory;
    LockstepInputSlidingWindow input_sliding_window;
    network::Simulator * network_simulator;
};

LockstepDemo::LockstepDemo( core::Allocator & allocator )
{
    m_allocator = &allocator;
    m_internal = nullptr;
    m_settings = CORE_NEW( *m_allocator, CubesSettings );
    m_lockstep = CORE_NEW( *m_allocator, LockstepInternal, *m_allocator );
}

LockstepDemo::~LockstepDemo()
{
    Shutdown();
    CORE_DELETE( *m_allocator, LockstepInternal, m_lockstep );
    CORE_DELETE( *m_allocator, CubesSettings, m_settings );
    m_settings = nullptr;
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

    return true;
}

void LockstepDemo::Shutdown()
{
    CORE_ASSERT( m_allocator );
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

    update_config.run_update[0] = true;
    update_config.input[0] = local_input;

    // insert local input for this frame in the input sliding window

    auto & inputs = m_lockstep->input_sliding_window;

    CORE_ASSERT( !inputs.IsFull() );

    inputs.Insert( local_input );

    // send an input packet to the left simulation (all inputs since last ack)

    auto input_packet = (LockstepInputPacket*) m_lockstep->packet_factory.Create( LOCKSTEP_PACKET_INPUT );

    input_packet->sequence = inputs.GetSequence();

    inputs.GetArray( input_packet->inputs, input_packet->num_inputs );

    m_lockstep->network_simulator->SendPacket( network::Address( "::1", RightPort ), input_packet );

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

            printf( "input packet for right simulation\n" );

            auto input_packet = (LockstepInputPacket*) read_packet;

            if ( !received_input_this_frame )
            {
                received_input_this_frame = true;
                ack_sequence = input_packet->sequence;
            }
            else
            {
                if ( core::sequence_greater_than( input_packet->sequence, ack_sequence ) )
                    ack_sequence = input_packet->sequence;
            }

            // todo: insert inputs into playout delay buffer
        }
        else if ( type == LOCKSTEP_PACKET_ACK && port == LeftPort )
        {
            // ack packet for left simulation

            printf( "ack packet for left simulation\n" );

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

    // todo: if we have a frame available from playout delay buffer set the right simulation to run a frame with that input

    update_config.run_update[1] = true;
    update_config.input[1] = m_internal->GetLocalInput();

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
    if ( action == GLFW_PRESS && mods == 0 )
    {
        if ( key == GLFW_KEY_BACKSPACE )
        {
            Shutdown();
            Initialize();
            return true;
        }
        else if ( key == GLFW_KEY_F1 )
        {
            m_settings->deterministic = !m_settings->deterministic;
        }
    }

    return m_internal->KeyEvent( key, scancode, action, mods );
}

bool LockstepDemo::CharEvent( unsigned int code )
{
    return false;
}

#endif // #ifdef CLIENT
