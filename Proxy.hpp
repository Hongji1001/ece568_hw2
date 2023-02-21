#include "Server.hpp"
#include "Request.hpp"
#include <pthread.h>
#include "httprequest.hpp"


class Proxy{
    private:
        int port;

    public:
        explicit Proxy(int port_num);
        void startRun();
        void* handle(char* msg);


};