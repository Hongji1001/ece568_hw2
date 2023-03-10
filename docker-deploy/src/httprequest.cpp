#include "httprequest.hpp"

HttpRequest::HttpRequest(const std::string &rawRequest) : httpRequest(rawRequest)
{
    requestTime = Time::getLocalUTC();
    hasError = false;
    try
    {
        // TODO: 如果解析报文格式不正确应该在哪里处理，是proxy还是webserver
        // verifyBasicFormat();
        parseStartLine();
        parseHeaderFields();
        parseHostAndPort();
    }
    catch (const std::exception &e)
    {
        hasError = true;
        std::cout << "Malformed Request" << std::endl;
        // send 400 bad request response (还没实现)
    }

    // record request and write log
}

std::string HttpRequest::getMethod() const
{
    return method;
}

std::string HttpRequest::getRequestTarget() const
{
    return requestTarget;
}

std::string HttpRequest::getPort() const
{
    return port;
}

std::string HttpRequest::getHost() const
{
    return host;
}

std::string HttpRequest::getRawRequestText() const
{
    return httpRequest;
}

std::string HttpRequest::getRequestTime() const
{
    return requestTime;
}

std::string HttpRequest::getRequestLine() const{
    return requestLine;
}

std::map<std::string, std::string> HttpRequest::getHeaderMap() const
{
    return headerMap;
}

bool HttpRequest::getHasError() const
{
    return hasError;
}


void HttpRequest::verifyBasicFormat()
{
    std::regex pattern("(^([A-Z]+) (\\S*) HTTP/(\\d)\\.(\\d)\\r\\n(.*: .*\\r\\n)*\\r\\n(.*))");
    // if not match basic format
    if (!std::regex_match(httpRequest, pattern))
    {
        throw std::exception();
    }
}

void HttpRequest::parseStartLine()
{
    size_t requestLineEnd = httpRequest.find("\r\n"); // TODO: there is no \r\n in request header
    requestLine = httpRequest.substr(0, requestLineEnd);
    std::vector<std::string> requestLineParts;
    size_t pos = 0;
    while (pos != std::string::npos)
    {
        size_t end = requestLine.find(" ", pos);
        if (end == std::string::npos)
        {
            requestLineParts.push_back(requestLine.substr(pos));
            break;
        }
        requestLineParts.push_back(requestLine.substr(pos, end - pos));
        pos = end + 1;
    }
    if (requestLineParts.size() != 3)
    {
        throw std::exception();
    }
    if (!(requestLineParts[0] == "GET" || requestLineParts[0] == "POST" || requestLineParts[0] == "CONNECT"))
    {
        // method is not correct
        throw std::exception();
    }
    method = requestLineParts[0];
    requestTarget = requestLineParts[1];
    httpVersion = requestLineParts[2];
}

void HttpRequest::parseHeaderFields()
{
    size_t requestLineEnd = httpRequest.find("\r\n");
    size_t headerEnd = httpRequest.find("\r\n\r\n");
    std::string headers = httpRequest.substr(requestLineEnd + 2, headerEnd - requestLineEnd - 2);
    size_t pos = 0;
    while (true)
    {
        size_t end = headers.find("\r\n", pos);
        std::string line = headers.substr(pos, end - pos);
        pos = end + 2;

        size_t colon_pos = line.find(":");
        if (colon_pos != std::string::npos)
        {
            std::string key = line.substr(0, colon_pos);
            std::transform(key.begin(), key.end(), key.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });
            std::string value = line.substr(colon_pos + 2); // +2 to skip ": "
            headerMap[key] = value;
        }
        if (end == std::string::npos)
        {
            break;
        }
    }
}

void HttpRequest::parseHostAndPort()
{
    if (headerMap.count("host") <= 0)
    {
        throw std::exception();
    }
    std::string Host = headerMap["host"];
    size_t pos = Host.find(":");
    if (pos != std::string::npos)
    {
        host = Host.substr(0, pos);
        port = Host.substr(pos + 1);
        if (port.empty())
        {
            port = "80";
        }
    }
    else
    {
        host = Host;
        port = "80";
    }
    // 如果Host为空按理说是不需要处理的
}

// 需要判断好请求中是否没有If-None-Match头
void HttpRequest::buildConRequest(const std::string &ETag, const std::string &LastModified)
{
    size_t headerEnd = httpRequest.find("\r\n\r\n");
    std::string head = httpRequest.substr(0, headerEnd + 2);
    std::string msgBody = httpRequest.substr(headerEnd + 4);
    if (ETag.size() != 0)
    {
        head += "If-None-Match: " + ETag + "\r\n";
        headerMap["if-none-match"] = ETag;
    }

    if (LastModified.size() != 0)
    {
        head += "If-Modified-Since: " + LastModified + "\r\n";
        headerMap["if-modified-since"] = LastModified;
    }
    httpRequest = head + "\r\n" + msgBody;
}