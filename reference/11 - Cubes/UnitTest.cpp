/*****************************************************************************
 * Unit Tests for Networking Library by Glenn Fiedler <gaffer@gaffer.org>
 * Copyright (c) Sony Computer Entertainment America 2008-2010
 ****************************************************************************/

#include "network/netDefinitions.h"
#include "network/netSockets.h"
#include "tests/UnitTest++/UnitTest++.h"
#include "tests/UnitTest++/TestRunner.h"
#include "tests/UnitTest++/TestReporterStdout.h"

#if NET_PLATFORM == NET_PLATFORM_PS3
	#include <sys/timer.h>
#elif NET_PLATFORM == NET_PLATFORM_WINDOWS
	#include <windows.h>
#elif NET_PLATFORM == NET_PLATFORM_UNIX || NET_PLATFORM == NET_PLATFORM_MAC
	#include <unistd.h>
#else
	#error unsupported net platform
#endif

#if NET_PLATFORM == NET_PLATFORM_PS3
#include <sys/process.h>
#include <sdk_version.h>
#include <cell/sysmodule.h>
SYS_PROCESS_PARAM(1000,256*1024);
#endif

class MyTestReporter : public UnitTest::TestReporterStdout
{
	virtual void ReportTestStart( UnitTest::TestDetails const & details )
	{
		printf( "%s\n", details.testName );
	}
};

inline void wait_seconds( float seconds )
{
	#if NET_PLATFORM == NET_PLATFORM_PS3
	sys_timer_usleep( (usecond_t) ( seconds * 1000000.0f ) );
	#elif NET_PLATFORM == NET_PLATFORM_WINDOWS
	Sleep( (int) ( seconds * 1000.0f ) );
	#elif NET_PLATFORM == NET_PLATFORM_UNIX || NET_PLATFORM == NET_PLATFORM_MAC
	usleep( (int) ( seconds * 1000000.0f ) );
	#else
	#error unsupported net platform
	#endif
}

int main( int argc, char * argv[] )
{
	#if NET_PLATFORM == NET_PLATFORM_PS3
	cellSysmoduleLoadModule( CELL_SYSMODULE_NET );
	#endif

	float t = 0.0f;
	float dt = 0.1f;
	net::InitializeSockets();
	while ( net::GetInitializeState() == net::Initializing )
	{
		net::InitializeUpdate();
		wait_seconds( 0.1f );
		t += dt;
		if ( t > 60.0f )
		{
			printf( "error: timeout initializing sockets\n" );
			exit(1);
		}
	}
	if ( net::GetInitializeState() == net::InitializeFailed )
	{
		printf( "error: failed to initialize sockets\n" );
		net::ShutdownSockets();
		return 1;
	}
	NET_ASSERT( net::IsInitialized() );

	MyTestReporter reporter;
	UnitTest::TestRunner runner( reporter );
	return runner.RunTestsIf( UnitTest::Test::GetTestList(), NULL, UnitTest::True(), 1000 );

	net::ShutdownSockets();

	return 0;
}

/*
========================================================================================================

     - add "node disconnected" event to make node disconnect atomic in token manager and reduce bandwidth

========================================================================================================

 	 - variable journal needs to encode "write bool" using less bits
 	   (currently journal writing lots of bools takes up *more* space than the data itself!)

     - rework reliability manager, event queue, chunk queue etc. natively implementing them
       as protocol managers -- no point wrapping classes, they'll only get used inside protocol

========================================================================================================

     - i think i need access to the subwad data structure to make token manager
       and level state machine subwad aware. *think about this it is obvious*

     - test session id increases in token manager

     - test sequence # increases, late client connect, post-transfer etc.
       in level state machine (was not working!)

     - stress test for level state machine. important to flush out random crashes and asserts

========================================================================================================
 
     - *IF* and only if stress test uncovers asserts with overlapping sessions etc...

     - think up unit test case that would break if flush state does not wait for other nodes
       to be >= token counter (eg. go to outside state while still in "waiting for session"
       with block)
	
     - implement flush state waiting for other nodes to ack >= token counter at time of flush

     - implement restart state waiting for other nodes to ack >= token counter at time of restart
       before transitioning to waiting for session.

     - implement dummy sessions and add a test to capture this case (clearly this is to put
       dummy sessions in place if you are told to server transfer while you are still server
       transferring... otherwise there would be a broken chain of sessions and the transfer
       would never complete)

   SOLUTION

     - while is flush state, keep sending out a "flush request" event once per-second with token counter
       at the time of release. receiving nodes reply with the token manager counter on their machine "flush response".
       when the flush response from all nodes says that they have exceeded the token counter sent from the flushing
       node, then it is safe to exit flush state (this stops late sessions opening and closing dummy client sessions)

     - this same trick is probably useful for the "restart" state when we get a client transfer while stuck
       in the "waiting for session" state. we want to WAIT until the server has had a chance to open/close
       sessions with us.

========================================================================================================

     - would be nice to be able to receive a number of events at once
       into an array instead of having to receive them one at a time

     - add test for signed integer serialize problem found by magnus (+128 instead of +127?)

	 - there is really no need to cache acks from the reliability system
	   they do not need to be used outside protocol managers. remove the
	   caching and the "ack overflow" error case

     - track down the off-by-one errors in the acked bandwidth vs. sent bandwidth calculations 
       (very embarassing to report a higher acked bandwidth than what was sent...)

	 - reliability system should have an interface for querying whether or not the sliding
	   window is full, eg. if sending packets would result in losing pending acks
	   this would allow the application to make the decision to stop sending packets
	   vs. powering through and potentially losing some pending acks

 	 - extend ack bits beyond just 32 bits to an arbitrary # of bits in the sliding window
       this should actually be quite easy if the reliability system implemented its
       own serialize method via net stream

     - extend event system to support concept of event channels.
       will allow one channel per-level etc.
*/

/*
	occasional test fail:
	
	test_node_join_multi
	TestNodeMesh.cpp:151: error: Failure in test_node_join_multi: !node[i]->JoinFailed()
	TestNodeMesh.cpp:151: error: Failure in test_node_join_multi: !node[i]->JoinFailed()
	TestNodeMesh.cpp:151: error: Failure in test_node_join_multi: !node[i]->JoinFailed()
	TestNodeMesh.cpp:151: error: Failure in test_node_join_multi: !node[i]->JoinFailed()
	test_node_rejoin
	test_node_timeout
	
*/

/*
    occasional fail:
    test_node_join_multi
    test_node_rejoin
    error: failed to bind socket
    z:\user\networking\testnodemesh.cpp(182): error: Failure in test_node_rejoin: no
    de.Start( "node", NodePort )
    Assertion failed: m_running, file z:\user\networking\lannode.cpp, line 98

*/

/*
	occasional hang:
		node_join_timeout
*/
