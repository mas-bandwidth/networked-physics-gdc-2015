#ifndef GAME_CLIENT_H
#define GAME_CLIENT_H

#include "protocol/Client.h"

extern class GameClient * CreateGameClient( core::Allocator & allocator, int clientPort = 0 );

extern void DestroyGameClient( core::Allocator & allocator, class GameClient * client );

class GameClient : public protocol::Client
{
public:

    const struct GameContext * GetGameContext() const;

    uint16_t GetPort() const;

    double GetTime() const;

protected:

    friend GameClient * CreateGameClient( core::Allocator & allocator, int clientPort );

    GameClient( const protocol::ClientConfig & config );

    void OnConnect( const network::Address & address ) override;

    void OnConnect( const char * hostname ) override;

    void OnStateChange( protocol::ClientState previous, protocol::ClientState current ) override;

    void OnDisconnect() override;

    void OnError( protocol::ClientError error, uint32_t extendedError ) override;

    void OnServerDataReceived( const protocol::Block & block ) override;
};

#endif // #ifndef GAME_CLIENT_H
