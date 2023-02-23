#ifndef __HTTPREQUEST_HPP__
#define __HTTPREQUEST_HPP__
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <regex>

/* danger log */
// 1.请求体可能是有误的，没有考虑request存在问题的情况
// 2.需要分解host的端口号， host中可能会带有端口号而非默认的80

class HttpRequest
{
private:
    std::string httpRequest;
    std::string method;
    std::string requestTarget;
    std::string httpVersion;
    std::string port;
    std::string host;
    std::map<std::string, std::string> headerMap;

public:
    HttpRequest(const std::string &rawRequest);
    std::string getMethod() const;
    std::string getRequestTarget() const;
    std::string getPort() const;
    std::string getHost() const;
    std::string getRawRequestText() const;
    void verifyBasicFormat();
    void parseStartLine();
    void parseHeaderFields();
    void parseHostAndPort();
    std::string buildConRequest(std::string &Etag);
};

// 1.验证方法是否是GET POST CONNECT中的一种, 不是就返回405 method not allowed
// 2.在http 1.1中不能缺失host字段, 如果缺失, 服务器返回400 bad request

#endif