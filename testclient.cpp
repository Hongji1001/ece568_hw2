#include "client.hpp"

int main(){

    // browser -> proxy server: test httprequest & recv string
    // proxy server -> google.com: response(string)
    Client client = Client(12345, "vcm-30576.vm.duke.edu");
    char* msg = "GET www.google.com/ HTTP/1.1\r\n";
    msg += "port hostname"
    client.sendRequest(msg, strlen(msg));
    return 0;
}