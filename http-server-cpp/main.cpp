#include <iostream>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <string>
#include <tuple>
#include <vector>
#include <sstream>
#include <map>
#include <array>
#include <algorithm>
#include <assert.h>
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
		this->headers = std::move(getHeaders(tokens));
		/*for (size_t i = 1; i < tokens.size(); i++)
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
		}*/
		std::vector<std::string> bodyTokens = SplitMessage(path, "/");
		// /echo/abc  只需要abc
		body = bodyTokens.back();
	}

private:
	std::map<std::string, std::string> getHeaders(const std::vector<std::string>& headerlines)
	{
		std::map<std::string, std::string> headers;
		for (const auto& line : headerlines)
		{
			if (line.empty())
			{
				std::clog << "请求头内容为空" << std::endl;
				break;
			}
			size_t delimPos = line.find(": ");
			if (delimPos != std::string::npos)
			{
				std::string key = line.substr(0, delimPos);
				key.erase(0, key.find_first_not_of(" \t\r\n"));
				key.erase(key.find_last_not_of(" \t\r\n") + 1);
				//Host: localhost:4221
				std::string value = line.substr(delimPos + 2);
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
				if (!value.empty() && value.back() == '\n')
				{
                    value.pop_back();
				}

				if (!value.empty() && value.back() == '\r')
				{
					value.pop_back();
				}
				headers[key] = value;
			}
			else
			{
				std::cout << "请求头内容格式可能不正确: " << line << std::endl;
			}
		}
		return headers;
	}


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
	HttpResponse(std::string status, std::string contentType, std::tuple<std::string, size_t> contentLength, std::string body)
		: status(std::move(status)), contentType(std::move(contentType)), contentLength(std::move(contentLength)), body(std::move(body))
	{
		// 确保在构造函数中就完成初始化
		auto [ctlString, len] = this->contentLength;
		if (len == 0) {
			std::cerr << "Warning: Content-Length is 0!" << std::endl;
		}
	}

public:
	std::string GetResponse()
	{
		auto [ctlString, contentLength] = getContentLength();
		return status + contentType + ctlString + std::to_string(contentLength) + "\r\n\r\n" + body;

	}
private:
	std::tuple<std::string, size_t> getContentLength()
	{
		return contentLength;
	}

};


