#include "Proxy.hpp"
Cache Proxy::cache;
ProxyLog Proxy::proxyLog;


int main()
{
    // if(daemon(0, 0) == -1){
    //     std::cout<<"error\n"<<std::endl;
    //     exit(-1);
    // }
    Proxy *proxy_daemon = new Proxy(12345);
    // Proxy proxy_daemon = Proxy(12345);
    proxy_daemon->startRun();
    std::cout << "exit" << std::endl;
    return EXIT_SUCCESS;
}
