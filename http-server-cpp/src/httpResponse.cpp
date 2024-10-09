#include "../include/httpResponse.h"


HttpResponse::HttpResponse(std::string status, std::string contentType, std::tuple<std::string, size_t> contentLength, std::string body)
	: status(std::move(status)), contentType(std::move(contentType)), contentLength(std::move(contentLength)), body(std::move(body))
{
	// 确保在构造函数中就完成初始化
	auto [ctlString, len] = this->contentLength;
	if (len == 0) {
		std::cerr << "Warning: Content-Length is 0!" << std::endl;
	}
}

std::string HttpResponse::GetResponse()
{
	auto [ctlString, contentLength] = getContentLength();
	return status + contentType + ctlString + std::to_string(contentLength) + "\r\n\r\n" + body;

}

std::tuple<std::string, size_t> HttpResponse::getContentLength()
{
	return contentLength;
}