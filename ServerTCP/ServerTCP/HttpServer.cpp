#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX         

#include "HttpServer.h"
#include <iostream>
using namespace std;
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace common;

Server::Server()
{
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == listenSock)
	{
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}
	sockaddr_in sin{ AF_INET,htons(HTTP_PORT),{INADDR_ANY} };
	bind(listenSock, (sockaddr*)&sin, sizeof(sin));
	listen(listenSock, 5);


	// Set the socket to non-blocking mode
	u_long nb = 1;
	ioctlsocket(listenSock, FIONBIO, &nb);
}

	Server::~Server() { closesocket(listenSock); }


void Server::run()
{
	while (true)
	{
		fd_set rd, wr; buildFdSets(rd, wr);
		select(0, &rd, &wr, nullptr, nullptr);

		if (FD_ISSET(listenSock, &rd)) acceptNew();
		for (auto& c : conns)
			if (c && c->rcv() == RecvState::RECEIVE &&
				FD_ISSET(c->fd(), &rd))
			{
				try {
					c->recvChunk();
				}
				catch (...) {                
					c.reset();                
				}
			}

		for (auto& c : conns)
			if (c && c->snd() == SendState::SEND &&
				FD_ISSET(c->fd(), &wr))
			{
				try {
					c->sendChunk();
				}
				catch (...) {                 
					c.reset();
				}
			}

		sweepIdle();
		conns.erase(std::remove_if(conns.begin(), conns.end(),
			[](auto& p) { return p == nullptr; }), conns.end());
	}
}


void Server::buildFdSets(fd_set& rd, fd_set& wr)
{
	FD_ZERO(&rd); FD_ZERO(&wr);
	FD_SET(listenSock, &rd);
	for (auto& c : conns) {
		if (c->rcv() == RecvState::RECEIVE) FD_SET(c->fd(), &rd);
		if (c->snd() == SendState::SEND)    FD_SET(c->fd(), &wr);
	}
}


void Server::acceptNew()
{
	SOCKET cs = accept(listenSock, nullptr, nullptr);
	conns.emplace_back(std::make_unique<Connection>(cs));
}

void Server::sweepIdle()
{
	time_t now = time(nullptr);
	for (auto& c : conns)
		if (c && c->idleTooLong(now)) c.reset();
}








