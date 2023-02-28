#include "Proxy.hpp"
Cache Proxy::cache;
ProxyLog Proxy::proxyLog;


int main()
{
    // if(daemon(0, 0) == -1){
    //     std::cout<<"error\n"<<std::endl;
    //     exit(-1);
    // }
    try
    {
        Proxy *proxy_daemon = new Proxy(12345);
        proxy_daemon->startRun();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    std::cout << "exit" << std::endl;
    return EXIT_SUCCESS;
}
