#define NOMINMAX         
#include "HttpServer.h"
#include <winsock2.h>
#include <iostream>

int main()
{


    // Create a WSADATA object called wsaData.
    // The WSADATA structure contains information about the Windows 
    // Sockets implementation.

    WSADATA wsaData;

	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    common::Server HttpServer;
    HttpServer.run();
}
