#pragma once

#include <functional>
#include <condition_variable>
#include <mutex>

#include <transfer/src/Peer.h>
#include <transfer/src/TransferInterface.h>
#include "PeerLocalhost.h"

#include "boost/interprocess/exceptions.hpp"

namespace ipc
{
    class PeerLocalhost: public Transfer::Peer
    {
    public:
    	explicit PeerLocalhost(const std::string& connectedName, const Transfer::PeerType& peerType, size_t size = 4096);
        explicit PeerLocalhost(const std::string& ipcName, const Transfer::PeerType& peerType, const Transfer::SendDataType& dataType, bool noSetRecvCallback = false);

		virtual ~PeerLocalhost();

        void InitServer();
        void InitClient();

        void Send(const google::protobuf::MessageLite& message, bool async) override final;

        void SetRecvCallback(const Transfer::RecvCallback& recvCallback, const google::protobuf::MessageLite& message) override final;

		bool RecvNow(google::protobuf::MessageLite& message) override final;

    protected:
        std::string ipcName;
        Transfer::PeerType peerType;
        Transfer::SendDataType dataType;
        std::string connectedName;
        std::unique_ptr<Transfer::strong::TransferInterface> ipcObj;
        std::unique_ptr<Transfer::strong::TransferInterface> connectedPeer;
        size_t ipcSize = 0;
        char* dataToSend = nullptr;
		size_t dataToSendSize = 0;
		char* dataToRecv = nullptr;
		size_t dataToRecvSize = 0;
        Transfer::RecvCallback recvCallback;
		std::string messageToRecv;

    private:
        void RecvInternal(const char* message, size_t size);

        google::protobuf::MessageLite* messageRecv = nullptr;
		bool SetRecvCBSetted = false; //the SetRecvCallback is setted on the other side (client or server)?
		std::condition_variable condvar;
		std::mutex mutex;
		bool noSetRecvCallback = false;
    };
}   // ipc
