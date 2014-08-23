#ifndef GAME_CLIENT_H
#define GAME_CLIENT_H

#include "Client.h"
#include "GameMessages.h"

using namespace protocol;

class GameClient : public Client
{
public:

    GameClient( const ClientConfig & config ) : Client( config ) {}

    const GameContext * GetGameContext() const
    {
        const Block * block = GetServerData();
        if ( block )
            return (const GameContext*) block->GetData();
        else
            return nullptr;
    }

protected:

    void OnServerDataReceived( const Block & block ) override
    {
        SetContext( CONTEXT_USER, block.GetData() );
    }
};

#endif
