#include "Proxy.hpp"


void Proxy::startRun(){
    // init as server and start listening 
    std::cout << "start Run server " << std::endl;
    Server server = Server(port_num);
    // check whether proxy server can init successful
    if (server.getErrorSign() == -1){
        // how to exit gracefully return nullptr
        std::cout << "can not init as a server";
        exit(EXIT_FAILURE);
    }
    std::cout << "inited server " << std::endl;
    // start getting request from client
    while(true){
        int client_connection_fd = server.tryAccept();
        if (client_connection_fd == -1){
            std::cout << "fail to accept" << std::endl;
            exit(EXIT_FAILURE);
        }
        if (server.getErrorSign() == -1){
            std::cout << "can not init as a server";
            exit(EXIT_FAILURE);
        }
        std::cout << "has accept" << std::endl;
        // recv msg
        std::string msg = server.recvData(0);
        // string msgString(msg, msg + strlen(msg));
        std::cout << msg << std::endl;
        Request* newRequest = new Request(msg, client_connection_fd);
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
    std::string web_response = sendMsgToWebserver(newHttpRequest);
    sendMsgBackToClient(((Request*)newRequest)->getClientFd(), web_response.c_str(), web_response.size());
    return nullptr;
}


std::string Proxy::sendMsgToWebserver(HttpRequest newHttpRequest){
    // send request to webserver
    // get port and hostname of webserver
    unsigned short webserver_port_num = std::stoul(newHttpRequest.getPort());
    // const char* webserver_hostname = newHttpRequest.getHost().c_str();
    std::cout << "webserver_port_num" << std::endl;
    std::cout << webserver_port_num << std::endl;
    std::cout << "newHttpRequest.getHost()" << std::endl;
    std::cout << newHttpRequest.getHost() << std::endl;
    Client proxy_own_client = Client(webserver_port_num, newHttpRequest.getHost());
    // send msg to webserver
    size_t http_raw_text_size = newHttpRequest.getRawRequestText().size();
    proxy_own_client.sendRequest(newHttpRequest.getRawRequestText().c_str(), http_raw_text_size);

    // recv response from webserver
    std::string webserver_response = proxy_own_client.recvResponse();
    std::cout << webserver_response << std::endl;

    std::cout << std::endl;
    std::cout << std::endl;

    HttpResponse recvHttpResponse = HttpResponse(webserver_response);
    std::cout << recvHttpResponse.getStatusCode() << std::endl;
    std::cout << recvHttpResponse.getReasonPhrase() << std::endl;
    std::cout << recvHttpResponse.getMsgBody() << std::endl;
    return recvHttpResponse.getRawResponseText();
}


void Proxy::sendMsgBackToClient(int sockfd, const char* msg, size_t size){
    int numBytes = 0;
    int recvBytes = 0;
    while ((numBytes < size))
    {
        if ((recvBytes = send(sockfd, msg, size, 0)) == -1)
        {
            perror("client send");
            throw std::exception();
        }
        numBytes += recvBytes;
    }
}