#include "Proxy.hpp"
Cache Proxy::cache;
ProxyLog Proxy::proxyLog;


int main()
{
    // if(daemon(1, 0) == -1){
    //     std::cout<<"error\n"<<std::endl;
    //     exit(-1);
    // }
    Proxy *proxy_daemon = nullptr;
    try
    {
        proxy_daemon = new Proxy(12345);
        proxy_daemon->startRun();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    delete proxy_daemon;
    // Proxy::proxyLog.closeLogFile();
    std::cout << "exit" << std::endl;
    return EXIT_SUCCESS;
}
