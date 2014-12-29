#include "CommandLine.h"
#include "Global.h"
#include "Console.h"
#include "ReplayManager.h"
#include <string>

static char commandLineBuffer[CommandLineBufferSize];

void StoreCommandLine( int argc, char * argv[] )
{
    commandLineBuffer[0] = '\0';
    for ( int i = 1; i < argc; ++i )
    {
        // todo: this should be strncat!!!
        strcat( commandLineBuffer, argv[i] );
        if ( i != argc -1 )
            strcat( commandLineBuffer, " " );
    }
}

void StoreCommandLine( const char * commandLine )
{
    strncpy( commandLineBuffer, commandLine, CommandLineBufferSize );
    commandLineBuffer[CommandLineBufferSize-1] = '\0';
}

void ProcessCommandLine()
{
    // todo: implement proper command line parsing, eg. "-blah", "-blah = X", "+command something something something -switch"

//    printf( "command line: '%s'\n", buffer );

#ifdef CLIENT

    const char replayFile[] = "replay.bin";

    bool playback = false;

    if ( strcmp( commandLineBuffer, "+load stone" ) == 0 )
    {
        global.console->ExecuteCommand( "load stone" );
    }
    else if ( strcmp( commandLineBuffer, "+load cubes" ) == 0 )
    {
        global.console->ExecuteCommand( "load cubes" );
    }
    else if ( strcmp( commandLineBuffer, "+load lockstep" ) == 0 )
    {
        global.console->ExecuteCommand( "load lockstep" );
    }
    else if ( strcmp( commandLineBuffer, "+load snapshot" ) == 0 )
    {
        global.console->ExecuteCommand( "load snapshot" );
    }
    else if ( strcmp( commandLineBuffer, "+playback" ) == 0 )
    {
        global.replayManager->StartPlayback( replayFile );
        playback = true;
    }
    else
    {
        global.console->Activate();
    }

    if ( !global.replayManager->IsPlayback() && !playback )
    {
        global.replayManager->StartRecording( replayFile );

        global.replayManager->RecordCommandLine( commandLineBuffer );
    }

#endif // #ifdef CLIENT
}
