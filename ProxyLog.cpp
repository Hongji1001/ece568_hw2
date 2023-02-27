#include "ProxyLog.hpp"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void ProxyLog::openLogFile(std::string path){
    if(!logfile.is_open()){
        logfile.open(path, std::ios::out | std::ios::trunc);
    }
}

void ProxyLog::writeLogFile(std::string logLine){
    pthread_mutex_lock(&lock);
    logfile << logLine << std::endl;
    pthread_mutex_unlock(&lock);
}

void ProxyLog::closeLogFile(){
    if (logfile.is_open()){
        logfile.close();
    }
}


void ProxyLog::writeRequstLogLine(const HttpRequest &newHttpRequest, void *newRequest, const std::string& mode)
{
    std::string requestID = std::to_string(((Request *)newRequest)->getRequestID());
    std::string requestStartLine = newHttpRequest.getRequestLine();
    if (mode == "from_browser_to_proxy")
    {
        std::string requestIP = ((Request *)newRequest)->getClientIP();
        std::string requestTime = Time::getLocalUTC();
        requestTime.erase(std::remove(requestTime.begin(), requestTime.end(), '\n'),
                          requestTime.end()); // trim \n
        writeLogFile(requestID + ": " + requestStartLine + " from " + requestIP + " @ " + requestTime);
        return;
    }
    if (mode == "from_proxy_to_webserver")
    {
        std::string requestHost = newHttpRequest.getHost();
        writeLogFile(requestID + ": " + "Requesting " + requestStartLine + " from " + requestHost);
        return;
    }
}

void ProxyLog::writeResponseLogLine(const std::string &responseStartLine, void* newRequest, std::string hostname, const std::string& mode){
    // create a response back to browser
    std::string requestID = std::to_string(((Request*)newRequest)->getRequestID());
    if (mode == "from_webserver_to_browser"){
        writeLogFile(requestID + ": " + "Received " + responseStartLine + " from " + hostname); 
        return;  
    }
    if (mode == "from_proxy_to_browser"){
        writeLogFile(requestID + ": " + "Responding " + responseStartLine);
        return;
    }
}
