#include "client.hpp"

int main(){
    Client client = Client(12345, "vcm-30576.vm.duke.edu");
    
    char* msg = "GET www.google.com/ HTTP/1.1";
    
    client.sendRequest(msg, strlen(msg));
    return 0;

}