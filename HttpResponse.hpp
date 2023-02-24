#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <map>
#include <string>
#include <vector>
#include <iostream>

/* danger log */
// 1.请求体可能是有误的，没有考虑request存在问题的情况
// 2.需要分解host的端口号， host中可能会带有端口号而非默认的80

class HttpResponse
{
private:
    std::string httpResponse;
    std::string statusCode;
    std::string statusLine;
    std::string reasonPhrase;
    std::string httpVersion;
    std::string Headers;
    std::string msgBody;
    size_t msgBodySize;
    size_t msgContentLength;
    bool isChunked;
    std::map<std::string, std::string> headerMap;

public:
    HttpResponse(const std::string &rawResponse);
    std::string getStatusCode() const;
    std::string getReasonPhrase() const;
    std::string getMsgBody() const;
    std::string getRawResponseText() const;
    std::string getStartLine() const;
    std::string getHead() const;
    size_t getMaxAge() const;
    size_t getMsgBodySize() const;
    size_t getContentLength() const;
    bool checkIsChunked() const;
    std::map<std::string, std::string> getHeaderMap() const;
    void parseStatusLine();
    void parseHeaderFields();
    void parseMsgBody();
};

// 1.验证方法是否是GET POST CONNECT中的一种, 不是就返回405 method not allowed
// 2.在http 1.1中不能缺失host字段, 如果缺失, 服务器返回400 bad request
