#pragma once
#include <winsock2.h>
#include <iostream>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <WS2tcpip.h>
#include <filesystem>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
class Server {
private:
	SOCKET serverSocket;
	char buffer[1024]{};
	//std::string buffer(1024, '\0');
	unsigned int connectionBackLog = 10;
	std::vector<SOCKET> clientSockets;
	bool CoreFunc();
	bool ConductMsg(SOCKET clientFd);
public:
	Server();
	//ip暂时没用
	bool InitServer(int port);
	//启动
    bool StartServer();
	
	void CloseServer();
	~Server();
};
