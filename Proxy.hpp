#include "Server.hpp"
#include "client.hpp"
#include "Request.hpp"
#include <pthread.h>
#include "httprequest.hpp"
#include "HttpResponse.hpp"
#define PORT_NUM 12345

class Proxy{
    private:
        const int port_num;

    public:
        explicit Proxy(const int port_num) : port_num(port_num) {};
        void startRun();
        static void* handle(void* newRequest);
        static std::string sendMsgToWebserver(HttpRequest newHttpRequest);
        static void sendMsgBackToClient(int sockfd, const char* msg, size_t size);
};