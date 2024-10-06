#include <iostream>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <string>
#include <tuple>
#include <vector>
#include <sstream>
#include <map>
#include "requestparse.h"
#pragma comment(lib,"ws2_32.lib")


//User-Agent: PostmanRuntime/7.42.0
//Accept: "*/*"
//Cache - Control: no - cache
//Postman - Token : d93b3b5c - b02a - 4048 - b104 - 858edbdfcf27
//Host : localhost:4221
//Accept - Encoding : gzip, deflate, br
//Connection : keep - alive

struct HttpRequest : public RequestParse {
	std::string method;
	std::string path;
	std::string version;
	std::map<std::string, std::string> headers;
	std::string body;
public:
	
	void InitHttpRequest(const std::string& requset)
	{
		auto [path, tokens] = getPath(requset);
		if (!tokens.empty())
		{
			std::vector<std::string> paths = SplitMessage(tokens[0], " ");
			//Get /path HTTP/1.1

			if (paths.size() >= 3)
			{
				this->method = paths[0];
				this->path = path;
				this->version = paths[2];
			}
			else
			{
				std::cerr << "Request is not valid" << std::endl;

			}
		}
		
		//User-Agent: PostmanRuntime/7.42.0
		//key: value
		//第一个元素是get 路径 然后协议所以不需要
		for (size_t i = 1; i < tokens.size(); i++)
		{
			std::vector<std::string> header = SplitMessage(tokens[i], ": ");
			if (header.size() == 2)
			{
				headers[header[0]] = header[1];
			}
			else
			{
                std::clog << "Header is not valid" << std::endl;
			}
		}
		std::vector<std::string> bodyTokens = SplitMessage(path, "/");
		// /echo/abc  只需要abc
		body = bodyTokens.back();
	}

private:
	std::tuple<std::string, std::vector<std::string>> getPath(const std::string& request) override
	{
		std::vector<std::string> tokens = SplitMessage(request, "\r\n");
		std::vector<std::string> paths = SplitMessage(tokens[0], " ");
		//返回的是完整路径比如/echo/abc 
		//一个返回的是字符串数据根据换号符分割的字符串数组
		this->requestMesssage = { paths[1], tokens };
		return requestMesssage;
	}
	//分割字符串
	std::vector<std::string> SplitMessage(const std::string& message, const std::string& delim) override
	{
		if (delim.empty())
		{
			std::clog << "Delim is empty" << std::endl;
            return {};
		}
		std::vector<std::string> tokens;
		std::stringstream ss = std::stringstream{ message };
		std::string line;
		//寻找更好的处理方式
		if (message.starts_with("Host"))
		{
			tokens.emplace_back("Host");
			tokens.emplace_back(message.substr(6));
            return tokens;
		}
		while (std::getline(ss, line, *delim.begin()))
		{
			tokens.push_back(line);
			//"\r\n" 遇到\就忽略后面3个不读了
			//delim.length() - 1 的类型是 size_t，size_t 类型是无符号整数，而 std::streamsize 是有符号整数。
			//将 size_t 类型的值赋值给 std::streamsize 类型的变量时，会触发类型转换，导致溢出
			
			ss.ignore(static_cast<std::streamsize>(delim.length() - 1));

		}
		return tokens;
	}
};

struct HttpResponse {
	std::string status;
	std::string contentType;
	std::tuple<std::string, size_t> contentLength;
	std::string body;


public:
	std::string GetResponse()
	{
		auto[ctlString, contentLength] = getContentLength();
		return status + contentType + ctlString + std::to_string(contentLength) + "\r\n\r\n" + body;

	}
private:
	std::tuple<std::string, size_t> getContentLength() 
	{
		return contentLength;
	}

};




