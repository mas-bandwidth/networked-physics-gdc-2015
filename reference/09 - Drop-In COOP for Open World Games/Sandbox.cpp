#include "Mathematics.h"
#include "Common.h"
#include "Simulation.h"
#include "RubyNative.h"
#include "Net.h"
#include "Platform.h"

// conversion between ruby and c++

VALUE NewRubyVector( const math::Vector & vector )
{
	VALUE args[] = { RubyFloat::create(vector.x), RubyFloat::create(vector.y), RubyFloat::create(vector.z) };
	VALUE rubyVector = RUBY_NEW_ARGS( "Vector", 3, args );
	return rubyVector;
}

VALUE NewRubyQuaternion( const math::Quaternion & quaternion )
{
	VALUE args[] = { RubyFloat::create(quaternion.w), RubyFloat::create(quaternion.x), RubyFloat::create(quaternion.y), RubyFloat::create(quaternion.z) };
	VALUE rubyQuaternion = RUBY_NEW_ARGS( "Quaternion", 4, args );
	return rubyQuaternion;
}

void RubyVector( VALUE rubyVector, math::Vector & vector )
{
	vector.x = RUBY_FLOAT( rb_iv_get( rubyVector, "@x") );
	vector.y = RUBY_FLOAT( rb_iv_get( rubyVector, "@y") );
	vector.z = RUBY_FLOAT( rb_iv_get( rubyVector, "@z") );
}

void RubyQuaternion( VALUE rubyQuaternion, math::Quaternion & quaternion )
{
	quaternion.w = RUBY_FLOAT( rb_iv_get( rubyQuaternion, "@w") );
	quaternion.x = RUBY_FLOAT( rb_iv_get( rubyQuaternion, "@x") );
	quaternion.y = RUBY_FLOAT( rb_iv_get( rubyQuaternion, "@y") );
	quaternion.z = RUBY_FLOAT( rb_iv_get( rubyQuaternion, "@z") );
}

void RubyAddress( VALUE rubyAddress, Address & address )
{
	int a = RUBY_INT( rb_iv_get( rubyAddress, "@a" ) );
	int b = RUBY_INT( rb_iv_get( rubyAddress, "@b" ) );
	int c = RUBY_INT( rb_iv_get( rubyAddress, "@c" ) );
	int d = RUBY_INT( rb_iv_get( rubyAddress, "@d" ) );
	int port = RUBY_INT( rb_iv_get( rubyAddress, "@port" ) );
	address = Address( a, b, c, d, port );
}

// ruby simulation

class RubySimulation : public RubyObject<NewSimulation>
{
public:

    static void registerClass()
    {
        DefineClass( "Simulation" );
        AddStaticMethod( "new", rubyNew, 1 );
		AddStaticMethod( "shutdown", shutdown, 0 );
        AddMethod( "update", update, 1 );
		AddMethod( "addCube", addCube, 1 );
		AddMethod( "removeCube", removeCube, 1 );
		AddMethod( "setCubeState", setCubeState, 2 );
		AddMethod( "getCubeState", getCubeState, 1 );
		AddMethod( "addPlane", addPlane, 2 );
		AddMethod( "reset", reset, 0 );
    }

protected:

