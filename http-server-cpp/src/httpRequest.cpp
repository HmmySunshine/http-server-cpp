
#include "../include/httpRequest.h"

void HttpRequest::InitHttpRequest(const std::string& requset)
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
	std::vector<std::string> bodyTokens = SplitMessage(path, "/");
	// /echo/abc  只需要abc
	//不行body是 除了头后面发的字符串不是这个路径
	//7 0 - 6 0 是路径 1-5是请求头 后面的是body
	if (this->method == "POST")
	{
		for (size_t i = headers.size() + 1; i < tokens.size(); i++)
		{
			if (tokens[i].empty())
				continue;
			body += tokens[i] + "\n";
			std::cout << body << std::endl;
		}
	}
	else if (this->method == "GET")
	{
		//如果没用body我们就把最后一个路径作为body
		body = bodyTokens.back();
	}
}

std::map<std::string, std::string> HttpRequest::getHeaders(const std::vector<std::string>& headerlines)
{
	//目前处理按postman第一个用别的存储了后续的只能处理:
	std::map<std::string, std::string> headers;
	for (size_t i = 1; i < headerlines.size(); i++)
	{
		if (headerlines[i].empty())
		{
			std::cout << "请求头结束" << std::endl;
			break;
		}

		size_t delimPos = headerlines[i].find(": ");
		if (delimPos != std::string::npos)
		{
			std::string key = headerlines[i].substr(0, delimPos);
			key.erase(0, key.find_first_not_of(" \t\r\n"));
			key.erase(key.find_last_not_of(" \t\r\n") + 1);
			//Host: localhost:4221
			std::string value = headerlines[i].substr(delimPos + 2);
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
			/*if (key == "Content-Length")
				break;*/
		}
		else
		{
			std::cerr << "请求头内容不正确 key:value" << std::endl;

		}
	}


	return headers;
}


std::tuple<std::string, std::vector<std::string>> HttpRequest::getPath(const std::string& request) 
{
	std::vector<std::string> tokens = SplitMessage(request, "\r\n");
	std::vector<std::string> paths = SplitMessage(tokens[0], " ");
	//返回的是完整路径比如/echo/abc 
	//一个返回的是字符串数据根据换号符分割的字符串数组
	this->requestMesssage = { paths[1], tokens };
	return requestMesssage;
}
//分割字符串
std::vector<std::string> HttpRequest::SplitMessage(const std::string& message, const std::string& delim) 
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

