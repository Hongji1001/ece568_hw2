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

int next_request_id = 0;


class Request{
    private:
        char* raw_request_line;
        
        int request_id;

    public:
        explicit Request(char* msg);
}