#include "../include/server.h"
#include "../include/httpRequest.h"
#include "../include/httpResponse.h"
Server::Server()
	:serverSocket(INVALID_SOCKET)
{

}

bool Server::InitServer(int port)
{
	WSADATA data{};

	if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
	{
		std::cerr << "Failed to initialize winsock. Error Code : " << WSAGetLastError() << std::endl;
		return false;
	}
	// Create a socket
	//ipv4 tcp
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		std::cerr << "Failed to create server socket" << WSAGetLastError() << std::endl;
		WSACleanup();
		return false;
	}
	int resue = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&resue, sizeof(resue)) == SOCKET_ERROR)
	{
		std::cerr << "Failed to setsockopt" << WSAGetLastError() << std::endl;
		return false;
	}
	// Bind the socket to an IP / port
	sockaddr_in sockAddr{};
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(port);
	//这行代码将服务器绑定到所有可用的网络接口上。
	sockAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	//inet_pton(AF_INET, "127.0.0.1", &sockAddr.sin_addr.S_un.S_addr);


	if (bind(serverSocket, (sockaddr*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
		std::cerr << "Failed to bind to port 4221\n";
		closesocket(serverSocket);
		WSACleanup();
		return false;
	}
	// Listen for connections

	if (listen(serverSocket, connectionBackLog) == SOCKET_ERROR)
	{
		std::cerr << "Failed to listen on port 4221\n";
        closesocket(serverSocket);
        WSACleanup();
		return false;

	}
	return true;
}


bool Server::StartServer()
{
	return CoreFunc();
}

bool Server::CoreFunc()
{
	assert(serverSocket != INVALID_SOCKET);
	
	fd_set fdReads{};
	FD_ZERO(&fdReads);
	FD_SET(serverSocket, &fdReads);
	timeval timeout{ 10, 0 };
	//std::vector<SOCKET> clntFds{};
	while (1)
	{
		std::cout << "Waiting for a client to connect...\n";
		FD_ZERO(&fdReads);
		FD_SET(serverSocket, &fdReads);
		for (auto& clntFd : clientSockets)
		{
			FD_SET(clntFd, &fdReads);
		}
		int readyRead = select(0, &fdReads, nullptr, nullptr, &timeout);
		if (readyRead > 0)
		{
			for (uint16_t i = 0; i < fdReads.fd_count; ++i)
			{
				if (fdReads.fd_array[i] == serverSocket)
				{
					/*struct sockaddr_in clientAddr {};
					int clientAddrLen = sizeof(clientAddr);*/
					SOCKET newClntFd = accept(serverSocket, nullptr, nullptr);
					if (newClntFd == INVALID_SOCKET)
					{
						std::cerr << "Failed to accept new client connection error num:" << WSAGetLastError() << std::endl;
						return false;
					}
					FD_SET(newClntFd, &fdReads);//将新的客户端套接字添加到文件描述符集合中
					clientSockets.push_back(newClntFd);
				}
				else
				{
					if (!ConductMsg(fdReads.fd_array[i]))
					{
						FD_CLR(fdReads.fd_array[i], &fdReads);
						clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), 
							fdReads.fd_array[i]), clientSockets.end());
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

bool Server::ConductMsg(SOCKET clientFd)
{
	//缓冲区
	
	int brecvd = recv(clientFd, buffer, sizeof(buffer), 0);

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
		//调整字符串大小 未做
		

		std::clog << "Client Message (length: " << strlen(buffer) << ")" << std::endl;
		std::clog << buffer << std::endl;
		//结构化绑定
		HttpRequest httpRequest;
		httpRequest.InitHttpRequest(buffer);

		std::string response;

		std::string testResponse("hello");
		//std::string t2 = httpRequest.body;
		HttpResponse httpResponse{ "HTTP/1.1 200 OK\r\n", "Content-Type: TEXT/plain\r\n",
			{"Content-Length:", testResponse.length()}, testResponse };
		const std::string htmlContent = "<html><body><h1>Hello, World!</h1></body></html>";

		try
		{
			if (httpRequest.method == "GET")
			{
				if (httpRequest.path == "/")
				{
					httpResponse = { "HTTP/1.1 200 OK\r\n", "Content-Type: text/html\r\n",
						{ "Content-Length:", htmlContent.length()}, htmlContent };
					response = httpResponse.GetResponse();
				}
				else if (httpRequest.path.starts_with("/echo"))
				{
					httpResponse = { "HTTP/1.1 200 OK\r\n", "Content-Type: TEXT/plain\r\n",
						{ "Content-Length:", httpRequest.body.length()}, httpRequest.body };
					response = httpResponse.GetResponse();

				}
				else if (httpRequest.path == "/user-agent")
				{

					std::string userAgent = httpRequest.headers["User-Agent"];
					response = "HTTP/1.1 200 OK\r\nContent-Type: TEXT/plain\r\nContent-Length:"
						+ std::to_string(userAgent.size()) + "\r\n\r\n" + userAgent;
				}
				else if (httpRequest.path.starts_with("/files/") || httpRequest.path.starts_with("/files"))
				{
					/*
					$ echo -n 'Hello, World!' > /tmp/foo
					$ curl -i http://localhost:4221/files/foo */
					std::string fileName = httpRequest.path.substr(7);
					std::filesystem::path filePath(fileName);
					//判断文件是否有扩展名c++17
					if (filePath.has_extension())
						std::cout << "Ok have extension" << std::endl;
					else
						fileName += ".txt";

					std::string currentPath = std::filesystem::current_path().string();
					std::string tempPath = "/tmp/";
					std::string path = currentPath + tempPath + fileName;
					std::ifstream ifs(path, std::ios::binary);//不希望自动添加换行符
					if (ifs.good())
					{
						std::stringstream content;
						content << ifs.rdbuf();
						httpResponse = { "HTTP/1.1 200 OK\r\n", "Content - Type:application/octet-stream\r\n",
							{"Content-Length:", content.str().length()},content.str() };
						response = httpResponse.GetResponse();
					}
					else
					{
						response = "HTTP/1.1 404 Not Found\r\n";
					}
				}
				else
				{
					response = "HTTP/1.1 404 Not Found\r\n";
				}
			}
			else if (httpRequest.method == "POST")
			{
				//测试

				if (httpRequest.path.starts_with("/files"))
				{
					std::string fileName = httpRequest.path.substr(7);

					std::fstream file(fileName, std::ios::out);
					if (!file.is_open())
					{
						std::cerr << "Failed to open file for writing" << std::endl;
						return false;
					}
					try {
						file << httpRequest.body;

					}
					catch (const std::ios_base::failure& e)
					{
						std::cerr << "写入文件时发生错误：" << e.what() << std::endl;
						return false;
					}
					file.close();

					httpResponse = { "HTTP/1.1 201 Created\r\n\r\n", "Content-Type: application/octet-stream\r\n",
						{"Content-Length:",httpRequest.body.length()}, httpRequest.body };

					response = httpResponse.GetResponse();
				}
			}
			else
			{
				httpResponse = { "HTTP/1.1 405 Method Not Allowed\r\n",
					"Content-Type: TEXT/plain\r\n", {"Content-Length:", 0}, "Method Not Allowed" };
				response = httpResponse.GetResponse();

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

void Server::CloseServer()
{
	if (serverSocket != INVALID_SOCKET)
	{
        closesocket(serverSocket);
		serverSocket = INVALID_SOCKET;
	}
	for (auto& client : clientSockets)
	{
		if (client != INVALID_SOCKET)
			closesocket(client);
		client = INVALID_SOCKET;
	}
	clientSockets.clear();
	WSACleanup();
}
Server::~Server()
{
	CloseServer();
}

