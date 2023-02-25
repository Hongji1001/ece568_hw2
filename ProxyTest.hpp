#include "Server.hpp"
#include "client.hpp"
#include "Request.hpp"
#include <pthread.h>
// #include "httprequest.hpp"
// #include "HttpResponse.hpp"
#include <algorithm>
#include "Cache.hpp"
#define PORT_NUM 12345

class Proxy
{
private:
    const int port_num;
    static Cache cache;

public:
    explicit Proxy(const int port_num) : port_num(port_num){};
    void startRun();
    static void *handle(void *newRequest);
    static void handleCONNECT(HttpRequest newHttpRequest, void *newRequest);
    static void handlePOST(HttpRequest newHttpRequest, void *newRequest);
    static void handleGET(HttpRequest newHttpRequest, void *newRequest);
    static void Proxy::conditionalReq(HttpRequest newHttpRequest, void *newRequest);
    static void Proxy::nonConditionalReq(HttpRequest newHttpRequest, void *newRequest);
    static HttpResponse sendMsgToWebserver(HttpRequest newHttpRequest, void *newRequest);
    static void sendMsgFromProxy(int sockfd, const char *msg, size_t size);
};
