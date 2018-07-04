#include "IPCPipe.h"

#ifdef _WIN32
#include <Windows.h>
#else

#endif // _WIN32

namespace ipc
{
    class WinPipe : public Pipe
    {
    public:
        WinPipe(bool inDirection) //inDirection = true, for Server: In (Recv), Out (Send)
            : inDirection (inDirection)
            , name (R"(\\.\pipe\)")
            , size (0)
            , pipe (INVALID_HANDLE_VALUE)
            , errorCode (0)
            , buffer (nullptr)
        {
            this->isServer = false;
            this->connected = false;
        }

        virtual ~WinPipe()
        {
            delete[] buffer;
            Destroy();
        }

		void Write(const char* dataToSend, size_t size) override
		{
			BOOL success = FALSE;
			DWORD writtenBytes = 0;

			success = WriteFile(this->pipe, dataToSend, size, &writtenBytes, nullptr);
			FlushFileBuffers(this->pipe);
			if (success == TRUE)
				return;

			int numOfAttempts = 20;

			while ((success == FALSE) && (numOfAttempts != 0))
			{
				success = WriteFile(this->pipe, dataToSend, size, &writtenBytes, nullptr);
				FlushFileBuffers(this->pipe);

				if (success == FALSE)
				{
					--numOfAttempts;
					Sleep(10);
					this->errorCode = GetLastError();
				}
				else
					return;
			}

			if ((success == FALSE) && (numOfAttempts == 0))
			{
				LPCTSTR strErrorMessage = nullptr;

				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, this->errorCode, 0, (LPSTR)&strErrorMessage, 0, NULL);
				MessageBoxA(nullptr, strErrorMessage, "Write pipe ERROR", MB_OK);
			}
		}

        void Write(const std::string&  dataToSend) override
        {
            this->Write (dataToSend.c_str(), dataToSend.length());
        }

		bool Read(char*& dataToRecv, size_t dataToRecvSize, size_t* realSize) override
		{
			*realSize = 0;
			if (this->isServer)
				ConnectNamedPipe(this->pipe, nullptr);

			BOOL success = FALSE;

			DWORD readedBytes = 0;
			this->errorCode = 0;

			auto currentPointerToCopy = dataToRecv;

			while (true)
			{
				success = ReadFile(this->pipe, buffer, this->size, &readedBytes, nullptr);
				*realSize += readedBytes;
				if (*realSize > dataToRecvSize)
				{
					auto* tmp = new char[*realSize];
					const size_t diff = currentPointerToCopy - dataToRecv;
					memcpy(tmp, dataToRecv, diff);
					dataToRecvSize = *realSize;
					delete[] dataToRecv;
					dataToRecv = tmp;
					currentPointerToCopy = dataToRecv + diff;
				}

				if (success == FALSE)
				{
					this->errorCode = GetLastError();
					if (this->errorCode == ERROR_MORE_DATA)
					{
						memcpy(currentPointerToCopy, buffer, readedBytes);
						currentPointerToCopy += readedBytes;
						continue;
					}
					return false;
				}

				memcpy(currentPointerToCopy, buffer, readedBytes);

				return true;

			}
		}

        bool Read(std::string&  dataToRecv) override
        {
			return true;
        }

        void CreateServer(const std::string& name, size_t size) override
        {
            this->name.append(name);
            this->size = size;

            if (this->inDirection)
				this->pipe = CreateNamedPipeA(this->name.c_str(), PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, size, size, 0, nullptr);
            else
				this->pipe = CreateNamedPipeA(this->name.c_str(), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, size, size, 0, nullptr);

            this->errorCode = GetLastError();

            if (pipe != INVALID_HANDLE_VALUE)
            {
                this->buffer = new char[this->size];
            }

            this->isServer = true;
        }
        
