// Client Server Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef CLIENT_SERVER_CLIENT_H
#define CLIENT_SERVER_CLIENT_H

#include "ClientServerEnums.h"
#include "ClientServerPackets.h"
#include "ClientServerContext.h"
#include "ClientServerDataBlock.h"
#include "ClientServerConstants.h"

namespace network
{
    class Resolver;
    class Interface;
    class Simulator;
}

namespace protocol
{
    class Block;
    class Connection;
    class ChannelStructure;
    class PacketFactory;
}

namespace clientServer
{
    struct ClientConfig
    {
        core::Allocator * allocator = nullptr;                  // allocator used for objects with the same life cycle as this object. if null is passed in, default allocator is used.

        uint16_t defaultServerPort = 10000;                     // the default server port. used when resolving by hostname and address port is zero.

        float connectingTimeOut = 5.0f;                         // number of seconds before timeout for any situation *before* the client establishes connection
        float connectedTimeOut = 10.0f;                         // number of seconds in the connected state before timing out if no connection packet received

        float connectingSendRate = 10.0f;                       // client send rate while connecting
        float connectedSendRate = 30.0f;                        // client send rate *after* being connected, eg. connection packets

        #if PROTOCOL_USE_RESOLVER
        Resolver * resolver = nullptr;                          // optional resolver used to to lookup server address by hostname.
        #endif
        
        network::Interface * networkInterface = nullptr;        // network interface used to send and receive packets. required.

        protocol::ChannelStructure * channelStructure = nullptr; // channel structure for connections. required.

        protocol::Block * clientData = nullptr;                 // data sent from client to server on connect. must be constant. this block is not owned by us (we don't destroy it)
        int maxServerDataSize = 256 * 1024;                     // maximum size for data received from server on connect. if the server data is larger than this then the connect will fail.
        int fragmentSize = 1024;                                // send client data in 1k fragments by default. a good size given that MTU is typically 1200 bytes.
        int fragmentsPerSecond = 60;                            // number of fragment packets to send per-second. set pretty high because we want the data to get across quickly.

        network::Simulator * networkSimulator = nullptr;        // optional network simulator.
    };

    class Client
    {
        const ClientConfig m_config;

        core::Allocator * m_allocator;

        core::TimeBase m_timeBase;

        protocol::Connection * m_connection;
        protocol::PacketFactory * m_packetFactory;        // IMPORTANT: We don't own this pointer. It's owned by the network interface!

        network::Address m_address;
        ClientState m_state = CLIENT_STATE_DISCONNECTED;
        uint16_t m_clientId = 0;
        uint16_t m_serverId = 0;
        double m_accumulator = 0.0;
        double m_lastPacketReceiveTime = 0.0;
        ClientError m_error = CLIENT_ERROR_NONE;
        uint32_t m_extendedError = 0;

        DataBlockSender * m_dataBlockSender = nullptr;
        DataBlockReceiver * m_dataBlockReceiver = nullptr;

        ClientServerContext m_clientServerContext;

        const void * m_context[protocol::MaxContexts];

        char m_hostname[MaxHostName];

        Client( const Client & other );
        Client & operator = ( const Client & other );

    public:

        Client( const ClientConfig & config );

        ~Client();

        void Connect( const network::Address & address );

        void Connect( const char * hostname );

        void Disconnect();

        bool IsDisconnected() const;

        bool IsConnected() const;

        bool IsConnecting() const;

        ClientState GetState() const;

        bool HasError() const;

        ClientError GetError() const;

        uint32_t GetExtendedError() const;

        void ClearError();

        #if PROTOCOL_USE_RESOLVER
        Resolver * GetResolver() const;
        #endif

        network::Interface * GetNetworkInterface() const;

        protocol::Connection * GetConnection() const;

        const protocol::Block * GetServerData() const;

        void SetContext( int index, const void * ptr );

        uint16_t GetClientId() const { return m_clientId; }
        
        uint16_t GetServerId() const { return m_serverId; }

        void Update( const core::TimeBase & timeBase );

        const ClientConfig & GetConfig() const { return m_config; }

    protected:

        void UpdateNetworkSimulator();

        void UpdateNetworkInterface();

        #if PROTOCOL_USE_RESOLVER
        void UpdateResolver();
        #endif

        void UpdateConnection();

        void UpdateSendPackets();

        void UpdateReceivePackets();

        void UpdateSendClientData();

        void ProcessDisconnected( DisconnectedPacket * packet );

        void ProcessDataBlockFragment( DataBlockFragmentPacket * packet );

        void ProcessDataBlockFragmentAck( DataBlockFragmentAckPacket * packet );

        void UpdateTimeout();

        void DisconnectAndSetError( ClientError error, uint32_t extendedError = 0 );

        void ClearStateData();

        void SendPacket( protocol::Packet * packet );

        void SetClientState( ClientState state );

        const core::TimeBase & GetTimeBase() const { return m_timeBase; }

    protected:

        virtual void OnConnect( const network::Address & address ) {}

        virtual void OnConnect( const char * hostname ) {}

        virtual void OnStateChange( ClientState previous, ClientState current ) {}

        virtual void OnDisconnect() {}

        virtual void OnError( ClientError error, uint32_t extendedError ) {}

        virtual void OnServerDataReceived( const protocol::Block & block ) {};
    };
}

#endif
