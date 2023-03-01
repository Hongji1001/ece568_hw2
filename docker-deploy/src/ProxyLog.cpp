#include "ProxyLog.hpp"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void ProxyLog::openLogFile(std::string path)
{
    if (!logfile.is_open())
    {
        logfile.open(path, std::ios::out | std::ios::trunc);
    }
}

void ProxyLog::writeLogFile(std::string logLine)
{
    pthread_mutex_lock(&lock);
    logfile << logLine << std::endl;
    pthread_mutex_unlock(&lock);
}

void ProxyLog::closeLogFile()
{
    if (logfile.is_open())
    {
        logfile.close();
    }
}

void ProxyLog::writeRequstLogLine(const HttpRequest &newHttpRequest, void *newRequest, const std::string &mode)
{
    std::string requestID = std::to_string(((Request *)newRequest)->getRequestID());
    std::string requestStartLine = newHttpRequest.getRequestLine();
    if (mode == "from_browser_to_proxy")
    {
        std::string requestIP = ((Request *)newRequest)->getClientIP();
        std::string requestTime = Time::getLocalUTC();
        requestTime.erase(std::remove(requestTime.begin(), requestTime.end(), '\n'),
                          requestTime.end()); // trim \n
        writeLogFile(requestID + ": \"" + requestStartLine + "\" from " + requestIP + " @ " + requestTime);
        return;
    }
    if (mode == "from_proxy_to_webserver")
    {
        std::string requestHost = newHttpRequest.getHost();
        writeLogFile(requestID + ": " + "Requesting \"" + requestStartLine + "\" from " + requestHost);
        return;
    }
}

void ProxyLog::writeResponseLogLine(const std::string &responseStartLine, void *newRequest, std::string hostname, const std::string &mode)
{
    // create a response back to browser
    std::string requestID = std::to_string(((Request *)newRequest)->getRequestID());
    if (mode == "from_webserver_to_browser")
    {
        writeLogFile(requestID + ": " + "Received \"" + responseStartLine + "\" from " + hostname);
        return;
    }
    if (mode == "from_proxy_to_browser")
    {
        writeLogFile(requestID + ": " + "Responding \"" + responseStartLine + "\"");
        return;
    }
}

void ProxyLog::writeReqCacheLogLine(HttpRequest &newHttpRequest, void *newRequest, const std::string &mode, const std::string &response)
{
    std::string requestID = std::to_string(((Request *)newRequest)->getRequestID());
    if (mode == "not in cache")
    {
        writeLogFile(requestID + ": not in cache");
        return;
    }
    if (mode == "expired")
    {
        HttpResponse temp(response);
        std::string EXPIREDTIME;
        if (temp.getHeaderMap().count("expires"))
        {
            EXPIREDTIME = Time::gmtToUTC(temp.getHeaderMap()["expires"]);
            EXPIREDTIME.erase(std::remove(EXPIREDTIME.begin(), EXPIREDTIME.end(), '\n'),
                              EXPIREDTIME.end());
        }
        else
        {
            std::string Date = temp.getHeaderMap()["date"];
            size_t maxAge = temp.getMaxAge();
            struct tm gmt_struct = {0};
            strptime(Date.c_str(), "%a, %d %b %Y %T %Z", &gmt_struct);
            time_t gmt_time = mktime(&gmt_struct);
            std::tm *utc_time = std::gmtime(&gmt_time);
            std::time_t time2 = std::mktime(utc_time);
            std::time_t offset = time2 - gmt_time;           
            gmt_time = gmt_time - offset + maxAge;
            struct tm *utc_struct = gmtime(&gmt_time);
            std::string UTCTime(asctime(utc_struct));
            EXPIREDTIME = UTCTime;
            EXPIREDTIME.erase(std::remove(EXPIREDTIME.begin(), EXPIREDTIME.end(), '\n'),
                              EXPIREDTIME.end());
        }
        writeLogFile(requestID + ": in cache, but expired at " + EXPIREDTIME);
        return;
    }
    if (mode == "requires validation")
    {
        writeLogFile(requestID + ": in cache, requires validation");
        return;
    }
    if (mode == "valid")
    {
        writeLogFile(requestID + ": in cache, valid");
        return;
    }
}

void ProxyLog::writeResCacheLogLine(HttpResponse &webResponse, void *newRequest, const std::string &mode)
{
    if (webResponse.getStatusCode() != "200")
    {
        return;
    }
    std::string requestID = std::to_string(((Request *)newRequest)->getRequestID());
    if (mode == "not cacheable")
    {
        writeLogFile(requestID + ": not cacheable beacause response is private and/or no-store and/or chunked data");
        return;
    }
    if (mode == "cached expired")
    {
        std::string EXPIREDTIME;
        if (webResponse.getHeaderMap().count("expires"))
        {
            EXPIREDTIME = Time::gmtToUTC(webResponse.getHeaderMap()["expires"]);
            EXPIREDTIME.erase(std::remove(EXPIREDTIME.begin(), EXPIREDTIME.end(), '\n'),
                              EXPIREDTIME.end());
        }
        else
        {
            std::string Date = webResponse.getHeaderMap()["date"];
            size_t maxAge = webResponse.getMaxAge();
            struct tm gmt_struct = {0};
            strptime(Date.c_str(), "%a, %d %b %Y %T %Z", &gmt_struct);
            time_t gmt_time = mktime(&gmt_struct);
            std::tm *utc_time = std::gmtime(&gmt_time);
            std::time_t time2 = std::mktime(utc_time);
            std::time_t offset = time2 - gmt_time;           
            gmt_time = gmt_time - offset + maxAge;
            struct tm *utc_struct = gmtime(&gmt_time);
            std::string UTCTime(asctime(utc_struct));
            EXPIREDTIME = UTCTime;
            EXPIREDTIME.erase(std::remove(EXPIREDTIME.begin(), EXPIREDTIME.end(), '\n'),
                              EXPIREDTIME.end());
        }
        writeLogFile(requestID + ": cached, expires at " + EXPIREDTIME);
        return;
    }
    if (mode == "re-validation")
    {
        writeLogFile(requestID + ": cached, but requires re-validation");
        return;
    }
}
