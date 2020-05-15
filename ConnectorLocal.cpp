#include <vector>

#include "ConnectorLocal.h"

constexpr unsigned SMO_SERVER_ADDRESS_SIZE = 64;

namespace ipc
{
    PeerLocalhostServer::PeerLocalhostServer(const std::string& ipcName, const Transfer::SendDataType& dataType)
        : PeerLocalhost(ipcName, Transfer::PeerType::SERVER, dataType)
    {
    }

    PeerLocalhostServer::PeerLocalhostServer(const std::string& ipcName)
        : PeerLocalhost(ipcName, Transfer::PeerType::SERVER)
    {
    }

    ConnectedPeerLocalhostServer::ConnectedPeerLocalhostServer(const std::string& name)
        : PeerLocalhost(name, Transfer::PeerType::SERVER)
    {
    }

    std::string ConnectedPeerLocalhostServer::GetName()
    {
        const ConnectorLocalHelper helper{};

        char dataToRecv [7];
	size_t realSize = 0;
	this->connectedPeer->RecvNow(dataToRecv, 7, &realSize);

        if (strncmp (dataToRecv, "connect", realSize) == 0)
        {
		this->serverPipeName = helper.GenerateName();
        	connectedPeer->Send(serverPipeName);
        }
        else
		this->serverPipeName = "";

        return this->serverPipeName;
    }

    void ConnectedPeerLocalhostServer::SendConnect() const
    {
		this->connectedPeer->Send("connected");
		this->connectedPeer->Disconnect();
    }

	void ConnectedPeerLocalhostServer::Reconnect() const
	{
		this->connectedPeer->Reconnect();
	}

    PeerLocalhostClient::PeerLocalhostClient(const std::string& ipcName, const Transfer::SendDataType& dataType, bool noSetRecvCallback)
        : PeerLocalhost(ipcName, Transfer::PeerType::CLIENT, dataType, noSetRecvCallback) //Please use PeerMap parameter instead of strings
    {
    }

    ConnectedPeerLocalhostClient::ConnectedPeerLocalhostClient(const std::string& name)
        : PeerLocalhost(name, Transfer::PeerType::CLIENT)
    {
		this->connectedPeer->Send("connect");
    }

	std::string ConnectedPeerLocalhostClient::GetName() const
	{
		char dataToRecv[15];
		size_t realSize = 0;

		this->connectedPeer->RecvNow(dataToRecv, 15, &realSize);

		std::string clientPipeName(dataToRecv, realSize);
        	return clientPipeName;
    }

    bool ConnectedPeerLocalhostClient::WaitForConnect() const
    {
	    	char dataToRecv[9];
		size_t realSize = 0;
		this->connectedPeer->RecvNow(dataToRecv, 9, &realSize);

		if (strncmp(dataToRecv, "connected", realSize) == 0)
		{
			this->connectedPeer->Destroy();
			return true;
		}
		return false;
    }

        
    ConnectorLocal::ConnectorLocal(const ConnectorLocalParams* params) :
        ipcArbitName(params->ipcNamePrefix + params->ipcName),
        ipcNamePrefix(params->ipcNamePrefix),
        ipcName(params->ipcName),
        dataType(params->dataType)
    {
    }


    ConnectorLocalServer::ConnectorLocalServer(const ConnectorLocalParams* params) :
        ConnectorLocal(params),
        stopped(false),
        connectorHelperLocalhostServer(nullptr),
        connectedPeer(nullptr),
        waitForClientsConnect(nullptr)
    {
        if (this->ipcNamePrefix.empty())
        {
		this->connectedPeer = std::make_unique<ConnectedPeerLocalhostServer>(this->ipcName);
		this->waitForClientsConnect = std::make_unique <std::thread>([this, params]()
            	{
                	while (!this->stopped)
                	{
				this->connectedPeer->Reconnect();
                    		const auto ipcServerName = this->connectedPeer->GetName();
			    	if (!ipcServerName.empty())
			    	{
					this->serverPeers.emplace_back(std::make_shared<PeerLocalhostServer>(ipcServerName, params->dataType));

					if (this->clientConnectCallback)
					{
						this->connectedPeer->SendConnect();
						this->clientConnectCallback(&*this->serverPeers.back());
					}
			    	}
                	}		
            });
        }
    }

    ConnectorLocalServer::~ConnectorLocalServer()
    {
		this->stopped = true;
		this->waitForClientsConnect->join();
    }

    Transfer::Peer* ConnectorLocalServer::Create()
    {
		this->connectorHelperLocalhostServer = ConnectorLocalHelper::GetServerInstance(ipcArbitName);

		const auto ipcServerName = this->connectorHelperLocalhostServer->GenerateIPCName();

		this->server = std::make_unique<PeerLocalhostServer>(ipcServerName, dataType);

        	return dynamic_cast<Transfer::Peer*>(&*this->server);
    }

    void ConnectorLocalServer::OnClientConnected(const Transfer::ClientConnectCallback& callback)
    {
		this->clientConnectCallback = callback;
    }

    void ConnectorLocalServer::Disconnect(const Transfer::Peer* peer)
    {
        for (auto it = this->serverPeers.begin (); it != this->serverPeers.end (); ++it)
        {
            if (&**it == peer)
            {
		this->serverPeers.erase(it);
                break;
            }
        }
    }

    ConnectorLocalClient::ConnectorLocalClient(const ConnectorLocalParams* params) :
        ConnectorLocal(params),
        connectedPeer(nullptr)
    {
    }

    Transfer::Peer* ConnectorLocalClient::Open(bool noSetRecvCallback)
    {
        std::string ipcClientName;
             
        if (this->ipcNamePrefix.empty())
        {
			this->connectedPeer = std::make_unique<ConnectedPeerLocalhostClient>(ipcName);
            		ipcClientName = this->connectedPeer->GetName();

			this->connectedPeer->WaitForConnect();
			this->connectedPeer.reset();
        }
        else
        {
			this->connectorHelperLocalhostClient = ConnectorLocalHelper::CreateClientInstance(ipcArbitName);
            		ipcClientName = this->connectorHelperLocalhostClient->GetIPCName();
        }

		this->client = std::make_unique<PeerLocalhostClient>(ipcClientName, this->dataType, noSetRecvCallback);
        	return dynamic_cast<Transfer::Peer*>(&*this->client);
    }
}
