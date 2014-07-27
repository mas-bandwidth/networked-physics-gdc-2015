/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_CLIENT_H
#define PROTOCOL_CLIENT_H

#include "Packets.h"
#include "Resolver.h"
#include "Connection.h"
#include "NetworkInterface.h"
#include "ClientServerDataBlock.h"

namespace protocol
{
    class Allocator;
    class NetworkSimulator;

    struct ClientConfig
    {
        Allocator * allocator = nullptr;                        // allocator used for objects with the same life cycle as this object. if null is passed in, default allocator is used.

        uint16_t defaultServerPort = 10000;                     // the default server port. used when resolving by hostname and address port is zero.

        float connectingTimeOut = 5.0f;                         // number of seconds before timeout for any situation *before* the client establishes connection
        float connectedTimeOut = 10.0f;                         // number of seconds in the connected state before timing out if no connection packet received

        float connectingSendRate = 10.0f;                       // client send rate while connecting
        float connectedSendRate = 30.0f;                        // client send rate *after* being connected, eg. connection packets

        #if PROTOCOL_USE_RESOLVER
        Resolver * resolver = nullptr;                          // optional resolver used to to lookup server address by hostname.
        #endif
        
        NetworkInterface * networkInterface = nullptr;          // network interface used to send and receive packets. required.
        ChannelStructure * channelStructure = nullptr;          // channel structure for connections. required.

        Block * clientData = nullptr;                           // data sent from client to server on connect. must be constant. this block is not owned by us (we don't destroy it)
        int maxServerDataSize = 256 * 1024;                     // maximum size for data received from server on connect. if the server data is larger than this then the connect will fail.
        int fragmentSize = 1024;                                // send client data in 1k fragments by default. a good size given that MTU is typically 1200 bytes.
        int fragmentsPerSecond = 60;                            // number of fragment packets to send per-second. set pretty high because we want the data to get across quickly.

        NetworkSimulator * networkSimulator = nullptr;          // optional network simulator.
    };

    class Client
    {
        const ClientConfig m_config;

        Allocator * m_allocator;

        TimeBase m_timeBase;

        Connection * m_connection;
        PacketFactory * m_packetFactory;                        // IMPORTANT: We don't own this pointer. It's owned by the network interface!

        Address m_address;
        ClientState m_state = CLIENT_STATE_DISCONNECTED;
        uint64_t m_clientGuid = 0;
        uint64_t m_serverGuid = 0;
        double m_accumulator = 0.0;
        double m_lastPacketReceiveTime = 0.0;
        ClientError m_error = CLIENT_ERROR_NONE;
        uint32_t m_extendedError = 0;

        ClientServerDataBlockSender * m_dataBlockSender = nullptr;
        ClientServerDataBlockReceiver * m_dataBlockReceiver = nullptr;

        char m_hostname[MaxHostName];

        Client( const Client & other );
        Client & operator = ( const Client & other );

    public:

        Client( const ClientConfig & config );

        ~Client();

        void Connect( const Address & address );

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

        NetworkInterface * GetNetworkInterface() const;

        Connection * GetConnection() const;

        const Block * GetServerData() const;

        uint64_t GetClientGuid() const { return m_clientGuid; }
        
        uint64_t GetServerGuid() const { return m_serverGuid; }

        void Update( const TimeBase & timeBase );

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

        void SendPacket( Packet * packet );
    };
}

#endif