    RubyMethod rubyNew( VALUE rubyClass, VALUE configHash )
    {
		SimulationConfiguration simConfig;
		simConfig.ERP = RUBY_FLOAT( RUBY_HASH_LOOKUP( configHash, "ERP" ) );
		simConfig.CFM = RUBY_FLOAT( RUBY_HASH_LOOKUP( configHash, "CFM" ) );
		simConfig.MaxIterations = RUBY_INT( RUBY_HASH_LOOKUP( configHash, "MaxIterations" ) );
		simConfig.MaximumCorrectingVelocity = RUBY_FLOAT( RUBY_HASH_LOOKUP( configHash, "MaximumCorrectingVelocity" ) );
		simConfig.ContactSurfaceLayer = RUBY_FLOAT( RUBY_HASH_LOOKUP( configHash, "ContactSurfaceLayer" ) );
		simConfig.Elasticity = RUBY_FLOAT( RUBY_HASH_LOOKUP( configHash, "Elasticity" ) );
		simConfig.Friction = RUBY_FLOAT( RUBY_HASH_LOOKUP( configHash, "Friction" ) );
		simConfig.Gravity = RUBY_FLOAT( RUBY_HASH_LOOKUP( configHash, "Gravity" ) );
		simConfig.QuickStep = RUBY_BOOLEAN( RUBY_HASH_LOOKUP( configHash, "QuickStep" ) );
		simConfig.MaxCubes = RUBY_INT( RUBY_HASH_LOOKUP( configHash, "MaxCubes" ) );
        RUBY_BEGIN( "Simulation.new" )
        VALUE object;
 		NewSimulation * instance = construct( object );
		*instance = NewSimulation();
		instance->Initialize( simConfig );
        return object;
		RUBY_END()
    }

    RubyMethod shutdown( VALUE rubyClass )
    {
        RUBY_BEGIN( "Simulation.shutdown" )
		NewSimulation::Shutdown();
		return Qnil;
		RUBY_END()
    }

    RubyMethod update( VALUE self, VALUE deltaTime )
    {
        RUBY_BEGIN( "Simulation.update" )
	    NewSimulation * simulation = internal( self );
		simulation->Update( RUBY_FLOAT(deltaTime) );
		return Qnil;
        RUBY_END()
    }

    RubyMethod addCube( VALUE self, VALUE rubyState )
    {
        RUBY_BEGIN( "Simulation.addCube" )
	    NewSimulation * simulation = internal( self );
		SimulationInitialCubeState initialState;
		RubyVector( rb_iv_get( rubyState, "@position" ), initialState.position );
		RubyQuaternion( rb_iv_get( rubyState, "@orientation" ), initialState.orientation );
		RubyVector( rb_iv_get( rubyState, "@linearVelocity" ), initialState.linearVelocity );
		RubyVector( rb_iv_get( rubyState, "@angularVelocity" ), initialState.angularVelocity );
		initialState.enabled = RUBY_BOOLEAN( rb_iv_get( rubyState, "@enabled" ) );
		initialState.scale = RUBY_FLOAT( rb_iv_get( rubyState, "@scale" ) );
		initialState.density = RUBY_FLOAT( rb_iv_get( rubyState, "@density" ) );
		int id = simulation->AddCube( initialState );
		return RubyInt::create( id );
        RUBY_END()
    }

    RubyMethod removeCube( VALUE self, VALUE id )
    {
        RUBY_BEGIN( "Simulation.removeCube" )
	    NewSimulation * simulation = internal( self );
		simulation->RemoveCube( RUBY_INT( id ) );
		return Qnil;
        RUBY_END()
    }

    RubyMethod setCubeState( VALUE self, VALUE id, VALUE rubyState )
    {
        RUBY_BEGIN( "Simulation.removeCube" )
	    NewSimulation * simulation = internal( self );
		SimulationCubeState cubeState;
		RubyVector( rb_iv_get( rubyState, "@position" ), cubeState.position );
		RubyQuaternion( rb_iv_get( rubyState, "@orientation" ), cubeState.orientation );
		RubyVector( rb_iv_get( rubyState, "@linearVelocity" ), cubeState.linearVelocity );
		RubyVector( rb_iv_get( rubyState, "@angularVelocity" ), cubeState.angularVelocity );
		cubeState.enabled = RUBY_BOOLEAN( rb_iv_get( rubyState, "@enabled" ) );
		simulation->SetCubeState( RUBY_INT( id ), cubeState );
		return Qnil;
        RUBY_END()
    }

