#ifndef TEST_CLIENT_SERVER_H
#define TEST_CLIENT_SERVER_H

#include "Client.h"
#include "Server.h"
#include "TestMessages.h"

using namespace protocol;

class TestClient : public Client
{
public:

    TestClient( const ClientConfig & config ) : Client( config ) {}

    const TestContext * GetTestContext() const
    {
        const Block * block = GetServerData();
        if ( block )
            return (const TestContext*) block->GetData();
        else
            return nullptr;
    }

protected:

    void OnServerDataReceived( const Block & block ) override
    {
        // IMPORTANT: As soon as the client receives the server data
        // Set it as the user context. This simulates for example, send
        // tables or other data which the client needs to deserialize
        // connection packets.
        SetContext( CONTEXT_USER, block.GetData() );
    }
};

class TestServer : public Server
{
public:

    TestServer( const ServerConfig & config ) : Server( config )
    {
        CORE_ASSERT( config.serverData );
        SetContext( CONTEXT_USER, config.serverData->GetData() );
    }

    const TestContext * GetTestContext() const
    {
        const Block * block = GetConfig().serverData;
        if ( block )
            return (const TestContext*) block->GetData();
        else
            return nullptr;
    }
};

#endif
