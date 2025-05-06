#include "HttpConnection.h"
#include <ws2tcpip.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace common {

    Connection::Connection(SOCKET s) :sock(s), lastActivity(time(nullptr))
    {
        u_long nb = 1;
        ioctlsocket(sock, FIONBIO, &nb);
    }

    Connection::~Connection() { closesocket(sock); }

    void Connection::recvChunk()
    {
        int n = ::recv(sock, ioBuf, IO_BUF, 0);
        if (n <= 0) { throw std::runtime_error("closed"); }

        reqBuf.append(ioBuf, n);
        lastActivity = time(nullptr);

        if (requestComplete(reqBuf))
        {
            processRequest();         // fills respBuf
            recvState = RecvState::IDLE;
            sendState = SendState::SEND;
        }
    }


    void Connection::sendChunk()
    {
        size_t chunk = std::min(respBuf.size(), (size_t)IO_BUF);
        memcpy(ioBuf, respBuf.data(), chunk);
        int sent = ::send(sock, ioBuf, (int)chunk, 0);
        if (sent <= 0) { throw std::runtime_error("closed"); }

        respBuf.erase(0, sent);
        lastActivity = time(nullptr);
        if (respBuf.empty()) throw std::runtime_error("done");
    }


    void Connection::processRequest()
    {
        std::istringstream iss(reqBuf);
        std::string methodTok;
        iss >> methodTok;
        method = toMethod(methodTok);

        switch (method)
        {
        case HttpMethod::GET:     handleGetHead(true);  break;
        case HttpMethod::HEAD:    handleGetHead(false); break;
        case HttpMethod::POST:    handlePost();         break;
        case HttpMethod::OPTIONS: handleOptions();      break;
        case HttpMethod::TRACE:   handleTrace();        break;
        case HttpMethod::PUT:
        case HttpMethod::DELETE_:  handleNotImpl();      break;
        default:                  handleNotImpl();
        }

    }



    void Connection::handleNotImpl(int code, const std::string& msg)
    {
        std::ostringstream o;
        std::string body = "<html>" + msg + "</html>";
        o << "HTTP/1.1 " << code << ' ' << msg << "\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "Content-Type: text/html\r\n\r\n" << body;
        respBuf = o.str();
    }

    void Connection::handleOptions()
    {
        std::string allow = "OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE";
        respBuf = "HTTP/1.1 200 OK\r\nAllow: " + allow +
            "\r\nContent-Length: 0\r\n\r\n";
    }

    void Connection::handlePost()
    {
        auto pos = reqBuf.find("\r\n\r\n");
        std::string body = (pos == std::string::npos) ? "" : reqBuf.substr(pos + 4);
        std::cout << "POST body: [" << body << "]\n";
        handleNotImpl(200, "OK");        // simple 200 response
    }

    void Connection::handleTrace()
    {
        respBuf = "HTTP/1.1 200 OK\r\nContent-Type: message/http\r\n"
            "Content-Length: " + std::to_string(reqBuf.size()) +
            "\r\n\r\n" + reqBuf;
    }

    void Connection::handleGetHead(bool sendBody)
    {
        std::istringstream iss(reqBuf); std::string m, uri; iss >> m >> uri;

        // split URI and query
        std::string path = uri, lang;
        auto q = uri.find('?');
        if (q != std::string::npos) {
            path = uri.substr(0, q);
            auto qs = uri.substr(q + 1);
            if (qs.rfind("lang=", 0) == 0) lang = qs.substr(5);
        }
        if (path == "/" && lang != "") path = "/index." + lang + ".html";
		else if (path == "/") path = "/index.html";

        std::string file = common::WEBROOT + path;
        if (!lang.empty())
        {
            std::string alt = file + "." + lang + ".html";
            std::ifstream test(alt);
            if (test) file = alt;
        }

        std::string body;
        if (!common::loadFile(file, body)) {
            handleNotImpl(404, "Not Found"); return;
        }
        std::ostringstream hdr;
        hdr << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n"
            << "Content-Length: " << (sendBody ? body.size() : 0) << "\r\n\r\n";
        respBuf = sendBody ? hdr.str() + body : hdr.str();
    }


    bool Connection::idleTooLong(time_t now) const
    {
        return now - lastActivity > IDLE_TIMEOUT;
    }
}