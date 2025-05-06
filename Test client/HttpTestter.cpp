/*
 *  HttpTester – minimal Win‑TCP client for exercising your tiny server
 *
 *  Build:  cl /EHsc /DWIN32_LEAN_AND_MEAN HttpTester.cpp ws2_32.lib
 *  Run  :  HttpTester  METHOD  [URI]  [body]
 *
 *  Examples:
 *      HttpTester OPTIONS /
 *      HttpTester GET      /index.html?lang=he
 *      HttpTester HEAD     /index.html
 *      HttpTester POST     /echo.html   "hello‑body"
 *      HttpTester TRACE    /anything
 *      HttpTester PUT      /x.html      "new data"
 */

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib,"Ws2_32.lib")

#include <iostream>
#include <sstream>
#include <string>

constexpr const char* HOST = "127.0.0.1";
constexpr int   PORT = 27015;

void die(const char* msg) { std::cerr << msg << " (" << WSAGetLastError() << ")\n"; exit(1); }

int main(int argc,char* argv[])
{
    if(argc < 3){ std::cout <<
        "Usage: HttpTester METHOD URI [body]\n"; return 0; }

    std::string method = argv[1];
    std::string uri    = argv[2];
    std::string body   = (argc>3) ? argv[3] : "";

    WSADATA w;  WSAStartup(MAKEWORD(2,2),&w);

    SOCKET s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    sockaddr_in sin{AF_INET,htons(PORT)};
    sin.sin_addr.s_addr = inet_addr(HOST);
    if(connect(s,(sockaddr*)&sin,sizeof(sin))<0) die("connect");

    /* ---- build request ---- */
    std::ostringstream req;
    req << method << ' ' << uri << " HTTP/1.1\r\n"
        << "Host: " << HOST << "\r\n";
    if(!body.empty())
        req << "Content-Length: " << body.size() << "\r\n";
    req << "Connection: close\r\n\r\n";
    req << body;
    std::string sendBuf = req.str();

    send(s, sendBuf.data(), (int)sendBuf.size(), 0);

    /* ---- read and print response ---- */
    char buf[2048];
    int n;
    while((n = recv(s, buf, sizeof(buf)-1, 0)) > 0){
        buf[n] = 0;
        std::cout << buf;
    }
    std::cout.flush();

    closesocket(s);
    WSACleanup();
}
