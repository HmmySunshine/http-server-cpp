#include <iostream>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <string>
#include <tuple>
#include <vector>
#include <sstream>
#pragma comment(lib,"ws2_32.lib")



std::vector<std::string> SplitMessage(const std::string& message, const std::string& delim)
{
	std::vector<std::string> tokens;
	std::stringstream ss = std::stringstream{ message };
	std::string line;
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

std::tuple<std::string, std::vector<std::string>> GetPath(const std::string& request)
{
	std::vector<std::string> tokens = SplitMessage(request, "\r\n");
	std::vector<std::string> paths = SplitMessage(tokens[0], " ");
	return { paths[1], tokens };
}

std::string GetResponse(size_t contentLength, const std::string& content)
{
    return "HTTP/1.1 200 OK\r\nContent-Type: TEXT/plain\r\nContent-Length:" + std::to_string(contentLength) + "\r\n\r\n" + content;
}


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
		auto[path, requestData] = GetPath(clientMessage);
		auto paths = SplitMessage(path, "/");
		std::string response;
		try
		{
			if (path == "/")
			{
				response = "HTTP/1.1 200 OK\r\n";
			}
			else if (paths[1] == "echo")
			{
				response = GetResponse(paths[1].size(), paths[1]);
				
			}
			else if (paths[1] == "user-agent")
			{

				std::string userAgent = requestData[1].substr(12);
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