int main(int argc, char* argv[])
{
	//刷新缓冲区
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;
    // Initialize Winsock
	WSADATA data{};
	
	if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
	{
		std::cerr << "Failed to initialize winsock. Error Code : " << WSAGetLastError() << std::endl;
		return 1;
	}
    // Create a socket
	//ipv4 tcp
	SOCKET serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd == INVALID_SOCKET)
	{
		std::cerr << "Failed to create server socket" << WSAGetLastError() << std::endl;
		return 1;
	}
	int resue = 1;
	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&resue, sizeof(resue)) == SOCKET_ERROR)
	{
		std::cerr << "Failed to setsockopt" << WSAGetLastError() << std::endl;
        return 1;
	}
	// Bind the socket to an IP / port
	sockaddr_in sockAddr{};
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(4221);
	//这行代码将服务器绑定到所有可用的网络接口上。
	sockAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	//inet_pton(AF_INET, "127.0.0.1", &sockAddr.sin_addr.S_un.S_addr);
	

	if (bind(serverFd, (sockaddr*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
		std::cerr << "Failed to bind to port 4221\n";
		return 1;
	}
	// Listen for connections

	int connectionBackLog = 5;
	if (listen(serverFd, connectionBackLog) == SOCKET_ERROR)
	{
		std::cerr << "Failed to listen on port 4221\n";
        return 1;

	}
	// Accept a client connection
	sockaddr_in clientAddr{};
	int clientAddrLen = sizeof(clientAddr);
	std::cout << "Waiting for a client to connect...\n";
	SOCKET clientFd = accept(serverFd, (sockaddr*)&clientAddr, &clientAddrLen);
	if (clientFd == INVALID_SOCKET)
	{
		std::cerr << "Failed to accept client connection\n";
		closesocket(serverFd);
		return 1;
	}
	//std::string message = "HTTP/1.1 200 OK\r\n\r\n";
	//GET /index.html HTTP/1.1\r\nHost: localhost:4221\r\nUser-Agent: curl/7.64.1\r\nAccept: */*\r\n\r\n
	std::string clientMessage(1024, '\0');
	int brecvd = recv(clientFd, const_cast<char*>(clientMessage.c_str()), clientMessage.size(), 0);
	
	if (brecvd < 0)
	{
		std::cerr << "Failed to receive data from client" << WSAGetLastError() << "\n";
		closesocket(serverFd);
		closesocket(clientFd);
		WSACleanup();
		return 1;
	} 
	else if (brecvd == 0)
	{
		std::clog << "no bytes read" << std::endl;
	}
	else
	{
		std::clog << "Client Message (length: " << clientMessage.size() << ")" << std::endl;
		std::clog << clientMessage << std::endl;
		//结构化绑定
		HttpRequest httpRequest;
		httpRequest.InitHttpRequest(clientMessage);

		std::string response;

		std::string testResponse("Hello World!");

		HttpResponse httpResponse{ "HTTP/1.1 200 OK\r\n", "Content-Type: TEXT/plain\r\n",
			{"Content-Length:", testResponse.length()}, testResponse};
			
		try
		{
			if (httpRequest.path == "/")
			{
				response = httpResponse.GetResponse();
			}
			else if (httpRequest.path == "/echo/abc")
			{
				response = httpResponse.GetResponse();
				
			}
			else if (httpRequest.path == "/user-agent")
			{

				std::string userAgent = httpRequest.headers["User-Agent"];
				response = "HTTP/1.1 200 OK\r\nContent-Type: TEXT/plain\r\nContent-Length:"
					+ std::to_string(userAgent.size()) + "\r\n\r\n" + userAgent;


			}
			else
			{
				response = "HTTP/1.1 404 Not Found\n";
			}
		}
		catch(const std::out_of_range& e)
		{
			
			std::cout << __LINE__  << e.what() << std::endl;
		}

		if (send(clientFd, response.c_str(), response.size(), 0) == SOCKET_ERROR)
		{
			std::cerr << "Failed to send data to client\n";
			closesocket(serverFd);
			closesocket(clientFd);
			WSACleanup();
			return 1;
		}
	}
	

    closesocket(serverFd);
    closesocket(clientFd);

	WSACleanup();
	return 0;
}