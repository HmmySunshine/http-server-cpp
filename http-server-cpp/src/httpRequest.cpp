
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
	//��һ��Ԫ����get ·�� Ȼ��Э�����Բ���Ҫ
	this->headers = std::move(getHeaders(tokens));
	std::vector<std::string> bodyTokens = SplitMessage(path, "/");
	// /echo/abc  ֻ��Ҫabc
	//����body�� ����ͷ���淢���ַ����������·��
	//7 0 - 6 0 ��·�� 1-5������ͷ �������body
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
		//���û��body���ǾͰ����һ��·����Ϊbody
		body = bodyTokens.back();
	}
}

std::map<std::string, std::string> HttpRequest::getHeaders(const std::vector<std::string>& headerlines)
{
	//Ŀǰ����postman��һ���ñ�Ĵ洢�˺�����ֻ�ܴ���:
	std::map<std::string, std::string> headers;
	for (size_t i = 1; i < headerlines.size(); i++)
	{
		if (headerlines[i].empty())
		{
			std::cout << "����ͷ����" << std::endl;
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
			std::cerr << "����ͷ���ݲ���ȷ key:value" << std::endl;

		}
	}


	return headers;
}


std::tuple<std::string, std::vector<std::string>> HttpRequest::getPath(const std::string& request) 
{
	std::vector<std::string> tokens = SplitMessage(request, "\r\n");
	std::vector<std::string> paths = SplitMessage(tokens[0], " ");
	//���ص�������·������/echo/abc 
	//һ�����ص����ַ������ݸ��ݻ��ŷ��ָ���ַ�������
	this->requestMesssage = { paths[1], tokens };
	return requestMesssage;
}
//�ָ��ַ���
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
	//Ѱ�Ҹ��õĴ���ʽ
	if (message.starts_with("Host"))
	{
		tokens.emplace_back("Host");
		tokens.emplace_back(message.substr(6));
		return tokens;
	}
	while (std::getline(ss, line, *delim.begin()))
	{
		tokens.push_back(line);
		//"\r\n" ����\�ͺ��Ժ���3��������
		//delim.length() - 1 �������� size_t��size_t �������޷����������� std::streamsize ���з���������
		//�� size_t ���͵�ֵ��ֵ�� std::streamsize ���͵ı���ʱ���ᴥ������ת�����������

		ss.ignore(static_cast<std::streamsize>(delim.length() - 1));

	}
	return tokens;
}

