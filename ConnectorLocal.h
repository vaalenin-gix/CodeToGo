#pragma once

#include <string>
#include <random>
#include <functional>
#include <atomic>

#include <transfer/src/Connector.h>
#include "PeerLocalhost.h"
#include "ConnectorLocalHelper.h"

namespace ipc
{
    class PeerLocalhostServer : public PeerLocalhost
    {
    public:
        PeerLocalhostServer(const std::string& ipcName, const Transfer::SendDataType& dataType);
        PeerLocalhostServer(const std::string& ipcName);
        PeerLocalhostServer() = default;
        virtual ~PeerLocalhostServer() = default;
    };

    class ConnectedPeerLocalhostServer : public PeerLocalhost
    {
    public:
        ConnectedPeerLocalhostServer(const std::string& name);
        ConnectedPeerLocalhostServer() = default;
        virtual ~ConnectedPeerLocalhostServer() = default;

        std::string GetName();
        void SendConnect() const;

	void Reconnect() const;

    protected:
        std::string serverPipeName;
    };

    class PeerLocalhostClient : public PeerLocalhost
    {
    public:
        PeerLocalhostClient(const std::string& ipcName, const Transfer::SendDataType& dataType, bool noSetRecvCallback = false);
        PeerLocalhostClient() = default;
        virtual ~PeerLocalhostClient() = default;
    };

    class ConnectedPeerLocalhostClient : public PeerLocalhost
    {
    public:
        ConnectedPeerLocalhostClient(const std::string& name);
        ConnectedPeerLocalhostClient() = default;
        virtual ~ConnectedPeerLocalhostClient() = default;

        std::string GetName() const;
        bool WaitForConnect() const;

    protected:
        std::string clientPipeName;
    };

    struct ConnectorLocalParams : Transfer::ConnectorParams
    {
        ConnectorLocalParams() : ipcNamePrefix(""), ipcName("") {}

        std::string ipcNamePrefix;
        std::string ipcName;
        Transfer::SendDataType dataType;
    };


    class ConnectorLocal
    {
    public:
        ConnectorLocal() = default;
        virtual ~ConnectorLocal() = default;

        ConnectorLocal(const ConnectorLocalParams* params);

    protected:
        std::string ipcArbitName;
        std::string ipcNamePrefix;
        std::string ipcName;
        Transfer::SendDataType dataType;
    };


    class ConnectorLocalServer :
        public ConnectorLocal, public Transfer::ConnectorServer
    {
    public:
        ConnectorLocalServer() = default;
        ConnectorLocalServer(const ConnectorLocalParams* params);
        virtual ~ConnectorLocalServer();

        Transfer::Peer* Create() override;

        void OnClientConnected(const Transfer::ClientConnectCallback& callback) override;
        void Disconnect(Transfer::Peer* peer) override;

    private:
        std::atomic<bool> stopped;
        ConnectorLocalHelper* connectorHelperLocalhostServer;
        std::unique_ptr<ConnectedPeerLocalhostServer> connectedPeer;
        std::unique_ptr<std::thread> waitForClientsConnect;
        Transfer::ClientConnectCallback clientConnectCallback;

        std::vector<std::shared_ptr<Transfer::Peer>> serverPeers;
    };



    class ConnectorLocalClient :
        public ConnectorLocal, public Transfer::ConnectorClient //Please use PeerMap parameter instead of strings
    {
    public:
        ConnectorLocalClient() = default;
        ConnectorLocalClient(const ConnectorLocalParams* params);
        virtual ~ConnectorLocalClient() = default;

        Transfer::Peer* Open(bool noSetRecvCallback) override;

    private:
        std::unique_ptr<ConnectorLocalHelper> connectorHelperLocalhostClient;
        std::unique_ptr<ConnectedPeerLocalhostClient> connectedPeer;
    };
}   // ipc
