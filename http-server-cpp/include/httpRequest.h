#pragma once
#include "requestparse.h"
#include <sstream>
#include <map>
#include <iostream>
struct HttpRequest : public RequestParse {
public:
	std::string method;
	std::string path;
	std::string version;
	std::map<std::string, std::string> headers;
	std::string body;
public:
	void InitHttpRequest(const std::string& requset);
	
private:
	std::map<std::string, std::string> getHeaders(const std::vector<std::string>& headerlines);
	std::tuple<std::string, std::vector<std::string>> getPath(const std::string& request) override;
	//·Ö¸î×Ö·û´®
	std::vector<std::string> SplitMessage(const std::string& message, const std::string& delim) override;
	

};
