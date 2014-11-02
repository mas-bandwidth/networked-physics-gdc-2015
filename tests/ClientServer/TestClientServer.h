#ifndef TEST_CLIENT_SERVER_H
#define TEST_CLIENT_SERVER_H

#include "clientServer/Client.h"
#include "clientServer/Server.h"
#include "TestMessages.h"

class TestClient : public clientServer::Client
{
public:

    TestClient( const clientServer::ClientConfig & config ) : Client( config ) {}

    const TestContext * GetTestContext() const
    {
        const protocol::Block * block = GetServerData();
        if ( block )
            return (const TestContext*) block->GetData();
        else
            return nullptr;
    }

protected:

    void OnServerDataReceived( const protocol::Block & block ) override
    {
        // IMPORTANT: As soon as the client receives the server data
        // Set it as the user context. This simulates for example, send
        // tables or other data which the client needs to deserialize
        // connection packets.
        SetContext( clientServer::CONTEXT_USER, block.GetData() );
    }
};

class TestServer : public clientServer::Server
{
public:

    TestServer( const clientServer::ServerConfig & config ) : Server( config )
    {
        CORE_ASSERT( config.serverData );
        SetContext( clientServer::CONTEXT_USER, config.serverData->GetData() );
    }

    const TestContext * GetTestContext() const
    {
        const protocol::Block * block = GetConfig().serverData;
        if ( block )
            return (const TestContext*) block->GetData();
        else
            return nullptr;
    }
};

#endif
