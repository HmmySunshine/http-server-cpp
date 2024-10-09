#pragma once
#include <string>
#include <iostream>

struct HttpResponse {
public:
	std::string status;
	std::string contentType;
	std::tuple<std::string, size_t> contentLength;
	std::string body;
public:
	HttpResponse(std::string status, std::string contentType, std::tuple<std::string, size_t> contentLength, std::string body);
		

public:
	std::string GetResponse();
	
private:
	std::tuple<std::string, size_t> getContentLength();
	

};