    RubyMethod getCubeState( VALUE self, VALUE id )
    {
        RUBY_BEGIN( "Simulation.removeCube" )
	    NewSimulation * simulation = internal( self );
		SimulationCubeState cubeState;
		simulation->GetCubeState( RUBY_INT( id ), cubeState );
		VALUE rubyCubeState = RUBY_NEW( "SimulationCubeState" );
		rb_iv_set( rubyCubeState, "@position", NewRubyVector(cubeState.position) );
		rb_iv_set( rubyCubeState, "@orientation", NewRubyQuaternion(cubeState.orientation) );
		rb_iv_set( rubyCubeState, "@linearVelocity", NewRubyVector(cubeState.linearVelocity) );
		rb_iv_set( rubyCubeState, "@angularVelocity", NewRubyVector(cubeState.angularVelocity) );
		rb_iv_set( rubyCubeState, "@enabled", cubeState.enabled ? Qtrue : Qfalse );
		return rubyCubeState;
        RUBY_END()
    }

    RubyMethod addPlane( VALUE self, VALUE rubyNormal, VALUE d )
    {
        RUBY_BEGIN( "Simulation.addPlane" )
		math::Vector normal( 0, 0, 1 );
		RubyVector( rubyNormal, normal );
	    NewSimulation * simulation = internal( self );
		simulation->AddPlane( normal, RUBY_FLOAT(d) );
		return Qnil;
        RUBY_END()
    }

    RubyMethod reset( VALUE self )
    {
        RUBY_BEGIN( "Simulation.reset" )
	    NewSimulation * simulation = internal( self );
		simulation->Reset();
		return Qnil;
        RUBY_END()
    }
};

class RubyPacket : public ComplexRubyObject<Packet>
{
public:

    static void registerClass()
    {
        DefineClass( "Packet" );
        AddStaticMethod( "new", rubyNew, 1 );
        AddMethod( "beginWrite", beginWrite, 0 );
        AddMethod( "writeBoolean", writeBoolean, 1 );
        AddMethod( "writeByte", writeByte, 1 );
        AddMethod( "writeInteger", writeInteger, 1 );
        AddMethod( "writeFloat", writeFloat, 1 );
        AddMethod( "readBoolean", readBoolean, 0 );
        AddMethod( "readByte", readByte, 0 );
        AddMethod( "readInteger", readInteger, 0 );
        AddMethod( "readFloat", readFloat, 0 );
    }

protected:

    RubyMethod rubyNew( VALUE rubyClass, VALUE maximumPacketSize )
    {
        RUBY_BEGIN( "Packet.new" )
		VALUE object = RUBY_NEW( "Packet" );
		assert( object != Qnil );
		Packet * instance = new Packet( RUBY_INT( maximumPacketSize ) );
 		ComplexRubyObject<Packet> * complexObject = new ComplexRubyObject<Packet>( instance );
		wrap( object, complexObject );
        return object;
		RUBY_END()
    }

    RubyMethod beginWrite( VALUE self )
    {
        RUBY_BEGIN( "Packet.beginWrite" )
 		Packet * packet = internal( self );
		packet->BeginWrite();
		return Qnil;
        RUBY_END()
    }

    RubyMethod writeBoolean( VALUE self, VALUE rubyBoolean )
    {
        RUBY_BEGIN( "Packet.writeBoolean" )
 		Packet * packet = internal( self );
		return packet->WriteBoolean( RUBY_BOOLEAN( rubyBoolean ) ) ? Qtrue : Qnil;
        RUBY_END()
    }

    RubyMethod writeByte( VALUE self, VALUE rubyByte )
    {
        RUBY_BEGIN( "Packet.writeByte" )
 		Packet * packet = internal( self );
		return packet->WriteByte( RUBY_INT( rubyByte ) ) ? Qtrue : Qnil;			// todo: check value is in [0,255]
        RUBY_END()
    }

    RubyMethod writeInteger( VALUE self, VALUE rubyInteger )
    {
        RUBY_BEGIN( "Packet.writeInteger" )
 		Packet * packet = internal( self );
		return packet->WriteInteger( RUBY_INT( rubyInteger ) ) ? Qtrue : Qnil;
        RUBY_END()
    }

    RubyMethod writeFloat( VALUE self, VALUE rubyFloat )
    {
        RUBY_BEGIN( "Packet.writeFloat" )
 		Packet * packet = internal( self );
		return packet->WriteFloat( RUBY_FLOAT( rubyFloat ) ) ? Qtrue : Qnil;
        RUBY_END()
    }

