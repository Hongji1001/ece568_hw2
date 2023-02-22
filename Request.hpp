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

static int next_request_id = 0;


class Request{
    private:
        std::string raw_request_line;
        int request_id;

    public:
        Request(std::string msg){
            raw_request_line = msg;
            request_id = next_request_id;
            next_request_id ++ ;
        }
        std::string getRequestLine();
};