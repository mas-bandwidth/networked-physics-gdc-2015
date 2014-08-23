#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include "Server.h"
#include "GameContext.h"
#include "GameMessages.h"

using namespace protocol;

class GameServer : public Server
{
public:

    GameServer( const ServerConfig & config ) : Server( config )
    {
        PROTOCOL_ASSERT( config.serverData );

        SetContext( CONTEXT_USER, config.serverData->GetData() );
    }
};

#endif
