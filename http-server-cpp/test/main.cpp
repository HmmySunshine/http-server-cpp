#include <iostream>

#include "../include/server.h"
#pragma comment(lib,"ws2_32.lib")


int main(int argc, char* argv[])
{
	//Ë¢ÐÂ»º³åÇø
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;
	

	Server server;
	if (!server.InitServer(4221))
	{
		return 1;
	}

	if (!server.StartServer());
	{
		return 1;
	}

	
	
	

    
	return 0;
}