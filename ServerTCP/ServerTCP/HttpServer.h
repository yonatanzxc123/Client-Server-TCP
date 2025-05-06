#pragma once
#define NOMINMAX         
#include "HttpConnection.h"
#include <vector>
#include <memory>
#include <winsock2.h>

namespace common
{

	class Server
	{
	private:
		SOCKET listenSock;
		std::vector<std::unique_ptr<Connection>> conns;

		void acceptNew();
		void buildFdSets(fd_set& rd, fd_set& wr);
		void sweepIdle();

	public:
		Server();
		~Server();
		void run();
	};


}