    RubyMethod readBoolean( VALUE self )
    {
        RUBY_BEGIN( "Packet.readBoolean" )
 		Packet * packet = internal( self );
		bool boolean;
		if ( !packet->ReadBoolean( boolean ) )
			return Qnil;
		return boolean ? Qtrue : Qfalse;
        RUBY_END()
    }

    RubyMethod readByte( VALUE self )
    {
        RUBY_BEGIN( "Packet.readByte" )
 		Packet * packet = internal( self );
		unsigned char byte;
		if ( !packet->ReadByte( byte ) )
			return Qnil;
		return RubyInt::create( byte );
        RUBY_END()
    }

    RubyMethod readInteger( VALUE self )
    {
        RUBY_BEGIN( "Packet.readInteger" )
 		Packet * packet = internal( self );
		unsigned int integer;
		if ( !packet->ReadInteger( integer ) )
			return Qnil;
		return RubyInt::create( integer );
        RUBY_END()
    }

    RubyMethod readFloat( VALUE self )
    {
        RUBY_BEGIN( "Packet.readFloat" )
 		Packet * packet = internal( self );
		float floatValue;
		if ( !packet->ReadFloat( floatValue ) )
			return Qnil;
		return RubyFloat::create( floatValue );
        RUBY_END()
    }
};

class RubyConnection : public ComplexRubyObject<AckConnection>
{
public:

    static void registerClass()
    {
        DefineClass( "Connection" );
        AddStaticMethod( "new", rubyNew, 0 );
        AddMethod( "listen", listen, 1 );
        AddMethod( "connect", connect, 1 );
        AddMethod( "connected?", connected, 0 );
		AddMethod( "timeout?", timeout, 0 );
        AddMethod( "send", send, 2 );
        AddMethod( "receive", receive, 2 );
		AddMethod( "update", update, 2 );
		AddMethod( "rtt", rtt, 0 );
		AddMethod( "sent_packets", sent_packets, 0 );
		AddMethod( "acked_packets", acked_packets, 0 );
		AddMethod( "lost_packets", lost_packets, 0 );
		AddMethod( "sent_bandwidth", sent_bandwidth, 0 );
		AddMethod( "acked_bandwidth", acked_bandwidth, 0 );
    }

protected:

    RubyMethod rubyNew( VALUE rubyClass, VALUE packetSize )
    {
        RUBY_BEGIN( "Connection.new" )
		VALUE object = RUBY_NEW( "Connection" );
		AckConnection * instance = new AckConnection();
 		ComplexRubyObject<AckConnection> * complexObject = new ComplexRubyObject<AckConnection>( instance );
		wrap( object, complexObject );
        return object;
		RUBY_END()
    }

    RubyMethod listen( VALUE self, VALUE port )
    {
        RUBY_BEGIN( "Connection.listen" )
 		AckConnection * connection = internal( self );
		return connection->Listen( RUBY_INT(port) ) ? Qtrue : Qfalse;
        RUBY_END()
    }

    RubyMethod connect( VALUE self, VALUE rubyAddress )
    {
        RUBY_BEGIN( "Connection.connect" )
 		AckConnection * connection = internal( self );
		Address address;
		RubyAddress( rubyAddress, address );
		return connection->Connect( address ) ? Qtrue : Qnil;
        RUBY_END()
    }

    RubyMethod connected( VALUE self )
    {
        RUBY_BEGIN( "Connection.connected?" )
 		AckConnection * connection = internal( self );
		return connection->IsConnected() ? Qtrue : Qfalse;
        RUBY_END()
    }

    RubyMethod timeout( VALUE self )
    {
        RUBY_BEGIN( "Connection.timeout?" )
 		AckConnection * connection = internal( self );
		return connection->IsTimedOut() ? Qtrue : Qfalse;
        RUBY_END()
    }

