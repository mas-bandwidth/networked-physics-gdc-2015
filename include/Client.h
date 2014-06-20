/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_CLIENT_H
#define PROTOCOL_CLIENT_H

#include "Packets.h"
#include "Resolver.h"
#include "Connection.h"
#include "NetworkInterface.h"

namespace protocol
{
    struct ClientConfig
    {
        uint16_t defaultServerPort = 10000;                     // the default server port. used when resolving by hostname and address port is zero.

        float connectingTimeOut = 5.0f;                         // number of seconds before timeout for any situation *before* the client establishes connection
        float connectedTimeOut = 10.0f;                         // number of seconds in the connected state before timing out if no connection packet received

        float connectingSendRate = 10.0f;                       // client send rate while connecting
        float connectedSendRate = 30.0f;                        // client send rate *after* being connected, eg. connection packets

        Block * block = nullptr;                                // data block sent to server on connect. optional.
        Resolver * resolver = nullptr;                          // optional resolver used to to lookup server address by hostname.
        NetworkInterface * networkInterface = nullptr;          // network interface used to send and receive packets. required.
        ChannelStructure * channelStructure = nullptr;          // channel structure for connections. required.
    };

    class Client
    {
        const ClientConfig m_config;

        TimeBase m_timeBase;

        Connection * m_connection;
        ClientServerPacketFactory * m_packetFactory;

        std::string m_hostname;
        Address m_address;
        ClientState m_state = CLIENT_STATE_DISCONNECTED;
        uint64_t m_clientGuid = 0;
        uint64_t m_serverGuid = 0;
        double m_accumulator = 0.0;
        double m_lastPacketReceiveTime = 0.0;
        ClientError m_error = CLIENT_ERROR_NONE;
        uint32_t m_extendedError = 0;

        Client( const Client & other );
        Client & operator = ( const Client & other );

    public:

        Client( const ClientConfig & config );

        ~Client();

        void Connect( const Address & address );

        void Connect( const std::string & hostname );

        void Disconnect();

        bool IsDisconnected() const;

        bool IsConnected() const;

        bool IsConnecting() const;

        ClientState GetState() const;

        bool HasError() const;

        ClientError GetError() const;

        uint32_t GetExtendedError() const;

        Resolver * GetResolver() const;

        NetworkInterface * GetNetworkInterface() const;

        Connection * GetConnection() const;

        void Update( const TimeBase & timeBase );

        void UpdateNetworkInterface();

        void UpdateResolver();

        void UpdateConnection();

        void UpdateSendPackets();

        void UpdateReceivePackets();

        void ProcessDisconnected( DisconnectedPacket * packet );

        void UpdateTimeout();

        void DisconnectAndSetError( ClientError error, uint32_t extendedError = 0 );

        void ClearError();

        void ClearStateData();
    };
}

#endif
