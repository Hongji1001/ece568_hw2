#include "Server.hpp"
#include "client.hpp"
#include "Request.hpp"
#include <pthread.h>
#include "httprequest.hpp"
#include "HttpResponse.hpp"

int main(){
    // browser -> proxy server: test httprequest & recv string
    // proxy server -> google.com: response(string)

    const char* msg = "GET / HTTP/1.1\r\n"
            "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:58.0) Gecko/20100101 Firefox/58.0\r\n"
            "Host: www.example.com\r\n"
            "Accept-Language: en, mi\r\n\r\n";

    Request* newRequest = new Request(msg);
    HttpRequest newHttpRequest = HttpRequest(((Request*)newRequest)->getRequestLine());
    unsigned short webserver_port_num = std::stoul(newHttpRequest.getPort());
    const char* webserver_hostname = newHttpRequest.getHost().c_str();
    Client proxy_own_client = Client(webserver_port_num, webserver_hostname);
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

    // std::cout << webserver_response << std::endl;
    return 0;
}