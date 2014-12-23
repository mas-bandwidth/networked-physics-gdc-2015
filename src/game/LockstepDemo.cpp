#include "LockstepDemo.h"
#include "Cubes.h"

#ifdef CLIENT

#include "Global.h"
#include "Render.h"
#include "Console.h"
#include "protocol/Stream.h"
#include "protocol/SlidingWindow.h"
#include "protocol/PacketFactory.h"

static const int MaxInputs = 256;

struct LockstepInput
{
    uint32_t valid : 1;
    uint32_t sequence : 16;
    game::Input input;
};

typedef protocol::SlidingWindow<LockstepInput> LockstepInputSlidingWindow;

enum LockstepPackets
{
    LOCKSTEP_PACKET_INPUTS,
    LOCKSTEP_PACKET_ACK,
    LOCKSTEP_NUM_PACKETS
};

struct LockstepPacketInputs : public protocol::Packet
{
    uint16_t sequence;
    uint16_t num_inputs;
    game::Input inputs[MaxInputs];

    LockstepPacketInputs() : Packet( LOCKSTEP_PACKET_INPUTS )
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

struct LockstepPacketAck : public protocol::Packet
{
    uint16_t ack;

    LockstepPacketAck() : Packet( LOCKSTEP_PACKET_ACK )
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
            case LOCKSTEP_PACKET_INPUTS:    return CORE_NEW( *m_allocator, LockstepPacketInputs ); 
            case LOCKSTEP_PACKET_ACK:       return CORE_NEW( *m_allocator, LockstepPacketAck );          
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
        // ...
    }

    LockstepPacketFactory packet_factory;
    LockstepInputSlidingWindow input_sliding_window;
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

    update_config.run_update[0] = true;
    update_config.input[0] = m_internal->GetLocalInput();

    // todo: dequeue input and frames from playout delay buffer vs. feeding in local input

    update_config.run_update[1] = true;
    update_config.input[1] = m_internal->GetLocalInput();

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

    // todo: if not deterministic, show text "non-deterministic!" flashing on screen?
}

bool LockstepDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    if ( action == GLFW_PRESS && mods == 0 )
    {
        if ( key == GLFW_KEY_BACKSPACE )
        {
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
