#include "HttpResponse.hpp"

HttpResponse::HttpResponse(const std::string &rawResponse) : httpResponse(rawResponse)
{
    try
    {
        // TODO: 返回的报文长度可能会有错误
        parseStatusLine();
        parseHeaderFields();
        parseMsgBody();
    }
    catch (const std::exception &e)
    {
        // send 400 bad request response (还没实现)
    }

    // record request and write log
}

std::string HttpResponse::getStatusCode() const
{
    return statusCode;
}

std::string HttpResponse::getReasonPhrase() const
{
    return reasonPhrase;
}

std::string HttpResponse::getMsgBody() const
{
    return msgBody;
}

std::string HttpResponse::getRawResponseText() const
{
    return httpResponse;
}

void HttpResponse::parseStatusLine()
{
    size_t statusLineEnd = httpResponse.find("\r\n"); // TODO: there is no \r\n in request header
    std::string statusLine = httpResponse.substr(0, statusLineEnd);
    std::vector<std::string> statusLineParts;
    size_t pos = 0;
    while (pos != std::string::npos)
    {
        size_t end = statusLine.find(" ", pos);
        if (end == std::string::npos)
        {
            statusLineParts.push_back(statusLine.substr(pos));
            break;
        }
        statusLineParts.push_back(statusLine.substr(pos, end - pos));
        pos = end + 1;
    }
    if (statusLineParts.size() != 3)
    {
        throw std::exception();
    }
    if (statusLineParts[1] >= "600" || statusLineParts[1] < "100")
    {
        // status code is not correct
        throw std::exception();
    }
    httpVersion = statusLineParts[0];
    statusCode = statusLineParts[1];
    reasonPhrase = statusLineParts[2];
}

void HttpResponse::parseHeaderFields()
{
    size_t statusLineEnd = httpResponse.find("\r\n");
    size_t headerEnd = httpResponse.find("\r\n\r\n");
    std::string headers = httpResponse.substr(statusLineEnd + 2, headerEnd - statusLineEnd - 2);
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

void HttpResponse::parseMsgBody()
{
    size_t headerEnd = httpResponse.find("\r\n\r\n");
    msgBody = httpResponse.substr(headerEnd + 4);
}
