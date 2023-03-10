#include "HttpResponse.hpp"

HttpResponse::HttpResponse(const std::string &rawResponse) : httpResponse(rawResponse)
{
    try
    {
        // TODO: 返回的报文长度可能会有错误
        // std::cout << "parseStatusLine" << std::endl;
        isChunked = false;
        msgContentLength = 0;
        msgBodySize = 0;
        parseStatusLine();
        // std::cout << "parseHeaderFields" << std::endl;
        parseHeaderFields();
        // std::cout << "parseMsgBody" << std::endl;
        parseMsgBody();
        // std::cout << "finished parse" << std::endl;
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

std::string HttpResponse::getStartLine() const
{
    return statusLine;
}

std::string HttpResponse::getHttpVersion() const
{
    return httpVersion;
}

std::string HttpResponse::getHead() const
{
    return Headers;
}

size_t HttpResponse::getMaxAge() const
{
    size_t default_max_age = 600;
    // std::string cache_control_key("Cache-Control");
    std::map<std::string, std::string>::const_iterator it = headerMap.find("Cache-Control");
    if (it == headerMap.end())
    {
        return default_max_age;
    }
    std::string cache_control_line = it->second;
    size_t pos = 0;
    std::string max_age_string("max-age");
    pos = cache_control_line.find(max_age_string);
    if (pos == std::string::npos)
    {
        return default_max_age;
    }
    pos = cache_control_line.find("=", pos + max_age_string.size());
    size_t digit_start = pos;
    if (pos == std::string::npos)
    {
        return default_max_age;
    }
    pos = cache_control_line.find(",", pos);
    if (pos == std::string::npos)
    {
        default_max_age = std::stoul(cache_control_line.substr(digit_start + 1));
        return default_max_age;
    }
    else
    {
        default_max_age = std::stoul(cache_control_line.substr(digit_start + 1, pos - digit_start - 1));
        return default_max_age;
    }
}

bool HttpResponse::checkIsChunked() const
{
    return isChunked;
}

size_t HttpResponse::getMsgBodySize() const
{
    return msgBodySize;
}

size_t HttpResponse::getContentLength() const
{
    return msgContentLength;
}

std::map<std::string, std::string> HttpResponse::getHeaderMap() const
{
    return headerMap;
}

void HttpResponse::parseStatusLine()
{
    size_t statusLineEnd = httpResponse.find("\r\n"); // TODO: there is no \r\n in request header
    statusLine = httpResponse.substr(0, statusLineEnd);
    std::vector<std::string> statusLineParts;
    size_t pos = 0;
    while (pos != std::string::npos)
    {
        size_t end = statusLine.find(" ", pos);
        if (statusLineParts.size() == 2)
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
    Headers = httpResponse.substr(statusLineEnd + 2, headerEnd - statusLineEnd);
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
            if (key == "transfer-encoding" && value == "chunked")
            {
                isChunked = true;
            }
            if (key == "content-length")
            {
                isChunked = false;
                msgContentLength = std::stoul(value);
            }
        }
        if (end == std::string::npos)
        {
            break;
        }
    }
}

void HttpResponse::parseMsgBody()
{
    // if (isChunked){
    //     size_t headerEnd = httpResponse.find("\r\n\r\n");
    //     std::string chunked_data = httpResponse.substr(headerEnd + 4);
    //     size_t pos = 0;
    //     while (true){
    //         size_t chunked_size_end = chunked_data.find("\r\n", pos);
    //         std::string size_HEXDIG = chunked_data.substr(pos, chunked_size_end - pos);
    //         size_t size_DEC = std::stoul(size_HEXDIG, nullptr, 16);
    //         if (size_DEC == 0){
    //             break;
    //         }
    //         msgBody += chunked_data.substr(chunked_size_end + 2, size_DEC);
    //         pos = chunked_size_end + 2 + size_DEC;
    //     }
    // } else{
    // 这些地方解析可能问题，就是数据返回没有\r\n的时候
    size_t headerEnd = httpResponse.find("\r\n\r\n");
    msgBody = httpResponse.substr(headerEnd + 4);
    msgBodySize = msgBody.size();
    // }
}
