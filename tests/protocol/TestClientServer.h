#ifndef TEST_CLIENT_SERVER_H
#define TEST_CLIENT_SERVER_H

#include "protocol/Client.h"
#include "protocol/Server.h"
#include "TestMessages.h"

class TestClient : public protocol::Client
{
public:

    TestClient( const protocol::ClientConfig & config ) : Client( config ) {}

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
        SetContext( protocol::CONTEXT_USER, block.GetData() );
    }
};

class TestServer : public protocol::Server
{
public:

    TestServer( const protocol::ServerConfig & config ) : Server( config )
    {
        CORE_ASSERT( config.serverData );
        SetContext( protocol::CONTEXT_USER, config.serverData->GetData() );
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
