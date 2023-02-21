#include "Proxy.hpp"

using namespace std;


Proxy::Proxy(int port_num) : port(port_num) {}


void Proxy::startRun(){
    // init as server and start listening 
    Server server = Server(port);
    // check whether proxy server can init successful
    if (server.getErrorSign() == -1){
        // how to exit gracefully return nullptr
        cout << "can not init as a server";
        exit(EXIT_FAILURE);
    }
    // start getting request from client
    while(true){
        server.tryAccept();
        if (server.getErrorSign() == -1){
            cout << "can not init as a server";
            exit(EXIT_FAILURE);
        }
        
        // recv msg
        char* msg = server.recvData(0);
        Request* newRequest = new Request(msg);
        pthread_t thread;
        pthread_create(&thread, NULL, handle, newRequest);
    }
}


void* Proxy::handle(Request* newRequest){
    size_t msgLen = strlen(newRequest->raw_request_line);
    string rawMsgContent(newRequest->raw_request_line, newRequest->raw_request_line + msgLen);

    HttpRequest newHttpRequest = HttpRequest(rawMsgContent);
    // start to verify http request
    cout << newHttpRequest.getHost() << endl;
    cout << newHttpRequest.getMethod() << endl;
    cout << newHttpRequest.getPort() << endl;
    cout << newHttpRequest.getRequestTarget() << endl;

}