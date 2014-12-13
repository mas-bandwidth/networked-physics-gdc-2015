#ifndef GAME_COMMAND_LINE_H
#define GAME_COMMAND_LINE_H

const int CommandLineBufferSize = 4 * 1024 + 1;

void extern StoreCommandLine( int argc, char * argv[] );

void extern StoreCommandLine( const char * commandLine );

void extern ProcessCommandLine();

#endif // #ifndef GAME_COMMAND_LINE