        void CreateClient(const std::string& name, size_t size) override
        {
            this->name.append(name);
            this->size = size;

	        const BOOL result = WaitNamedPipe(this->name.c_str(), 20000);

			if (result == FALSE)
			{
				this->errorCode = GetLastError();
				MessageBoxA(nullptr, "Can`t connect to pipe", this->name.c_str(), MB_OK);
				return;
			}

            if (this->inDirection)
                this->pipe = CreateFileA(this->name.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            else
                this->pipe = CreateFileA(this->name.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

            this->errorCode = GetLastError();

            if (pipe != INVALID_HANDLE_VALUE)
            {
                this->buffer = new char[this->size];
            }
        }

		void Destroy() override
		{
			BOOL err = FALSE;
			if (this->pipe)
			{
				if (this->inDirection)
					err = CancelIoEx(this->pipe, nullptr);

				CloseHandle(this->pipe);
				this->pipe = nullptr;
			}
		}

		void Disconnect() override
		{
			FlushFileBuffers(this->pipe);

			BOOL err = FALSE;
			if (this->isServer)
			{
				err = DisconnectNamedPipe(this->pipe);
				this->connected = false;
			}

			if (err == FALSE)
				this->errorCode = GetLastError();

		}

		void Reconnect() override
		{
			if (this->isServer && !this->connected)
				this->connected = ConnectNamedPipe(this->pipe, NULL) != 0;
		}

        int GetErrorCode() override
        {
            return static_cast<int>(this->errorCode);
        }

    private:
        bool inDirection = false;
        std::string name;
		std::size_t size = 0;
        HANDLE pipe = nullptr;
        DWORD errorCode = 0;
        char* buffer = nullptr;
    };


#ifdef _WIN32
    typedef WinPipe OSPipe;
#else

#endif


    IPCPipe::IPCPipe() :
		readChannel(std::make_unique<OSPipe>(true)),
		writeChannel(std::make_unique<OSPipe>(false))
    {

    }

    IPCPipe::~IPCPipe()
    {
		if (this->dataToRecv)
			delete[] this->dataToRecv;

		this->readChannel.reset();
		this->writeChannel.reset();

        if (this->recvThread)
            this->recvThread->join();
    }

    void IPCPipe::SetRecvCallback(RecvCallback recvCallback)
    {
        this->recvCallback = recvCallback;
		this->recvThread = std::make_unique<std::thread>(std::bind(&IPCPipe::RecvThread, this));
    }

	void IPCPipe::SetRecvCallback2(RecvCallback2 recvCallback)
	{
		this->recvCallback2 = recvCallback;
		this->recvThread = std::make_unique<std::thread>(std::bind(&IPCPipe::RecvThread, this));
	}

	void IPCPipe::Destroy()
	{
		this->readChannel->Destroy();
		this->writeChannel->Destroy();
	}

	void IPCPipe::Disconnect()
	{
		if (isServer)
		{
			this->readChannel->Disconnect();
			this->writeChannel->Disconnect();
		}
	}

	void IPCPipe::Reconnect()
	{
		this->readChannel->Reconnect();
		this->writeChannel->Reconnect();
	}
		 
    void IPCPipe::RecvThread()
    {
		size_t realSize = 0;
        while (this->RecvNow(this->dataToRecv, this->dataToRecvSize, &realSize))
        {
			if (realSize > this->dataToRecvSize)
				this->dataToRecvSize = realSize;

            if (this->recvCallback2)
                this->recvCallback2(this->dataToRecv, realSize);

			std::this_thread::yield();
        }
    }

    void IPCPipe::Send(const std::string& message, bool async)
    {
		this->writeChannel->Write(message);
    }

	void IPCPipe::Send(const char * message, size_t size)
	{
		this->writeChannel->Write(message, size);
	}

    bool IPCPipe::RecvNow(std::string& message)
    {
        return this->readChannel->Read(message);
    }

	bool IPCPipe::RecvNow(char*& message, size_t size, size_t * realSize)
	{
		return this->readChannel->Read(message, size, realSize);
	}

    IPCPipeServer::IPCPipeServer(const std::string & name, size_t size)
    {
		this->dataToRecvSize = size * 2;
		this->dataToRecv = new char[dataToRecvSize];

		this->isServer = true;

        std::string nameIn(name);
        nameIn.append("_In");
        this->readChannel->CreateServer(nameIn, size);
    
        std::string nameOut(name);
        nameOut.append("_Out");
        this->writeChannel->CreateServer(nameOut, size);
    }

    IPCPipeClient::IPCPipeClient(const std::string& name, size_t size)
    {
		this->dataToRecvSize = size * 2;
		this->dataToRecv = new char[this->dataToRecvSize];

		std::string nameOut(name);
		nameOut.append("_In");
		this->writeChannel->CreateClient(nameOut, size);

        std::string nameIn(name);
        nameIn.append("_Out");
        this->readChannel->CreateClient(nameIn, size);
    }
}
