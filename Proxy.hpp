#include "Server.hpp"
#include "client.hpp"
#include "Request.hpp"
#include <pthread.h>
#include "httprequest.hpp"


class Proxy{
    private:
        int port;

    public:
        explicit Proxy(int port_num) : port(port_num) {};
        void startRun();
        static void* handle(void* newRequest);
        void sendMsgToWebserver(HttpRequest newRequest);
};