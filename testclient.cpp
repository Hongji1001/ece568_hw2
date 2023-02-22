#include "Server.hpp"
#include "client.hpp"
#include "Request.hpp"
#include <pthread.h>
#include "httprequest.hpp"

int main(){
    // browser -> proxy server: test httprequest & recv string
    // proxy server -> google.com: response(string)
    // Client client = Client(12345, "vcm-30576.vm.duke.edu");
    // const char* msg = "GET /hello-world HTTP/1.1\r\n"
    //     "Host: www.example.com\r\n"
    //     "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:58.0) Gecko/20100101 Firefox/58.0\r\n"
    //     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    //     "Accept-Language: en-US,en;q=0.5\r\n"
    //     "Connection: keep-alive\r\n\r\n";
    // const char* msg = "GET / HTTP/1.1\r\n"
    //     "Host: www.google.com:\r\n"
    //     "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:58.0) Gecko/20100101 Firefox/58.0\r\n"
    //     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    //     "Accept-Language: en-US,en;q=0.5\r\n"
    //     "Connection: keep-alive\r\n\r\n";

        const char* msg = "GET / HTTP/1.1\r\n"
                "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:58.0) Gecko/20100101 Firefox/58.0\r\n"
                "Host: www.example.com\r\n"
                "Accept-Language: en, mi\r\n\r\n";

    // const char* msg = "CONNECT www.google.com:443 HTTP/1.1\r\n"
    //         "Host: www.google.com\r\n"
    //         "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:58.0) Gecko/20100101 Firefox/58.0\r\n"
    //         "Proxy-Connection: keep-alive\r\n\r\n";
    // const char* msg = "CONNECT www.google.com:443 HTTP/1.1\r\n"
    //         "Host: www.google.com:443\r\n"
    //         "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:58.0) Gecko/20100101 Firefox/58.0\r\n"
    //         "Proxy-Connection: keep-alive\r\n\r\n";

    // client.sendRequest(msg, strlen(msg));
    Request* newRequest = new Request(msg);
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
    std::cout << http_raw_text_size << std::endl;
    std::cout << strlen(msg) << std::endl;
    proxy_own_client.sendRequest(newHttpRequest.getRawRequestText().c_str(), http_raw_text_size);
    std::string webserver_response = proxy_own_client.recvResponse();
    std::cout << webserver_response << std::endl;
    // proxy_own_client.sendRequest(msg, strlen(msg));
    // webserver_response = proxy_own_client.recvResponse();

    // std::cout << webserver_response << std::endl;
    return 0;
}