#include "client.hpp"

int main(){
    // browser -> proxy server: test httprequest & recv string
    // proxy server -> google.com: response(string)
    Client client = Client(12345, "vcm-30576.vm.duke.edu");
    // const char* msg = "GET /hello-world HTTP/1.1\r\n"
    //     "Host: www.example.com\r\n"
    //     "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:58.0) Gecko/20100101 Firefox/58.0\r\n"
    //     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    //     "Accept-Language: en-US,en;q=0.5\r\n"
    //     "Connection: keep-alive\r\n\r\n";
    const char* msg = "GET / HTTP/1.1\r\n"
        "Host: www.google.com\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:58.0) Gecko/20100101 Firefox/58.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Connection: keep-alive\r\n\r\n";
    client.sendRequest(msg, strlen(msg));
    return 0;
}