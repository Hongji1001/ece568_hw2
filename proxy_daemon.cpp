#include "Proxy.hpp"


int main(){
    // TODO: ADD daemon
    // LOG(INFO)<<"testDaemon start";
    // if(daemon(0, 0) == -1){
    //     std::cout<<"error\n"<<std::endl;
    //     exit(-1);
    // }
    try
    {
        Proxy* proxy_daemon = new Proxy(12345);
        proxy_daemon->startRun();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return EXIT_SUCCESS;

}