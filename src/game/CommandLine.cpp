#include "Global.h"
#include "Console.h"
#include "ReplayManager.h"
#include <string>

static char commandLineBuffer[2048];

void ProcessCommandLine( int argc, char * argv[] )
{
    commandLineBuffer[0] = '\0';
    for ( int i = 1; i < argc; ++i )
    {
        strcat( commandLineBuffer, argv[i] );
        if ( i != argc -1 )
            strcat( commandLineBuffer, " " );
    }
}

void CommandLinePostGameInit()
{
    global.replayManager->RecordCommandLine( commandLineBuffer );

    // todo: implement proper command line parsing, eg. "-blah", "-blah = X", "+command something something something -switch"

//    printf( "command line: '%s'\n", buffer );

    bool stone_demo = false;
    bool cubes_demo = false;
    bool interpolation_demo = false;

    if ( strcmp( commandLineBuffer, "+load stone" ) == 0 )
        stone_demo = true;
    else if ( strcmp( commandLineBuffer, "+load cubes" ) == 0 )
        cubes_demo = true;
    else if ( strcmp( commandLineBuffer, "+load interpolation" ) == 0 )
        interpolation_demo = true;

    if ( stone_demo )
        global.console->ExecuteCommand( "load stone" );
    else if ( cubes_demo )
        global.console->ExecuteCommand( "load cubes" );
    else if ( interpolation_demo )
        global.console->ExecuteCommand( "load interpolation" );
    else
        global.console->Activate();
}
