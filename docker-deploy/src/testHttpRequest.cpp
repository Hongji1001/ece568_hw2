#include "httprequest.hpp"
#include <cstdio>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <cstring>
#include <string>
#include <stdlib.h>
#include <string.h>

int main(){
    // browser -> proxy server: test httprequest & recv string
    // proxy server -> google.com: response(string)
    // Client client = Client(12345, "vcm-30576.vm.duke.edu");
    std::vector<std::string> test;
    test.push_back("GET");
    if (test[0] == "GET") std::cout << "success" << std::endl;
    
    const char* msg = "GET /hello-world HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:58.0) Gecko/20100101 Firefox/58.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Connection: keep-alive\r\n\r\n";
    
    std::string request_test = std::string(msg, strlen(msg));
    HttpRequest newHttpRequest = HttpRequest(request_test);
    // start to verify http request
    std::cout << newHttpRequest.getHost() << std::endl;
    std::cout << newHttpRequest.getMethod() << std::endl;
    std::cout << newHttpRequest.getPort() << std::endl;
    std::cout << newHttpRequest.getRequestTarget() << std::endl;
    return 0;
}