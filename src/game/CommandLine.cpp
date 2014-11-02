#include "Global.h"
#include "Console.h"
#include <string>

static bool stone_demo = false;
static bool cubes_demo = false;
static bool interpolation_demo = false;

void ParseCommandLine( int argc, char * argv[] )
{
    // HACK HACK HACK -- this is a big hack for now!!!

    // todo: implement proper command line parsing, eg. "-blah", "-blah = X", "+command something something something -switch"

    char buffer[2048];
    buffer[0] = '\0';
    for ( int i = 1; i < argc; ++i )
    {
        strcat( buffer, argv[i] );
        if ( i != argc -1 )
            strcat( buffer, " " );
    }

//    printf( "command line: '%s'\n", buffer );

    if ( strcmp( buffer, "+load stone" ) == 0 )
        stone_demo = true;
    else if ( strcmp( buffer, "+load cubes" ) == 0 )
        cubes_demo = true;
    else if ( strcmp( buffer, "+load interpolation" ) == 0 )
        interpolation_demo = true;
}

void CommandLinePostGameInit()
{
    if ( stone_demo )
        global.console->ExecuteCommand( "load stone" );
    else if ( cubes_demo )
        global.console->ExecuteCommand( "load cubes" );
    else if ( interpolation_demo )
        global.console->ExecuteCommand( "load interpolation" );
    else
        global.console->Activate();
}
