#include "Server.hpp"
#include "client.hpp"
#include "Request.hpp"
#include <pthread.h>
#include "httprequest.hpp"
#include "HttpResponse.hpp"
#include <algorithm>
#include "Cache.hpp"
#define PORT_NUM 12345
#define CACHE_CAPACITY 100

class Proxy
{
private:
    const int port_num;
    Cache cache;

public:
    explicit Proxy(const int port_num) : port_num(port_num), cache(CACHE_CAPACITY){};
    void startRun();
    static void *handle(void *newRequest);
    static void handleCONNECT(HttpRequest newHttpRequest, void *newRequest);
    static void handlePOST(HttpRequest newHttpRequest, void *newRequest);
    static std::string sendMsgToWebserver(HttpRequest newHttpRequest);
    static void sendMsgFromProxy(int sockfd, const char *msg, size_t size);
    void validation(HttpRequest &request, int sockfd);
};