bool ConductMsg(SOCKET clientFd)
{
	//缓冲区
	std::string clientMessage(1024, '\0');
	int brecvd = recv(clientFd, const_cast<char*>(clientMessage.c_str()), clientMessage.size(), 0);

	if (brecvd < 0)
	{
		std::cerr << "Failed to receive data from client" << WSAGetLastError() << "\n";
		return false;
	}
	else if (brecvd == 0)
	{
		std::clog << "no bytes read or Client disconnected" << std::endl;
		return false;
	}
	else
	{
		//调整字符串大小
		clientMessage.resize(brecvd);

		std::clog << "Client Message (length: " << clientMessage.size() << ")" << std::endl;
		std::clog << clientMessage << std::endl;
		//结构化绑定
		HttpRequest httpRequest;
		httpRequest.InitHttpRequest(clientMessage);

		std::string response;

		std::string testResponse("1111");
		//std::string t2 = httpRequest.body;
		HttpResponse httpResponse{ "HTTP/1.1 200 OK\r\n", "Content-Type: TEXT/plain\r\n",
			{"Content-Length:", testResponse.length()}, testResponse };

		/*
		First request 第一个请求
The first request will ask for a file that exists in the files directory:
第一个请求将请求存在于 files 目录中的文件：

$ echo -n 'Hello, World!' > /tmp/foo
$ curl -i http://localhost:4221/files/foo*/
		try
		{
			if (httpRequest.method == "GET")
			{
				if (httpRequest.path == "/")
				{
					httpResponse = { "HTTP/1.1 200 OK\r\n", "Content-Type: TEXT/plain\r\n",
						{ "Content-Length:", testResponse.length() }, testResponse };
					response = httpResponse.GetResponse();
				}
				else if (httpRequest.path.starts_with("/echo"))
				{
					httpResponse = { "HTTP/1.1 200 OK\r\n", "Content-Type: TEXT/plain\r\n",
						{ "Content-Length:", httpRequest.body.length()}, httpRequest.body};
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
			else
			{
				httpResponse = { "HTTP/1.1 405 Method Not Allowed", "text/plain", {}, "Method Not Allowed" };

			}

		}
		catch (const std::out_of_range& e)
		{

			std::cout << __LINE__ << e.what() << std::endl;
		}
		if (send(clientFd, response.c_str(), response.size(), 0) == SOCKET_ERROR)
		{
			std::cerr << "Failed to send data to client" << WSAGetLastError() << "\n";
			return false;
		}
	}
	

	return true;
}

bool CoreFunc(SOCKET serverFd, std::vector<SOCKET>& clntFds)
{
	assert(serverFd != INVALID_SOCKET);
	fd_set fdReads{};
	FD_ZERO(&fdReads);
	FD_SET(serverFd, &fdReads);
	timeval timeout{ 10, 0 };
	//std::vector<SOCKET> clntFds{};
	while (1)
	{
		std::cout << "Waiting for a client to connect...\n";
		FD_ZERO(&fdReads);
		FD_SET(serverFd, &fdReads);
		for (auto& clntFd : clntFds)
		{
			FD_SET(clntFd, &fdReads);
		}
		int readyRead = select(0, &fdReads, nullptr, nullptr, &timeout);
		if (readyRead > 0)
		{
			for (uint16_t i = 0; i < fdReads.fd_count; ++i)
			{
				if (fdReads.fd_array[i] == serverFd)
				{
					/*struct sockaddr_in clientAddr {};
                    int clientAddrLen = sizeof(clientAddr);*/
					SOCKET newClntFd = accept(serverFd, nullptr, nullptr);
					if (newClntFd == INVALID_SOCKET)
					{
						std::cerr << "Failed to accept new client connection error num:" << WSAGetLastError() << std::endl;
						return false;
					}
					FD_SET(newClntFd, &fdReads);//将新的客户端套接字添加到文件描述符集合中
					clntFds.push_back(newClntFd);
				}
				else
				{
					if (!ConductMsg(fdReads.fd_array[i]))
					{
						FD_CLR(fdReads.fd_array[i], &fdReads);
						clntFds.erase(std::remove(clntFds.begin(), clntFds.end(), fdReads.fd_array[i]), clntFds.end());
                        closesocket(fdReads.fd_array[i]);
						return false;
					}
					
					
				}
			}
		}
		else if (readyRead == 0)
		{
			std::cout << "timeout or client disconnected" << std::endl;
			continue;
		}
		else
		{
			std::cerr << "select failed bad num = " << WSAGetLastError() << std::endl;
			return false;
		}
	}

	return true;
}


int main(int argc, char* argv[])
{
	//刷新缓冲区
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;
	// ./your_program.sh --directory /tmp/
	std::string dir;
	if (argc == 3 && strcmp(argv[1], "--directory") == 0)
		dir = argv[2];

	
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
	/*sockaddr_in clientAddr{};
	int clientAddrLen = sizeof(clientAddr);
	std::cout << "Waiting for a client to connect...\n";
	SOCKET clientFd = accept(serverFd, (sockaddr*)&clientAddr, &clientAddrLen);
	if (clientFd == INVALID_SOCKET)
	{
		std::cerr << "Failed to accept client connection\n";
		closesocket(serverFd);
		return 1;
	}*/
	//std::string message = "HTTP/1.1 200 OK\r\n\r\n";
	//GET /index.html HTTP/1.1\r\nHost: localhost:4221\r\nUser-Agent: curl/7.64.1\r\nAccept: */*\r\n\r\n
	std::vector<SOCKET> clntFds;
	if (!CoreFunc(serverFd, clntFds))
	{
		
		closesocket(serverFd);
		for (const auto& clntFd : clntFds)
		{
			closesocket(clntFd);
		}
		clntFds.clear();
        WSACleanup();
		return 1;
	}
	

    closesocket(serverFd);
    //closesocket(clientFd);
	for (const auto& clntFd : clntFds)
	{
        closesocket(clntFd);
	}

	WSACleanup();
	return 0;
}