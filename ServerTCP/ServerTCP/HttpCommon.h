#pragma once
#include <string>
#include <ctime>

namespace common
{
	//Constants 
	inline constexpr int HTTP_PORT = 27015;
	inline constexpr int MAX_SOCKETS = 60;
	inline constexpr int IO_BUF = 1024;
	inline constexpr int IDLE_TIMEOUT = 120;

	//Enums
	enum class HttpMethod
	{ 
		NONE, OPTIONS, GET, HEAD, POST, PUT, DELETE_, TRACE 
	};
	enum class RecvState 
	{ 
		EMPTY, LISTEN, RECEIVE, IDLE 
	};
	enum class SendState 
	{ 
		EMPTY, IDLE, SEND 
	};

	//Utilities 
	std::string  httpDate();                                  
	HttpMethod   toMethod(const std::string& token);
	bool requestComplete(const std::string& buf);
	bool loadFile(const std::string& path, std::string& out);   

	inline const std::string WEBROOT = "./wwwroot";            

} 
