#pragma once

#include <memory>
#include <string>
#include <thread>

#include <transfer/src/TransferInterface.h>

namespace ipc
{
    class Pipe
    {
    public:
        Pipe() = default;
        virtual ~Pipe() = 0

        virtual void Write(const std::string&  dataToSend) = 0;
		virtual void Write(const char* dataToSend, size_t size) = 0;
        virtual bool Read(std::string&  dataToRecv) = 0;
		virtual bool Read(char*& dataToRecv, size_t dataToRecvSize, size_t* realSize) = 0;

        virtual void CreateServer(const std::string& name, size_t size) = 0;
        virtual void CreateClient(const std::string& name, size_t size) = 0;

        virtual int GetErrorCode() = 0;

		virtual void Destroy() = 0;
		virtual void Disconnect() = 0;
		virtual void Reconnect() = 0;

    protected:
        bool isServer = false;
        bool connected = false;
    };


    class IPCPipe: public Transfer::TransferInterface
    {
    public:
        IPCPipe();
        virtual ~IPCPipe();

        void Send(const std::string& message, bool async = false) override; //write in 
		void Send(const char* message, size_t size) override; //write in 
        bool RecvNow(std::string& message) override;
		bool RecvNow(char*& message, size_t size, size_t* realSize) override;
        void SetRecvCallback(RecvCallback recvCallback) override;
		void SetRecvCallback2(RecvCallback2 recvCallback) override;

		void Destroy() override;
		void Disconnect() override;

		void Reconnect() override;

    protected:
		RecvCallback recvCallback;
        std::unique_ptr <std::thread> recvThread;

		std::unique_ptr<Pipe> readChannel;   //Recv 
		std::unique_ptr<Pipe> writeChannel;  //Send

		bool isServer = false;

		char* dataToRecv;
		size_t dataToRecvSize = 0;

        void RecvThread();
    };

    class IPCPipeServer: public IPCPipe
    {
    public:
        IPCPipeServer(const std::string& name, size_t size);
        virtual ~IPCPipeServer() = default;;
    };


    class IPCPipeClient: public IPCPipe
    {
    public:
        IPCPipeClient(const std::string& name, size_t size);
        virtual ~IPCPipeClient() = default;
    };
};
