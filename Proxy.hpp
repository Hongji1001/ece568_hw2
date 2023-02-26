#include "Server.hpp"
#include "client.hpp"
#include "Request.hpp"
#include "httprequest.hpp"
#include "HttpResponse.hpp"
#include "ProxyLog.hpp"
#include <algorithm>

#define PORT_NUM 12345

class Proxy{
    private:
        const int port_num;
        static ProxyLog proxyLog;

    public:
        explicit Proxy(const int port_num) : port_num(port_num) {};
        void startRun();
        static void* handle(void* newRequest);
        static void handleCONNECT(HttpRequest newHttpRequest, void* newRequest);
        static void handlePOST(HttpRequest newHttpRequest, void* newRequest);
        static void handleGET(HttpRequest newHttpRequest, void* newRequest);
        static HttpResponse sendMsgToWebserver(HttpRequest newHttpRequest, void* newRequest, Client& proxy_own_client);
        static void sendMsgFromProxy(int sockfd, const char* msg, size_t size);
        static std::string recvAllData(Client& client, std::string server_meg, size_t contentLength, size_t msgBodySize);
        static std::string getRequstLogLine(HttpRequest newHttpRequest, void* newRequest, std::string mode);
        static std::string getResponseLogLine(HttpResponse webserver_response, void* newRequest, std::string hostname);
};