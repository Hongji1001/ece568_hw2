#include "Proxy.hpp"


void Proxy::startRun(){
    // init as server and start listening 
    std::cout << "start Run server " << std::endl;
    Server server = Server(port);
    // check whether proxy server can init successful
    if (server.getErrorSign() == -1){
        // how to exit gracefully return nullptr
        std::cout << "can not init as a server";
        exit(EXIT_FAILURE);
    }
    std::cout << "inited server " << std::endl;
    // start getting request from client
    while(true){
        server.tryAccept();
        if (server.getErrorSign() == -1){
            std::cout << "can not init as a server";
            exit(EXIT_FAILURE);
        }
        std::cout << "has accept" << std::endl;
        // recv msg
        std::string msg = server.recvData(0);
        // string msgString(msg, msg + strlen(msg));
        std::cout << msg << std::endl;
        Request* newRequest = new Request(msg);
        pthread_t thread;
        pthread_create(&thread, NULL, handle, newRequest);
    }
}


void* Proxy::handle(void* newRequest){

    HttpRequest newHttpRequest = HttpRequest(((Request*)newRequest)->getRequestLine());
    // start to verify http request
    std::cout << newHttpRequest.getHost() << std::endl;
    std::cout << newHttpRequest.getMethod() << std::endl;
    std::cout << newHttpRequest.getPort() << std::endl;
    std::cout << newHttpRequest.getRequestTarget() << std::endl;

    // if httprequest has cached

    // did not cache
    std::cout << std::endl;
    std::cout << std::endl;
    // sendMsgToWebserver(newHttpRequest);
    unsigned short webserver_port_num = std::stoul(newHttpRequest.getPort());
    const char* webserver_hostname = newHttpRequest.getHost().c_str();
    std::cout << "webserver_hostname" << std::endl;
    std::cout << webserver_hostname << std::endl;
    Client proxy_own_client = Client(webserver_port_num, webserver_hostname);
    std::cout << std::endl;
    std::cout << std::endl;
    const char* http_raw_text = newHttpRequest.getRawRequestText().c_str();
    std::cout << "newHttpRequest.getRawRequestText().c_str()" << std::endl;
    std::cout << newHttpRequest.getRawRequestText().c_str() << std::endl;
    size_t http_raw_text_size = newHttpRequest.getRawRequestText().size();
    std::cout << std::endl;
    std::cout << std::endl;
    proxy_own_client.sendRequest(http_raw_text, http_raw_text_size);

    std::string webserver_response = proxy_own_client.recvResponse();

    std::cout << webserver_response << std::endl;
    return nullptr;

}


void Proxy::sendMsgToWebserver(HttpRequest newRequest){
    // send request to webserver
    // unsigned short webserver_port_num = std::stoul(newRequest.getPort());
    // const char* webserver_hostname = const_cast<char *>(newHttpRequest.getHost().c_str());
    // Client proxy_own_client = Client(webserver_port_num, webserver_hostname);
    // const char* http_raw_text = const_cast<char *>(newHttpRequest.getRawRequestText().c_str());
    // size_t http_raw_text_size = newHttpRequest.getRawRequestText().size();
    // proxy_own_client.sendRequest(http_raw_text, http_raw_text_size);

    // std::string webserver_response = proxy_own_client.recvResponse();

    // std::cout << webserver_response << std::endl;

}