#include "PeerLocalhost.h"

namespace ipc
{
	PeerLocalhost::PeerLocalhost(const std::string& connectedName, const Transfer::PeerType& peerType, size_t size)
		: ipcName(connectedName),
		peerType(peerType),
		dataType(),
		connectedName(""),
		ipcObj(nullptr)
	{
		switch (peerType)
		{
		case Transfer::PeerType::SERVER:
			this->connectedPeer = std::make_unique<IPCPipeServer>(this->ipcName, size);
			break;
		case Transfer::PeerType::CLIENT:
			this->connectedPeer = std::make_unique<IPCPipeClient>(this->ipcName, size);
			break;
		};
	}

	PeerLocalhost::PeerLocalhost(const std::string& ipcName, const Transfer::PeerType& peerType, const Transfer::SendDataType& dataType, bool noSetRecvCallback)
		: ipcName (ipcName),
		peerType (peerType),
		dataType (dataType),
		connectedName (""),
		ipcObj (nullptr),
		connectedPeer(nullptr),
		noSetRecvCallback (noSetRecvCallback)
	{
		switch (dataType)
		{
		case Transfer::SendDataType::REAL_TIME:
			this->ipcSize = 4096;
			this->dataToSendSize = this->ipcSize * 4;
			this->dataToSend = new char[this->dataToSendSize];
			this->dataToRecvSize = this->dataToSendSize;
			this->dataToRecv = new char[this->dataToRecvSize];
			break;
		case Transfer::SendDataType::BIG_DATA:
			this->ipcSize = 65536;
			this->dataToSendSize = this->ipcSize;
			this->dataToSend = new char[this->dataToSendSize];
			this->dataToRecvSize = this->dataToSendSize;
			this->dataToRecv = new char[this->dataToRecvSize];
			break;
		};

		switch (this->peerType)
		{
		case Transfer::PeerType::SERVER:
			this->InitServer();
			break;
		case Transfer::PeerType::CLIENT:
			this->InitClient();
			break;
		};

		if (this->noSetRecvCallback)
		{
			size_t realSize = 0;
			this->ipcObj->RecvNow(this->dataToRecv, this->dataToSendSize, &realSize);
			assert(this->messageToRecv == "SetRecvCallback");
			if (strncmp (this->dataToRecv, "SetRecvCallback", realSize) == 0)
			{
				std::unique_lock<std::mutex> lock(this->mutex);
				this->SetRecvCBSetted = true;

				this->condvar.notify_one();
			}
			this->ipcObj->Send("SetRecvCallback");
		}
		else
			this->ipcObj->SetRecvCallback2(std::bind(&PeerLocalhost::RecvInternal, this, std::placeholders::_1, std::placeholders::_2));

		this->messageToRecv.reserve(1024);
	}

	PeerLocalhost::~PeerLocalhost()
	{
		delete[] this->dataToSend;

		delete[] this->dataToRecv;
	}

	void PeerLocalhost::InitServer()
	{
		this->ipcObj = std::make_unique<IPCPipeServer>(this->ipcName, this->ipcSize);
	}

	void PeerLocalhost::InitClient()
	{
		try
		{
			if (this->ipcObj == nullptr)
			{
				this->ipcObj = std::make_unique<IPCPipeClient>(this->ipcName, this->ipcSize);
			}
		}
		catch (boost::interprocess::interprocess_exception& exc)
		{
			if (exc.get_error_code() != boost::interprocess::error_code_t::not_found_error)
			{
			}
		}
	}

	void PeerLocalhost::Send(const google::protobuf::MessageLite& message, bool async)
	{
		if (!this->SetRecvCBSetted)
		{
			std::unique_lock<std::mutex> lock(this->mutex);
			while (!this->SetRecvCBSetted)
			{
				this->condvar.wait(lock);
			}
		}

		try
		{
			const auto byteSize = message.ByteSize();
			if (static_cast<size_t>(byteSize) > this->dataToSendSize)
			{
				this->dataToSendSize = byteSize;
				delete[] this->dataToSend;
				this->dataToSend = new char[this->dataToSendSize];
			}
			message.SerializePartialToArray(this->dataToSend, this->dataToSendSize);
			this->ipcObj->Send(this->dataToSend, byteSize);
		}
		catch (...)
		{
			delete[] this->dataToSend;
		}
	}

	void PeerLocalhost::SetRecvCallback(const Transfer::RecvCallback& recvCallback, const google::protobuf::MessageLite& message)
	{
		this->recvCallback = recvCallback;

		this->messageRecv = message.New();

		this->ipcObj->Send("SetRecvCallback");
	}

	bool PeerLocalhost::RecvNow(google::protobuf::MessageLite & message)
	{
		size_t realSize = 0;
		this->ipcObj->RecvNow(this->dataToRecv, this->dataToRecvSize, &realSize);
		if (realSize > this->dataToRecvSize)
			this->dataToRecvSize = realSize;

		message.ParsePartialFromArray(this->dataToRecv, realSize);
		std::this_thread::yield();
		return true;
	}

	void PeerLocalhost::RecvInternal(const char* message, size_t size)
	{

		if (strncmp (message, "SetRecvCallback", size) == 0)
		{
			std::unique_lock<std::mutex> lock(this->mutex);
			this->SetRecvCBSetted = true;

			this->condvar.notify_one();
			return;
		}

		this->messageRecv->ParsePartialFromArray(message, size);

		if (this->recvCallback)
			this->recvCallback(this->messageRecv);

		std::this_thread::yield();
	}
}   // ipc
