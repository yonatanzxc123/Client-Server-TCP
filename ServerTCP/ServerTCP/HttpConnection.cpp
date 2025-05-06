#include "HttpConnection.h"
#include <ws2tcpip.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <map>

namespace common
{

    Connection::Connection(SOCKET s)
        : sock(s), lastActivity(time(nullptr))
    {
        u_long nb = 1;
        ioctlsocket(sock, FIONBIO, &nb);
    }

    Connection::~Connection() {
        closesocket(sock);
    }

    void Connection::recvChunk() {
        int n = ::recv(sock, ioBuf, IO_BUF, 0);
        if (n <= 0) throw std::runtime_error("closed");
        reqBuf.append(ioBuf, n);
        lastActivity = time(nullptr);
        if (requestComplete(reqBuf))
        {
            processRequest();
            recvState = RecvState::IDLE;
            sendState = SendState::SEND;
        }
    }

    void Connection::sendChunk() {
        size_t chunk = std::min(respBuf.size(), (size_t)IO_BUF);
        memcpy(ioBuf, respBuf.data(), chunk);
        int sent = ::send(sock, ioBuf, (int)chunk, 0);
        if (sent <= 0) throw std::runtime_error("closed");
        respBuf.erase(0, sent);
        lastActivity = time(nullptr);
        if (respBuf.empty()) throw std::runtime_error("done");
    }

    void Connection::processRequest() {
        std::istringstream iss(reqBuf);
        std::string methodTok;
        iss >> methodTok;
        method = toMethod(methodTok);
        switch (method)
        {
        case HttpMethod::OPTIONS: handleOptions();       break;
        case HttpMethod::GET:     handleGetHead(true);   break;
        case HttpMethod::HEAD:    handleGetHead(false);  break;
        case HttpMethod::POST:    handlePost();          break;
        case HttpMethod::PUT:     handlePut();           break;
        case HttpMethod::DELETE_: handleDelete();        break;
        case HttpMethod::TRACE:   handleTrace();         break;
        default:                  handleNotImpl(501, "Not Implemented"); break;
        }
    }

    void Connection::handleNotImpl(int code, const std::string& msg) {
        std::ostringstream o;
        o << "HTTP/1.1 " << code << " " << msg << "\r\n"
            << "Content-Length: 0\r\n"
            << "\r\n";
        respBuf = o.str();
    }

    void Connection::handleOptions() {
        std::string allow = "OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE";
        std::ostringstream o;
        o << "HTTP/1.1 200 OK\r\n"
            << "Allow: " << allow << "\r\n"
            << "Content-Length: 0\r\n"
            << "\r\n";
        respBuf = o.str();
    }

    void Connection::handleGetHead(bool sendBody) {
        // Parse request line
        std::istringstream lineIss(reqBuf);
        std::string method, uri;
        lineIss >> method >> uri;

        // Split path and query
        std::string path = uri;
        std::map<std::string, std::string> params;
        auto qpos = uri.find('?');
        if (qpos != std::string::npos)
        {
            path = uri.substr(0, qpos);
            std::string qs = uri.substr(qpos + 1);
            std::istringstream qsIss(qs);
            std::string kv;
            while (std::getline(qsIss, kv, '&'))
            {
                auto eq = kv.find('=');
                if (eq != std::string::npos)
                {
                    params[kv.substr(0, eq)] = kv.substr(eq + 1);
                }
            }
        }
        if (path == "/") path = "/index.html";
        if (path.find("..") != std::string::npos)
        {
            handleNotImpl(400, "Bad Request"); return;
        }

        // Determine file
        std::string file = WEBROOT + path;
        if (auto it = params.find("lang"); it != params.end())
        {
            std::string lang = it->second;
            auto extPos = file.rfind('.');
            if (extPos != std::string::npos)
            {
                std::string base = file.substr(0, extPos);
                std::string alt = base + "." + lang + ".html";
                if (std::ifstream{ alt }) file = alt;
            }
        }

        std::string body;
        if (!loadFile(file, body))
        {
            handleNotImpl(404, "Not Found"); return;
        }

        std::ostringstream o;
        o << "HTTP/1.1 200 OK\r\n"
            << "Content-Length: " << (sendBody ? body.size() : 0) << "\r\n" << "\r\n";
        respBuf = sendBody ? o.str() + body : o.str();
    }

    void Connection::handlePost() {
        auto pos = reqBuf.find("\r\n\r\n");
        std::string body = (pos == std::string::npos ? "" : reqBuf.substr(pos + 4));
        std::cout << "POST body: [" << body << "]" << std::endl;
        std::ostringstream o;
        o << "HTTP/1.1 200 OK\r\n"
            << "Content-Length: 0\r\n"
            << "\r\n";
        respBuf = o.str();
    }

    void Connection::handlePut() {
        // Parse request‐line
        std::istringstream iss(reqBuf);
        std::string method, uri;
        iss >> method >> uri;

        // Simple origin‐server: reject any “..” in path
        if (uri.find("..") != std::string::npos)
        {
            handleNotImpl(400, "Bad Request");
            return;
        }

        // Map / to your default resource if you like
        std::string path = (uri == "/" ? "/index.html" : uri);
        std::string file = WEBROOT + path;

        // Extract body
        auto hdrEnd = reqBuf.find("\r\n\r\n");
        std::string body = hdrEnd == std::string::npos ? "" : reqBuf.substr(hdrEnd + 4);

        // Check if resource already exists
        bool existed = std::filesystem::exists(file);

        // Try to open
        std::ofstream ofs(file, std::ios::binary | std::ios::trunc);
        if (!ofs)
        {
            handleNotImpl(500, "Internal Server Error");
            return;
        }
        ofs << body;

        // Build response
        std::ostringstream o;
        if (!existed)
        {
            // 201 Created for new resources
            o << "HTTP/1.1 201 Created\r\n"
                << "Location: " << path << "\r\n";
        }
        else
        {
            // 200 OK for updates
            o << "HTTP/1.1 200 OK\r\n";
        }
        o << "Content-Length: 0\r\n" << "\r\n";

        respBuf = o.str();
    }

    void Connection::handleDelete() {
        std::istringstream iss(reqBuf);
        std::string m, uri;
        iss >> m >> uri;

        // Map root to index.html
        std::string path = (uri == "/" ? "/index.html" : uri);
        std::string file = WEBROOT + path;

        std::string page;
        if (!loadFile(file, page)) { handleNotImpl(404, "Not Found"); return; }
        auto endP = page.rfind("</p>");
        if (endP != std::string::npos)
        {
            auto startP = page.rfind("<p>", endP);
            if (startP != std::string::npos) page.erase(startP, endP + 4 - startP);
        }

        std::ofstream ofs(file, std::ios::binary | std::ios::trunc);
        if (!ofs) { handleNotImpl(500, "Internal Server Error"); return; }
        ofs << page;

        std::ostringstream o;
        o << "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        respBuf = o.str();
    }

    void Connection::handleTrace() {
        std::ostringstream o;
        o << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: message/http\r\n"
            << "Content-Length: " << reqBuf.size() << "\r\n"
            << "\r\n" << reqBuf;
        respBuf = o.str();
    }

    bool Connection::idleTooLong(time_t now) const {
        return now - lastActivity > IDLE_TIMEOUT;
    }

}
