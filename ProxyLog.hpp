#include <iostream>
#include <fstream>
#include <string>
#include <pthread.h>
#include "httprequest.hpp"
#include "HttpResponse.hpp"
#include "Request.hpp"

class ProxyLog
{
private:
    std::ofstream logfile;

public:
    void openLogFile(std::string path);
    void writeLogFile(std::string logLine);
    void closeLogFile();
    void writeRequstLogLine(const HttpRequest &newHttpRequest, void *newRequest, const std::string &mode);
    void writeResponseLogLine(const std::string &responseStartLine, void *newRequest, std::string hostname, const std::string &mode);
    void writeReqCacheLogLine(HttpRequest &newHttpRequest, void *newRequest, const std::string &mode, const std::string &response);
    void writeResCacheLogLine(HttpResponse &webResponse, void *newRequest, const std::string &mode);
};