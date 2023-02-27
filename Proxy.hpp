#include "Server.hpp"
#include "client.hpp"
#include "Request.hpp"
#include <pthread.h>
// #include "httprequest.hpp"
// #include "HttpResponse.hpp"
#include "ProxyLog.hpp"
#include "Cache.hpp"
#define PORT_NUM 12345

class Proxy
{
private:
    const int port_num;
    static Cache cache;
    static ProxyLog proxyLog;

public:
    explicit Proxy(const int port_num) : port_num(port_num){};
    void startRun();
    static void *handle(void *newRequest);
    static void handleCONNECT(HttpRequest& newHttpRequest, void *newRequest);
    static void handlePOST(HttpRequest& newHttpRequest, void *newRequest);
    static void handleGET(HttpRequest& newHttpRequest, void *newRequest);
    static void conditionalReq(HttpRequest& newHttpRequest, void *newRequest);
    static void nonConditionalReq(HttpRequest& newHttpRequest, void *newRequest);
    static std::string recvAllData(Client &client, std::string server_meg, size_t contentLength, size_t msgBodySize);
    static HttpResponse sendMsgToWebserver(HttpRequest& newHttpRequest, void *newRequest);
    static void sendMsgFromProxy(int sockfd, const char *msg, size_t size);
    static std::string getRequstLogLine(const HttpRequest& newHttpRequest, void* newRequest, const std::string& mode);
    static std::string getResponseLogLine(const std::string& responseStartLine, void* newRequest, std::string hostname, const std::string& mode);
    static void sendFormedHttpResponse(HttpResponse& formed_response, HttpRequest& newHttpRequest, void *newRequest);
    static HttpResponse getFormedHttpResponse(const int status_code);
    
};