    RubyMethod send( VALUE self, VALUE rubyPacket, VALUE time )
    {
        RUBY_BEGIN( "Connection.send" )
 		AckConnection * connection = internal( self );
		Packet * packet = RubyPacket::internal( rubyPacket );
		assert( packet );
		return connection->Send( *packet, RUBY_FLOAT(time) ) ? Qtrue : Qfalse;
        RUBY_END()
    }

	RubyMethod receive( VALUE self, VALUE rubyPacket, VALUE time )
	{
        RUBY_BEGIN( "Connection.receive" )
	 	AckConnection * connection = internal( self );
		return connection->Receive( *RubyPacket::internal(rubyPacket), RUBY_FLOAT(time) ) ? Qtrue : Qfalse;
		RUBY_END()
	}

    RubyMethod update( VALUE self, VALUE time, VALUE deltaTime )
    {
        RUBY_BEGIN( "Connection.update" )
 		AckConnection * connection = internal( self );
		connection->Update( RUBY_FLOAT(time), RUBY_FLOAT(deltaTime) );
		return Qnil;
        RUBY_END()
    }

    RubyMethod rtt( VALUE self )
    {
        RUBY_BEGIN( "Connection.rtt" )
 		AckConnection * connection = internal( self );
		return RubyFloat::create( connection->GetRoundTripTime() );
        RUBY_END()
    }

    RubyMethod sent_packets( VALUE self )
    {
        RUBY_BEGIN( "Connection.sent_packets" )
 		AckConnection * connection = internal( self );
		return RubyInt::create( connection->GetSentPackets() );
        RUBY_END()
    }

    RubyMethod acked_packets( VALUE self )
    {
        RUBY_BEGIN( "Connection.acked_packets" )
 		AckConnection * connection = internal( self );
		return RubyInt::create( connection->GetAckedPackets() );
        RUBY_END()
    }

    RubyMethod lost_packets( VALUE self )
    {
        RUBY_BEGIN( "Connection.lost_packets" )
 		AckConnection * connection = internal( self );
		return RubyInt::create( connection->GetLostPackets() );
        RUBY_END()
    }

    RubyMethod sent_bandwidth( VALUE self )
    {
        RUBY_BEGIN( "Connection.sent_bandwidth" )
 		AckConnection * connection = internal( self );
		return RubyFloat::create( connection->GetSentBandwidth() );
        RUBY_END()
    }

    RubyMethod acked_bandwidth( VALUE self )
    {
        RUBY_BEGIN( "Connection.acked_bandwidth" )
 		AckConnection * connection = internal( self );
		return RubyFloat::create( connection->GetAckedBandwidth() );
        RUBY_END()
    }
};

struct Dummy {};

class RubyPlatform : public RubyObject<Dummy>
{
public:

    static void registerClass()
    {
        DefineClass( "Platform" );
		AddStaticMethod( "sleep", sleep, 1 );			// todo: this needs to be higher accuracy for the server
    }

protected:

    RubyMethod sleep( VALUE rubyClass, VALUE seconds )
    {
        RUBY_BEGIN( "Platform.sleep" )
		net::wait_seconds( RUBY_FLOAT(seconds) );
		return Qnil;
		RUBY_END()
    }
};

class RubyNet : public RubyObject<Dummy>
{
public:

    static void registerClass()
    {
        DefineClass( "Net" );
		AddStaticMethod( "initialize", initialize, 0 );
		AddStaticMethod( "shutdown", shutdown, 0 );
    }

protected:

    RubyMethod initialize( VALUE rubyClass )
    {
        RUBY_BEGIN( "Net.initialize" )
		InitializeSockets();
		return Qnil;
		RUBY_END()
    }

    RubyMethod shutdown( VALUE rubyClass )
    {
        RUBY_BEGIN( "Net.shutdown" )
		ShutdownSockets();
		return Qnil;
		RUBY_END()
    }
};
	
extern "C" void Init_Sandbox()
{
    RubySimulation::registerClass();
	RubyPacket::registerClass();
	RubyConnection::registerClass();
	RubyPlatform::registerClass();
	RubyNet::registerClass();
}

int main()
{
	printf( "dummy main\n" );
}
