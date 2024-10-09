#pragma once

#include <string>
#include <vector>
#include <tuple>
class RequestParse {
public:
	//�洢���Ͷ˵���Ϣ
	std::tuple<std::string, std::vector<std::string>> requestMesssage;
	virtual std::vector<std::string> 
		SplitMessage(const std::string& message, const std::string& delim) = 0;

	virtual std::tuple<std::string, std::vector<std::string>> getPath(const std::string& request) = 0;

};