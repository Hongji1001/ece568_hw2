#include "httprequest.hpp"

HttpRequest::HttpRequest(const std::string &rawRequest) : httpRequest(rawRequest)
{
    try
    {
        parseStartLine();
        parseHeaderFields();
        parseHostAndPort();
    }
    catch (const std::exception &e)
    {
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

void HttpRequest::parseStartLine()
{
    size_t requestLineEnd = httpRequest.find("\r\n");
    std::string requestLine = httpRequest.substr(0, requestLineEnd);
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
    if (requestLineParts[0] != "GET" || requestLineParts[0] != "POST" || requestLineParts[0] != "CONNECT")
    {
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
    if (headerMap.count("Host") <= 0)
    {
        throw std::exception();
    }
    std::string Host = headerMap["Host"];
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