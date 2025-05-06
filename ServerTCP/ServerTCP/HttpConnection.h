#pragma once
#define NOMINMAX         
#include "HttpCommon.h"
#include <winsock2.h>
#include <string>

namespace common
{

    class Connection
    {
    private:
        SOCKET sock;
        RecvState recvState{ RecvState::RECEIVE };
        SendState sendState{ SendState::IDLE };
        time_t lastActivity;

        char ioBuf[IO_BUF];
        std::string reqBuf, respBuf;
        HttpMethod method{ HttpMethod::NONE };


        void processRequest();   // fills respBuf


        // per‑method helpers funcs:
        void handleGetHead(bool sendBody);
        void handlePost();
        void handleOptions();
        void handleTrace();
        void handlePut();
        void handleDelete();
        void handleNotImpl(int code = 501, const std::string& msg = "Not Implemented");





    public:
        explicit Connection(SOCKET s);
        ~Connection();


        SOCKET fd() const { return sock; }
        RecvState rcv() const { return recvState; }
        SendState snd() const { return sendState; }
        void setRecv(RecvState rs) { recvState = rs; }
        void setSend(SendState ss) { sendState = ss; }

        void recvChunk();        // called when FD_ISSET for read
        void sendChunk();        // called when FD_ISSET for write

        bool idleTooLong(time_t now) const;





    };
    
}

